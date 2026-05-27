#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include "api/Logger.hpp"
#include "network/Server.hpp"
#include <iostream>

int main() {
    try {
        catalog::Catalog catalog;
        core::Executor executor(catalog);
        api::Logger logger("db_server_access.log");

        network::Server server(8080, catalog, executor, logger);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка сервера: " << e.what() << "\n";
        return 1;
    }
    return 0;
}