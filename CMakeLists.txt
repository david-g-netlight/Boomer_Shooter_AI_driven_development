cmake_minimum_required(VERSION 3.20)
project(my_app CXX)

set(CMAKE_CXX_STANDARD 20) # C++20 for <concepts> and `requires` in templates
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- Engine (static library)
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/engine/*.cpp)
add_library(engine STATIC ${ENGINE_SOURCES})

target_include_directories(engine PUBLIC
    src/engine
    include
    include/raylib
)

# Static Raylib library
target_link_libraries(engine PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/libraylib.a
    "-framework CoreVideo"
    "-framework IOKit"
    "-framework Cocoa"
    "-framework GLUT"
    "-framework OpenGL"
)

# Bullet Physics: engine-private, the game never includes Bullet headers
target_include_directories(engine PRIVATE include/bullet)
target_link_libraries(engine PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/libBulletDynamics.a
    ${CMAKE_SOURCE_DIR}/lib/libBulletCollision.a
    ${CMAKE_SOURCE_DIR}/lib/libLinearMath.a
)

target_compile_options(engine PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall>
    $<$<CONFIG:Release>:-O2 -Wall>
)

# ---------------------------------------------------------------- Game (executable)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp)
add_executable(${PROJECT_NAME} src/main.cpp ${GAME_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src/game)
target_link_libraries(${PROJECT_NAME} PRIVATE engine)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall>
    $<$<CONFIG:Release>:-O2 -Wall>
)
