# Testing library
FetchContent_Declare(
        catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.13.6)
FetchContent_MakeAvailable(catch)
# Adds Catch2::Catch2

# Tests need to be added as executables first
add_executable(lock_test lock_test.cpp)

# Should be linked to the main library, as well as the Catch2 testing library
target_link_libraries(lock_test PRIVATE yarn Catch2::Catch2)

# If you register a test, then ctest and make test will run it.
# You can also run examples and check the output, as well.
add_test(NAME test_lock_test COMMAND lock_test) # Command can be a target
