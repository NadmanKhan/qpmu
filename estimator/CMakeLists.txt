add_library(${PROJECT_NAME}-estimator)
target_include_directories(
  ${PROJECT_NAME}-estimator
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../external/atcoder/ac-library)

target_link_libraries(${PROJECT_NAME}-estimator PUBLIC ${PROJECT_NAME}-common)

# Tests
add_executable(szcc_test tests/szcc_test.cpp)
target_link_libraries(szcc_test ${PROJECT_NAME}-estimator GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(szcc_test)
