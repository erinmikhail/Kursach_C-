#pragma once

#include "api/Logger.hpp"
#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace network {

class StorageServer {
public:
    StorageServer(int id, int port, std::string data_dir);
    ~StorageServer();

    void start();
    void stop();

private:
    void process_client(int client_socket);

    int id_;
    int port_;
    std::string data_dir_;
    int server_fd_ = -1;
    std::atomic<bool> running_{false};

    catalog::Catalog catalog_;
    core::Executor executor_;
    api::Logger logger_;
    std::mutex execute_mutex_;
};

} // namespace network
