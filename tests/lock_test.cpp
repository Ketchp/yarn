#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "primitives.hpp"


int test_lock_single_threaded() {
    Lock lock = Lock{};

    // we can lock and unlock
    lock.lock();
    lock.unlock();

    // first tryLock must work
    if( !lock.tryLock() )
        return 1;

    // second tryLock can't work
    if( lock.tryLock() )
        return 2;

    lock.unlock();

    return 0;
}


TEST_CASE( "Lock test single thread", "[main]" ) {
    REQUIRE( test_lock_single_threaded() == 0 );
}
