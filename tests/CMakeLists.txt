find_package(GTest CONFIG REQUIRED)

add_executable(entroui-tests
    core/test_core.cpp
    parser/test_parser.cpp
    style/test_style.cpp
    layout/test_layout.cpp
    events/test_events.cpp
    state/test_state.cpp
    integration/test_integration.cpp
)

target_link_libraries(entroui-tests PRIVATE entroui GTest::GTest GTest::Main)
target_compile_options(entroui-tests PRIVATE -Wall -Wextra)

include(GoogleTest)
gtest_discover_tests(entroui-tests)
