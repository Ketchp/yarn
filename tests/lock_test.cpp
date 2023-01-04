#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "primitives.hpp"
#include <thread>
#include <array>


auto more_threads( yarn::Lock &lock ) {
    uint32_t counter = 0;
    const uint32_t loops = 1 << 20, thread_count = 4;
    auto some_work = [&lock, &counter](){
        for(uint32_t i = 0; i < loops; i++) {
            lock.lock();
            counter++;
            lock.unlock();
        }
    };

    std::array<std::thread, thread_count> threads;
    for( auto &t: threads )
        t = std::thread{ some_work };

    for( auto &t: threads )
        t.join();

    return std::pair{loops * thread_count, counter};
}


TEST_CASE( "Lock tests", "[lock]" ) {
    yarn::Lock lock;

    SECTION( "Lock can be locked and unlocked, single thread." ) {
        for( uint8_t i = 0; i < 5; i++ ) {
            lock.lock();
            lock.unlock();
        }
    }

    SECTION( "Lock tryLock must return false on locked Lock" ) {
        for( uint8_t i = 0; i < 5; i++ ) {
            lock.lock();
            REQUIRE_FALSE( lock.tryLock() );
            lock.unlock();
        }
    }

    SECTION( "tryLock must work as lock on unlocked lock" ) {
        for( uint8_t i = 0; i < 5; i++ ) {
            REQUIRE( lock.tryLock() );
            REQUIRE_FALSE( lock.tryLock() );
            lock.unlock();
        }
    }

    SECTION( "lock with timeout must throw exception" ) {
        lock.lock();
        for( uint8_t i = 0; i < 5; i++ ) {
            REQUIRE_THROWS_AS( lock.lock( 10 ), yarn::TimeoutExpiredException );
        }
        lock.unlock();
    }

    SECTION( "Simple counting in more threads" ) {
        auto result = more_threads( lock );
        REQUIRE( result.first == result.second );
    }
}

TEST_CASE( "Congested locks", "[lock]" ) {
    constexpr uint32_t lock_count = 32, thread_count = 128;
    std::array<yarn::Lock, lock_count> locks;
    std::array<uint32_t, lock_count> counters{};
    std::fill(counters.begin(), counters.end(), 0);

    std::array<std::thread, thread_count> threads;

    uint32_t loops = 1 << 10;
    auto congestive_work = [&locks, &counters, loops](){
        for( uint32_t i = 0; i < loops; i++ ) {
            for( uint32_t idx = 0; idx < locks.size(); ++idx ) {
                auto &lock = locks[ idx ];
                auto &counter = counters[ idx ];
                if( i % 10 )
                    while( !lock.tryLock() );
                else lock.lock();

                ++counter;

                lock.unlock();

            }
        }
    };

    for( auto &thread: threads )
        thread = std::thread{ congestive_work };

    for( auto &thread: threads )
        thread.join();

    for( auto counter: counters )
        REQUIRE( counter == thread_count * loops );
}
