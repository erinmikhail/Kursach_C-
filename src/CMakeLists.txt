add_executable(Kursach
    main.cpp
    api/ConsoleInterface.cpp
)

target_include_directories(Kursach PRIVATE ${CMAKE_SOURCE_DIR}/include)