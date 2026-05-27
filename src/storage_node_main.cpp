#include "network/StorageServer.hpp"

#include <iostream>
#include <string>

namespace {

std::string arg_value(int argc, char** argv, const std::string& name, const std::string& fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) return argv[i + 1];
    }
    return fallback;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const int id = std::stoi(arg_value(argc, argv, "--id", "1"));
        const int port = std::stoi(arg_value(argc, argv, "--port", "9001"));
        const std::string data_dir = arg_value(argc, argv, "--data-dir", ".");

        network::StorageServer server(id, port, data_dir);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Storage fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
