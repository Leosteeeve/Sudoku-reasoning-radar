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
    src/SolveTrace.cpp
    src/CoreProtocol.cpp
)

target_compile_features(srr_core PUBLIC cxx_std_17)
target_include_directories(srr_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(EMSCRIPTEN)
    add_executable(srr_core_wasm src/WasmExports.cpp)
    target_link_libraries(srr_core_wasm PRIVATE srr_core)
    set_target_properties(srr_core_wasm PROPERTIES
        OUTPUT_NAME "srr-core"
        SUFFIX ".js"
    )
    target_link_options(srr_core_wasm PRIVATE
        "--no-entry"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sENVIRONMENT=web,worker"
        "-sFILESYSTEM=0"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sEXPORTED_FUNCTIONS=['_srr_dispatch','_srr_free','_malloc','_free']"
    )
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(srr_core_tests tests/core_tests.cpp)
    target_link_libraries(srr_core_tests PRIVATE srr_core)
    add_test(NAME srr_core_tests COMMAND srr_core_tests)

    add_executable(srr_core_protocol_tests tests/core_protocol_tests.cpp)
    target_link_libraries(srr_core_protocol_tests PRIVATE srr_core)
    add_test(NAME srr_core_protocol_tests COMMAND srr_core_protocol_tests)
endif()
