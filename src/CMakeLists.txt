set(HEADER_LIST "${yarn_SOURCE_DIR}/include/yarn/primitives.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(yarn primitives.cpp ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(yarn PUBLIC ../include/yarn)

# IDEs should put the headers in a nice place
source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header Files"
        FILES ${HEADER_LIST})
