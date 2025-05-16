cmake_minimum_required(VERSION 3.25)
project(event_manager)
set(PROJECT_NAMESPACE WillsonUtilities)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)

# ---- Dependencies ----
# Fetch fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.x
)
FetchContent_MakeAvailable(googletest)

add_library(${PROJECT_NAME}_lib
    src/event_manager.cpp
)
add_library(${PROJECT_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}_lib)


target_include_directories(${PROJECT_NAME}_lib PUBLIC include)
target_link_libraries(event_manager_lib PUBLIC fmt::fmt)



# ---- Tests (Only if standalone) ----
if (PROJECT_IS_TOP_LEVEL)
    enable_testing()

    # Define test sources
    file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)

    add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME}_tests
        PRIVATE
            ${PROJECT_NAME}_lib
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
endif()
