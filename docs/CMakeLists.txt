set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")


doxygen_add_docs(docs
        "${PROJECT_SOURCE_DIR}/README.md" yarn/primitives.hpp
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/include")
