file(GLOB SOURCES CONFIGURE_DEPENDS
    "src/*.cpp" "src/*.c"
    "src/*/*.cpp" "src/*/*.c"
    "include/sc-config-utils/*.h" "include/sc-config-utils/*.hpp"
    "include/sc-config-utils/*/*.h" "include/sc-config-utils/*/*.hpp"
)

add_library(sc-config-utils SHARED ${SOURCES})
target_link_libraries(sc-config-utils
    LINK_PRIVATE ${GLIB2_LIBRARIES}
    LINK_PUBLIC sc-memory
)
target_include_directories(sc-config-utils
    PRIVATE ${GLIB2_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(${SC_CLANG_FORMAT_CODE})
    target_clangformat_setup(sc-config-utils)
endif()
