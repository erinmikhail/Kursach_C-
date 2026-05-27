#include "network/EntryPointServer.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

std::string arg_value(int argc, char** argv, const std::string& name, const std::string& fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) return argv[i + 1];
    }
    return fallback;
}

std::string default_storage_binary(const char* argv0) {
    std::filesystem::path exe(argv0);
    std::filesystem::path dir = exe.parent_path();
    if (dir.empty()) dir = ".";
    return (dir / "DbStorage").string();
}

} // namespace

int main(int argc, char** argv) {
    try {
        const int port = std::stoi(arg_value(argc, argv, "--port", "8080"));
        const size_t storage_count = static_cast<size_t>(std::stoul(arg_value(argc, argv, "--storage-count", "2")));
        const int storage_base_port = std::stoi(arg_value(argc, argv, "--storage-base-port", "9001"));
        const std::string data_dir = arg_value(argc, argv, "--data-dir", "cluster_data");
        const std::string storage_binary = arg_value(argc, argv, "--storage-binary", default_storage_binary(argv[0]));

        network::EntryPointServer server(port, storage_count, storage_base_port, data_dir, storage_binary);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "EntryPoint fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
