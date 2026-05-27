#include "security/AuthManager.hpp"
#include <fstream>
#include <sstream>
#include <random>
#include <functional>
#include <iostream>

namespace security {

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

AuthManager::AuthManager(const std::string& data_dir) : data_dir_(data_dir) {
    load_data();
    // Создаем админа по умолчанию, если пользователей нет
    if (users_.empty()) {
        create_user("admin", "admin");
        grant_permission("admin", "*", Permission::ALL); // Доступ ко всему
    }
}

AuthManager::~AuthManager() {
    save_data();
}

std::string AuthManager::generate_salt() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    std::string salt = "";
    for (int i = 0; i < 8; ++i) {
        salt += static_cast<char>(dis(gen));
    }
    return base64_encode(salt);
}

size_t AuthManager::hash_password(const std::string& password, const std::string& salt) {
    std::hash<std::string> hasher;
    // Хеширование с солью
    return hasher(salt + password + salt); 
}

std::string AuthManager::base64_encode(const std::string& in) {
    std::string out;
    uint32_t val = 0;
    int valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string AuthManager::base64_decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;
    uint32_t val = 0;
    int valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string AuthManager::sign_data(const std::string& data) {
    std::hash<std::string> hasher;
    size_t sig = hasher(data + secret_key_);
    return base64_encode(std::to_string(sig));
}

void AuthManager::create_user(const std::string& username, const std::string& password) {
    if (users_.find(username) != users_.end()) {
        throw std::runtime_error("Пользователь уже существует");
    }
    User u;
    u.username = username;
    u.salt = generate_salt();
    u.password_hash = hash_password(password, u.salt);
    users_[username] = u;
    save_data();
}

std::string AuthManager::login(const std::string& username, const std::string& password) {
    auto it = users_.find(username);
    if (it == users_.end()) throw std::runtime_error("Неверный логин или пароль");
    
    if (it->second.password_hash != hash_password(password, it->second.salt)) {
        throw std::runtime_error("Неверный логин или пароль");
    }

    // Генерация JWT
    std::string header = base64_encode("{\"alg\":\"HS256\",\"typ\":\"JWT\"}");
    std::string payload = base64_encode("{\"user\":\"" + username + "\"}");
    std::string signature = sign_data(header + "." + payload);
    
    return header + "." + payload + "." + signature;
}

std::string AuthManager::validate_jwt(const std::string& jwt) {
    size_t first_dot = jwt.find('.');
    size_t second_dot = jwt.rfind('.');
    if (first_dot == std::string::npos || second_dot == std::string::npos || first_dot == second_dot) {
        throw std::runtime_error("Неверный формат токена");
    }

    std::string header = jwt.substr(0, first_dot);
    std::string payload = jwt.substr(first_dot + 1, second_dot - first_dot - 1);
    std::string signature = jwt.substr(second_dot + 1);

    if (sign_data(header + "." + payload) != signature) {
        throw std::runtime_error("Подпись токена недействительна");
    }

    std::string decoded_payload = base64_decode(payload);
    // Простой парсинг JSON {"user":"admin"}
    size_t u_pos = decoded_payload.find("\"user\":\"");
    if (u_pos == std::string::npos) throw std::runtime_error("Токен поврежден");
    u_pos += 8;
    size_t u_end = decoded_payload.find("\"", u_pos);
    
    return decoded_payload.substr(u_pos, u_end - u_pos);
}

void AuthManager::grant_permission(const std::string& username, const std::string& db_name, Permission perm) {
    permissions_[username][db_name].push_back(perm);
    save_data();
}

bool AuthManager::check_permission(const std::string& username, const std::string& db_name, parser::StatementType stmt_type) {
    if (username == "admin") return true;

    auto u_it = permissions_.find(username);
    if (u_it == permissions_.end()) return false;

    // Ищем права для конкретной БД или глобальные ("*")
    auto db_it = u_it->second.find(db_name);
    if (db_it == u_it->second.end()) {
        db_it = u_it->second.find("*");
        if (db_it == u_it->second.end()) return false;
    }

    Permission required_perm;
    switch (stmt_type) {
        case parser::StatementType::SELECT: required_perm = Permission::READ; break;
        case parser::StatementType::INSERT:
        case parser::StatementType::UPDATE:
        case parser::StatementType::DELETE: required_perm = Permission::WRITE; break;
        case parser::StatementType::CREATE_TABLE: required_perm = Permission::CREATE_TABLE; break;
        case parser::StatementType::DROP_TABLE: required_perm = Permission::DROP_TABLE; break;
        case parser::StatementType::DROP_DATABASE: required_perm = Permission::DROP_DB; break;
        default: return true; 
    }

    for (Permission p : db_it->second) {
        if (p == Permission::ALL || p == required_perm) return true;
    }
    return false;
}

void AuthManager::load_data() {
    std::ifstream in(data_dir_, std::ios::binary);
    if (!in.is_open()) return;

    uint32_t user_count = 0;
    if (!in.read(reinterpret_cast<char*>(&user_count), sizeof(user_count))) return;

    for(uint32_t i = 0; i < user_count; ++i) {
        uint32_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        std::string name(name_len, '\0');
        in.read(&name[0], name_len);

        User u;
        u.username = name;
        in.read(reinterpret_cast<char*>(&u.password_hash), sizeof(u.password_hash));

        uint32_t salt_len = 0;
        in.read(reinterpret_cast<char*>(&salt_len), sizeof(salt_len));
        u.salt.resize(salt_len);
        in.read(&u.salt[0], salt_len);

        users_[name] = u;
    }

    uint32_t perm_count = 0;
    if (!in.read(reinterpret_cast<char*>(&perm_count), sizeof(perm_count))) return;

    for (uint32_t i = 0; i < perm_count; ++i) {
        uint32_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        std::string username(name_len, '\0');
        in.read(&username[0], name_len);

        uint32_t db_count = 0;
        in.read(reinterpret_cast<char*>(&db_count), sizeof(db_count));
        for (uint32_t j = 0; j < db_count; ++j) {
            uint32_t db_len = 0;
            in.read(reinterpret_cast<char*>(&db_len), sizeof(db_len));
            std::string db_name(db_len, '\0');
            in.read(&db_name[0], db_len);

            uint32_t p_count = 0;
            in.read(reinterpret_cast<char*>(&p_count), sizeof(p_count));
            std::vector<Permission> perms(p_count);
            in.read(reinterpret_cast<char*>(perms.data()), p_count * sizeof(Permission));
            
            permissions_[username][db_name] = perms;
        }
    }
}

void AuthManager::save_data() {
    std::ofstream out(data_dir_, std::ios::binary);
    if (!out.is_open()) return;

    uint32_t user_count = users_.size();
    out.write(reinterpret_cast<const char*>(&user_count), sizeof(user_count));
    for (const auto& [name, u] : users_) {
        uint32_t name_len = name.size();
        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name.data(), name_len);
        out.write(reinterpret_cast<const char*>(&u.password_hash), sizeof(u.password_hash));
        uint32_t salt_len = u.salt.size();
        out.write(reinterpret_cast<const char*>(&salt_len), sizeof(salt_len));
        out.write(u.salt.data(), salt_len);
    }

    uint32_t perm_count = permissions_.size();
    out.write(reinterpret_cast<const char*>(&perm_count), sizeof(perm_count));
    for (const auto& [name, db_map] : permissions_) {
        uint32_t name_len = name.size();
        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name.data(), name_len);
        
        uint32_t db_count = db_map.size();
        out.write(reinterpret_cast<const char*>(&db_count), sizeof(db_count));
        for (const auto& [db, perms] : db_map) {
            uint32_t db_len = db.size();
            out.write(reinterpret_cast<const char*>(&db_len), sizeof(db_len));
            out.write(db.data(), db_len);
            uint32_t p_count = perms.size();
            out.write(reinterpret_cast<const char*>(&p_count), sizeof(p_count));
            out.write(reinterpret_cast<const char*>(perms.data()), p_count * sizeof(Permission));
        }
    }
}

} 