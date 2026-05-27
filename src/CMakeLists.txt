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
    network/Server.cpp  
)

target_include_directories(db_engine_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(Kursach main.cpp)
target_link_libraries(Kursach PRIVATE db_engine_lib nlohmann_json::nlohmann_json)

# Сервер
add_executable(DbServer server_main.cpp) 
target_link_libraries(DbServer PRIVATE db_engine_lib nlohmann_json::nlohmann_json)

# Клиент
add_executable(DbClient client_main.cpp) 