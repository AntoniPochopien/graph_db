cmake_minimum_required(VERSION 3.10)

project(graphdb_ffi LANGUAGES CXX)

# ===============================
# Compiler configuration
# ===============================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===============================
# Source files list
# (adjust paths if your structure is different)
# ===============================
set(SOURCES
    graph/infrastructure/graph.cpp
    graph/infrastructure/node.cpp
    graph/infrastructure/edge.cpp
    graph/infrastructure/property.cpp
    storage/infrastructure/storage.cpp
    graph_db_c_api.cpp
  )

find_library(log-lib log)  # szuka liblog.so

# ===============================
# Create a shared library (.so for Android / .a for iOS)
# ===============================
add_library(graph_db SHARED ${SOURCES})

# ===============================
# Include directories for header files
# These paths tell the compiler where to find .hpp/.h files
# ===============================
target_include_directories(graph_db PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/graph/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/storage/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/graph/infrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/storage/infrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

# ===============================
# Definitions for Flutter FFI
# DART_SHARED_LIB is required by Flutter's CMake template
# ===============================
target_compile_definitions(graph_db PUBLIC DART_SHARED_LIB)

# ===============================
# Link required system libraries
# On Android, stdc++fs is needed for <filesystem> support,
# and 'log' is used for Android logging
# ===============================
if (ANDROID)
    target_link_libraries(graph_db log)
else()
    target_link_libraries(graph_db)
endif()

# ===============================
# Output settings (optional)
# Define output name and public headers
# ===============================
set_target_properties(graph_db PROPERTIES
    PUBLIC_HEADER ""
    OUTPUT_NAME "graph_db"
)
