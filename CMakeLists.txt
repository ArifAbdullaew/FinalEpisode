cmake_minimum_required(VERSION 3.22)
project(saper)

set(CMAKE_CXX_STANDARD 23)

set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML COMPONENTS graphics window system audio)


add_executable(SaperProject Saper.cpp)
target_link_libraries(SaperProject PUBLIC sfml-graphics sfml-window sfml-system sfml-audio sfml-network)

enable_testing()
add_subdirectory(doctest)

#add_executable(SaperProject_test Source/test.cpp)
add_executable(SaperProject_test Source/test.cpp Source/src.cpp)
target_link_libraries(SaperProject_test PUBLIC doctest sfml-audio sfml-graphics sfml-window sfml-system sfml-network)


file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/openal32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/audio DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/material DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
