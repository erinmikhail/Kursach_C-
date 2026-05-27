add_library(db_engine_lib STATIC
    api/ConsoleInterface.cpp
    api/Logger.cpp
    parser/Lexer.cpp
    parser/Parser.cpp
    storage/Pager.cpp
    storage/BufferPool.cpp
    storage/RecordManager.cpp
    storage/StringPool.cpp
    storage/Table.cpp
    core/Executor.cpp
    core/QueryRunner.cpp
    network/SocketUtils.cpp
    network/Server.cpp
    network/StorageServer.cpp
    network/ClusterMetadata.cpp
    network/ClusterManager.cpp
    network/EntryPointServer.cpp
    security/AuthManager.cpp
)

target_include_directories(db_engine_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(db_engine_lib PUBLIC nlohmann_json::nlohmann_json)

add_executable(Kursach main.cpp)
target_link_libraries(Kursach PRIVATE db_engine_lib nlohmann_json::nlohmann_json)

# Сервер
add_executable(DbServer server_main.cpp) 
target_link_libraries(DbServer PRIVATE db_engine_lib nlohmann_json::nlohmann_json)

# Клиент
add_executable(DbClient client_main.cpp) 
# Кластерный Storage-узел
add_executable(DbStorage storage_node_main.cpp)
target_link_libraries(DbStorage PRIVATE db_engine_lib nlohmann_json::nlohmann_json)

# Кластерный EntryPoint
add_executable(DbEntryPoint entrypoint_main.cpp)
target_link_libraries(DbEntryPoint PRIVATE db_engine_lib nlohmann_json::nlohmann_json)
