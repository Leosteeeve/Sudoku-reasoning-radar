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

add_executable(srr_ocr_helper
    src/OcrSidecarMain.cpp
    src/OcrSidecarProtocol.cpp
)
target_compile_features(srr_ocr_helper PRIVATE cxx_std_17)
target_include_directories(srr_ocr_helper PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(srr_ocr_helper PROPERTIES OUTPUT_NAME "srr-ocr-helper")

find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs)
find_package(Tesseract QUIET)
if(OpenCV_FOUND AND Tesseract_FOUND)
    target_compile_definitions(srr_ocr_helper PRIVATE SRR_OCR_ENABLED=1)
    target_sources(srr_ocr_helper PRIVATE
        src/OCRImport.cpp
        src/ImagePreprocessor.cpp
        src/DigitRecognizer.cpp
        src/OCRReviewState.cpp
    )
    target_include_directories(srr_ocr_helper PRIVATE ${OpenCV_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS})
    target_link_libraries(srr_ocr_helper PRIVATE srr_core ${OpenCV_LIBS} ${Tesseract_LIBRARIES})
endif()

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
        "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','lengthBytesUTF8']"
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

    add_executable(srr_ocr_sidecar_protocol_tests
        tests/ocr_sidecar_protocol_tests.cpp
        src/OcrSidecarProtocol.cpp
    )
    target_include_directories(srr_ocr_sidecar_protocol_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(srr_ocr_sidecar_protocol_tests PRIVATE cxx_std_17)
    add_test(NAME srr_ocr_sidecar_protocol_tests COMMAND srr_ocr_sidecar_protocol_tests)
endif()
