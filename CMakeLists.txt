cmake_minimum_required(VERSION 4.2)

project(rsc-assignment CXX)

find_package(nlohmann_json REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(pybind11_json CONFIG REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")

set(SOURCES
    src/entity.cpp
    src/model.cpp
    src/world.cpp
)

add_compile_options(-Werror -Wall -Wextra -Weffc++ -fPIC)

pybind11_add_module(mt-bh src/bh.cpp)
target_include_directories(mt-bh PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(mt-bh PUBLIC nlohmann_json::nlohmann_json pybind11_json::pybind11_json)
install(TARGETS mt-bh
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
set_target_properties(mt-bh PROPERTIES
                      OUTPUT_NAME "PyModel")