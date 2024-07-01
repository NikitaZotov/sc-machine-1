if(${SC_BUILD_BENCH})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/performance)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/sc-server)
