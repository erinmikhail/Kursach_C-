#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include "api/ConsoleInterface.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    catalog::Catalog catalog;            
    core::Executor executor(catalog);    

    api::REPL repl(catalog, executor);
    
    if (argc == 1) {
        repl.start();
    } else if (argc == 2) {
        repl.run_batch(argv[1]);
    } else {
        std::cerr << "Использование:\n"
                  << "  " << argv[0] << "              - интерактивный режим\n"
                  << "  " << argv[0] << " <script.sql> - пакетный режим\n";
        return 1;
    }
    
    return 0;
}