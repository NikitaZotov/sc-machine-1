configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/units/test_defines.hpp.in"
    "${CMAKE_CURRENT_LIST_DIR}/units/test_defines.hpp"
)

make_tests_from_folder(${CMAKE_CURRENT_LIST_DIR}/units
    NAME sc-server-tests
    DEPENDS sc-server-lib
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/../include"
)

if(${SC_CLANG_FORMAT_CODE})
    target_clangformat_setup(sc-server-tests)
endif()
