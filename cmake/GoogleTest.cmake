include(FetchContent)

# ---- Fetch GoogleTest ----
FetchContent_Declare(
    googletest
    URL                         https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP  TRUE
)

# Prevent GoogleTest from overriding your compiler options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()
