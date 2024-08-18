add_executable(main main.cpp)
add_library(vects vects.cpp)
add_library(chunks chunks.cpp)

target_include_directories(
    vects
    PUBLIC
    .
)

target_include_directories(
    chunks
    PUBLIC
    .
)

target_link_libraries(
    main
#    vects
    SDL2
    SDL2main
)