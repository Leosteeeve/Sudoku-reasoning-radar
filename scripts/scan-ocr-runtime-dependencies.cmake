cmake_minimum_required(VERSION 3.24)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

if(NOT DEFINED SRR_HELPER OR NOT EXISTS "${SRR_HELPER}")
    message(FATAL_ERROR "SRR_HELPER must name the built OCR helper executable")
endif()
if(NOT DEFINED SRR_OUTPUT)
    message(FATAL_ERROR "SRR_OUTPUT must name the dependency-list output file")
endif()

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${SRR_HELPER}"
    DIRECTORIES ${SRR_SEARCH_DIRS}
    RESOLVED_DEPENDENCIES_VAR resolved_dependencies
    UNRESOLVED_DEPENDENCIES_VAR unresolved_dependencies
    CONFLICTING_DEPENDENCIES_PREFIX dependency_conflicts
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"
)

if(unresolved_dependencies)
    list(JOIN unresolved_dependencies ", " unresolved_text)
    message(FATAL_ERROR "Unresolved OCR runtime dependencies: ${unresolved_text}")
endif()
if(dependency_conflicts_FILENAMES)
    list(JOIN dependency_conflicts_FILENAMES ", " conflict_text)
    message(FATAL_ERROR "Conflicting OCR runtime dependencies: ${conflict_text}")
endif()

list(REMOVE_DUPLICATES resolved_dependencies)
list(SORT resolved_dependencies)
file(WRITE "${SRR_OUTPUT}" "")
foreach(dependency IN LISTS resolved_dependencies)
    file(APPEND "${SRR_OUTPUT}" "${dependency}\n")
endforeach()
