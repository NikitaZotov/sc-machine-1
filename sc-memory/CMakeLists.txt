set(SC_CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/sc-core/src")

add_subdirectory(sc-core)
add_subdirectory(sc-memory)

if(${SC_BUILD_TESTS})
    add_subdirectory(tests)
endif()
