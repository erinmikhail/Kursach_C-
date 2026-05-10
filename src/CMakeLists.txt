add_executable(Kursach
    main.cpp
    api/ConsoleInterface.cpp
    api/Logger.cpp
    parser/Lexer.cpp
    parser/Parser.cpp
)

target_include_directories(Kursach PRIVATE ${CMAKE_SOURCE_DIR}/include)
