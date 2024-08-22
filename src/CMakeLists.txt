add_library(tmp_extencions INTERFACE)
target_include_directories(
    tmp_extencions
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(vects vects.cpp)
target_include_directories(
    vects
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    vects
    PUBLIC
    tmp_extencions
)

add_library(chunks chunks.cpp)
target_include_directories(
    chunks
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    chunks
    PUBLIC
    vects
)

add_executable(main main.cpp)
target_link_libraries(
    main
    vects
    SDL2
    SDL2main
)