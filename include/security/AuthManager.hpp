#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include "parser/Parser.hpp"

namespace security {

enum class Permission {
    READ,
    WRITE,
    CREATE_TABLE,
    DROP_TABLE,
    DROP_DB,
    ALL
};

struct User {
    std::string username;
    std::string salt;
    size_t password_hash;
};

class AuthManager {
public:
    AuthManager(const std::string& data_dir);
    ~AuthManager();

    // Аутентификация
    void create_user(const std::string& username, const std::string& password);
    std::string login(const std::string& username, const std::string& password);
    std::string validate_jwt(const std::string& jwt); // Возвращает username

    // Авторизация (RBAC)
    void grant_permission(const std::string& username, const std::string& db_name, Permission perm);
    bool check_permission(const std::string& username, const std::string& db_name, parser::StatementType stmt_type);

private:
    std::string data_dir_;
    std::string secret_key_ = "SuperSecretKursachKey";

    std::unordered_map<std::string, User> users_;
    // username -> db_name -> list of permissions
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Permission>>> permissions_;

    void load_data();
    void save_data();

    // Утилиты
    std::string generate_salt();
    size_t hash_password(const std::string& password, const std::string& salt);
    std::string base64_encode(const std::string& in);
    std::string base64_decode(const std::string& in);
    std::string sign_data(const std::string& data);
};

}