cmake_minimum_required(VERSION 3.16)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
SET(CMAKE_BUILD_TYPE Debug)

project(shrg_graph_parser_legacy)

add_definitions(
    -D_GLIBCXX_USE_CXX11_ABI=0
    -DBOOST_NO_CXX98_FUNCTION_BASE
    -DBOOST_ALLOW_DEPRECATED_HEADERS
    -DBOOST_NO_CXX11_TEMPLATE_ALIASES
)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -include functional")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_PROFILING "Enable gprof" OFF)
option(ENABLE_PARSER_CHECK "Enable parser check" OFF)
option(USE_SYSTEM_BOOST "Use system FindBoost.cmake" OFF)

if(ENABLE_PROFILING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

if(ENABLE_PARSER_CHECK)
  add_definitions( -DSHRG_PARSER_CHECK )
endif()

# Boost configuration
if(USE_SYSTEM_BOOST)
  find_package(Boost REQUIRED)
  message(STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}")
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(STATUS "Boost_INCLUDE_DIRS = ${PROJECT_SOURCE_DIR}/third-party/")
  include_directories(${PROJECT_SOURCE_DIR}/third-party/)
endif()

# Include directories
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/third-party/sparsehash-c11
    ${PROJECT_SOURCE_DIR}/third-party/sparsehash-c11/sparsehash
    ${PROJECT_SOURCE_DIR}/src
)
include_directories(/Library/Developer/CommandLineTools/usr/include/c++/v1)

# ===== CORE LIBRARY =====

# Core SHRG graph parser library
file(GLOB GRAPH_PARSER_SOURCES "src/graph_parser/*.cpp")
add_library(shrg STATIC
    "src/include/bleu.cpp"
    ${GRAPH_PARSER_SOURCES}
)
set_target_properties(shrg PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# ===== LEGACY EM FRAMEWORK LIBRARY =====

# Legacy EM framework (original implementation)
file(GLOB EM_LEGACY_SOURCES
    "src/em_framework/find_derivations.cpp"
    "src/em_framework/em_base.cpp"
    "src/em_framework/em.cpp"
    "src/em_framework/em_utils.cpp"
    "src/em_framework/em_batch.cpp"
    "src/em_framework/em_viterbi.cpp"
    "src/em_framework/em_online.cpp"
    "src/em_framework/em_evaluate/eval_deriv.cpp"
)
add_library(em_legacy STATIC
    ${EM_LEGACY_SOURCES}
    "src/manager.cpp"
)
target_link_libraries(em_legacy PRIVATE shrg)

# ===== LEGACY EXECUTABLES =====

# Legacy EM algorithm runners
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/run_em.cpp")
    add_executable(run_em
        "src/run_em.cpp"
    )
    target_link_libraries(run_em PRIVATE em_legacy shrg)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/run_batch_em.cpp")
    add_executable(run_batch_em
        "src/run_batch_em.cpp"
    )
    target_link_libraries(run_batch_em PRIVATE em_legacy shrg)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/run_em_viterbi.cpp")
    add_executable(run_em_viterbi
        "src/run_em_viterbi.cpp"
    )
    target_link_libraries(run_em_viterbi PRIVATE em_legacy shrg)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/run_online_em.cpp")
    add_executable(run_online_em
        "src/run_online_em.cpp"
    )
    target_link_libraries(run_online_em PRIVATE em_legacy shrg)
endif()

# ===== UTILITY EXECUTABLES =====

# Generation utility
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/generate.cpp")
    add_executable(generate
        "src/generate.cpp"
    )
    target_link_libraries(generate PRIVATE em_legacy shrg)
endif()

# Evaluation utility
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/evaluate.cpp")
    add_executable(evaluate
        "src/evaluate.cpp"
    )
    target_link_libraries(evaluate PRIVATE em_legacy shrg)
endif()

# Sentence processing utilities
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/get_original_sentences.cpp")
    add_executable(add_original_sentences
        "src/get_original_sentences.cpp"
    )
    target_link_libraries(add_original_sentences PRIVATE em_legacy shrg)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/get_lemma_sentences.cpp")
    add_executable(get_lemma_sentences
        "src/get_lemma_sentences.cpp"
    )
    target_link_libraries(get_lemma_sentences PRIVATE em_legacy shrg)
endif()

# ===== INSTALL CONFIGURATION =====

# Collect all available legacy executables
set(LEGACY_EXECUTABLES)

# EM executables
if(TARGET run_em)
    list(APPEND LEGACY_EXECUTABLES run_em)
endif()
if(TARGET run_batch_em)
    list(APPEND LEGACY_EXECUTABLES run_batch_em)
endif()
if(TARGET run_em_viterbi)
    list(APPEND LEGACY_EXECUTABLES run_em_viterbi)
endif()
if(TARGET run_online_em)
    list(APPEND LEGACY_EXECUTABLES run_online_em)
endif()

# Utility executables
if(TARGET generate)
    list(APPEND LEGACY_EXECUTABLES generate)
endif()
if(TARGET evaluate)
    list(APPEND LEGACY_EXECUTABLES evaluate)
endif()
if(TARGET add_original_sentences)
    list(APPEND LEGACY_EXECUTABLES add_original_sentences)
endif()
if(TARGET get_lemma_sentences)
    list(APPEND LEGACY_EXECUTABLES get_lemma_sentences)
endif()

# Parser tools
if(TARGET shrg_parser)
    list(APPEND LEGACY_EXECUTABLES shrg_parser)
endif()
if(TARGET check_parser)
    list(APPEND LEGACY_EXECUTABLES check_parser)
endif()
if(TARGET treewidth)
    list(APPEND LEGACY_EXECUTABLES treewidth)
endif()
if(TARGET manager_test)
    list(APPEND LEGACY_EXECUTABLES manager_test)
endif()

# Install executables
if(LEGACY_EXECUTABLES)
    install(TARGETS ${LEGACY_EXECUTABLES}
        DESTINATION bin
        OPTIONAL
    )
endif()

# Install libraries
install(TARGETS shrg em_legacy
    DESTINATION lib
    OPTIONAL
)

# Install headers (for development)
install(DIRECTORY src/graph_parser/
    DESTINATION include/graph_parser
    FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY src/em_framework/
    DESTINATION include/em_framework
    FILES_MATCHING PATTERN "*.hpp"
)

# ===== BUILD SUMMARY =====

message(STATUS "=== Legacy Build Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Profiling enabled: ${ENABLE_PROFILING}")
message(STATUS "Parser check enabled: ${ENABLE_PARSER_CHECK}")
message(STATUS "Using system Boost: ${USE_SYSTEM_BOOST}")
message(STATUS "===============================================")

# Print available targets
message(STATUS "Legacy EM targets:")
if(TARGET run_em)
    message(STATUS "  run_em                     - Standard EM algorithm")
endif()
if(TARGET run_batch_em)
    message(STATUS "  run_batch_em               - Batch EM algorithm")
endif()
if(TARGET run_em_viterbi)
    message(STATUS "  run_em_viterbi             - Viterbi EM algorithm")
endif()
if(TARGET run_online_em)
    message(STATUS "  run_online_em              - Online EM algorithm")
endif()

message(STATUS "")
message(STATUS "Utility targets:")
if(TARGET generate)
    message(STATUS "  generate                   - Graph generation")
endif()
if(TARGET evaluate)
    message(STATUS "  evaluate                   - Evaluation utilities")
endif()
if(TARGET add_original_sentences)
    message(STATUS "  add_original_sentences     - Add original sentences")
endif()
if(TARGET get_lemma_sentences)
    message(STATUS "  get_lemma_sentences        - Get lemma sentences")
endif()

message(STATUS "")
message(STATUS "Parser tools:")
if(TARGET shrg_parser)
    message(STATUS "  shrg_parser               - SHRG parser tool")
endif()
if(TARGET check_parser)
    message(STATUS "  check_parser              - Parser checker")
endif()
if(TARGET treewidth)
    message(STATUS "  treewidth                 - Tree width computation")
endif()
if(TARGET manager_test)
    message(STATUS "  manager_test              - Manager testing tool")
endif()

message(STATUS "===============================================")

# Print library information
message(STATUS "Libraries:")
message(STATUS "  shrg                       - Core SHRG graph parser")
message(STATUS "  em_legacy                  - Legacy EM framework")
message(STATUS "===============================================")

# Helpful build commands
message(STATUS "Build commands:")
message(STATUS "  cmake --build . --target evaluate               # Build evaluation tool")
message(STATUS "  cmake --build . --target generate               # Build generation tool")
message(STATUS "  cmake --build . --target run_batch_em           # Build batch EM runner")
message(STATUS "  cmake --build . --target all                    # Build all targets")
message(STATUS "===============================================")
add_executable(get_deriv src/get_derivation_edges.cpp)
target_link_libraries(get_deriv PRIVATE em_legacy shrg)

#add_executable(get_deriv_rule_indices src/get_deriv_rule_indices.cpp)
#target_link_libraries(get_deriv_rule_indices PRIVATE em_legacy shrg)add_executable(count_node src/count_nodes.cpp)

# ===== AMBIGUITY METRICS LIBRARY =====

# Ambiguity metrics library for computing derivation entropy, expected count, and forest complexity
file(GLOB AMBIGUITY_METRICS_SOURCES "src/ambiguity_metrics/*.cpp")
add_library(ambiguity_metrics STATIC ${AMBIGUITY_METRICS_SOURCES})
target_include_directories(ambiguity_metrics PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ambiguity_metrics PRIVATE shrg)
set_target_properties(ambiguity_metrics PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Ambiguity metrics computation executable
add_executable(compute_ambiguity_metrics src/compute_ambiguity_metrics.cpp)
target_link_libraries(compute_ambiguity_metrics PRIVATE ambiguity_metrics em_legacy shrg)

# Derivation count executable (fast, no parents_sib computation)
add_executable(count_derivations src/count_derivations.cpp)
target_link_libraries(count_derivations PRIVATE ambiguity_metrics em_legacy shrg)
