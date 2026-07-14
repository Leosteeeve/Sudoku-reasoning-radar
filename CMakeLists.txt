cmake_minimum_required(VERSION 3.24)

project(SudokuReasoningRadar LANGUAGES CXX)

add_library(srr_core STATIC
    src/Board.cpp
    src/Solver.cpp
    src/DLXSolver.cpp
    src/StepRecorder.cpp
    src/PuzzleGenerator.cpp
    src/HintCoach.cpp
    src/DifficultyAnalyzer.cpp
    src/PuzzleString.cpp
)

target_compile_features(srr_core PUBLIC cxx_std_17)
target_include_directories(srr_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

include(CTest)

if(BUILD_TESTING)
    add_executable(srr_core_tests tests/core_tests.cpp)
    target_link_libraries(srr_core_tests PRIVATE srr_core)
    add_test(NAME srr_core_tests COMMAND srr_core_tests)
endif()
