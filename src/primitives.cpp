#include "primitives.hpp"

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <ctime>
#include <algorithm>

using namespace yarn;

static inline long
simple_futex( const uint32_t *uaddr,
       int futex_op,
       uint32_t val,
       const struct timespec *timeout = nullptr ) {
    return syscall( SYS_futex, uaddr, futex_op, val, timeout, nullptr, 0 );
}

static long
time_diff_ns( struct timespec *now, struct timespec *before ) {
    return 1000000l * ( now->tv_sec - before->tv_sec - 1 ) + ( 1000000000l + now->tv_nsec - before->tv_nsec ) / 1000;
}


TimeoutExpiredException::TimeoutExpiredException( const char *msg )
    : error_msg( msg ) {}

char *TimeoutExpiredException::what() {
    return error_msg.data();
}


Lock::Lock( uint32_t spinlock_time_ns ) noexcept
    : spin_time( spinlock_time_ns ) {}

void Lock::lock() noexcept {
    if( tryLock() )
        return;

    struct timespec start_time{};
    clock_gettime( CLOCK_MONOTONIC, &start_time );

    while( true ) {
        // only read loop to decrease cache invalidation by CMPXCHG
        // atomic swap does not execute unless lock value is observed as 0
        while( true ) {
            if( tryLock() )
                return;

            struct timespec now{};
            clock_gettime( CLOCK_MONOTONIC, &now );

            if( time_diff_ns( &now, &start_time) >= spin_time )
                break;
        }
        __sync_add_and_fetch( &waiter_count, 1 );
        simple_futex(&lock_value, FUTEX_WAIT, 1 );
        __sync_sub_and_fetch( &waiter_count, 1 );
    }
}

void Lock::lock( uint32_t timeout_ns ) {
    if( tryLock() )
        return;

    struct timespec start_time{}, now{};
    clock_gettime( CLOCK_MONOTONIC, &start_time );

    while( true ) {
        // only read loop to decrease cache invalidation by CMPXCHG
        // atomic swap does not execute unless lock value is observed as 0
        while( true ) {
            if( tryLock() )
                return;

            clock_gettime( CLOCK_MONOTONIC, &now );

            if( time_diff_ns( &now, &start_time) >= std::min( spin_time, timeout_ns ) )
                break;
        }

        long time_diff = time_diff_ns( &now, &start_time );

        if( time_diff >= timeout_ns )
            throw TimeoutExpiredException( "Timeout expired before lock was possible." );

        // time remaining for sleep
        time_diff -= timeout_ns;

        // store back to now struct
        now.tv_sec = time_diff / 1000000;
        now.tv_nsec = ( time_diff % 1000000 ) * 1000;

        __sync_add_and_fetch( &waiter_count, 1 );
        simple_futex(&lock_value, FUTEX_WAIT, 1, &now );
        __sync_sub_and_fetch( &waiter_count, 1 );

        // measure time if timeout expired
        clock_gettime( CLOCK_MONOTONIC, &now );
        if( time_diff_ns( &now, &start_time ) >= timeout_ns )
            throw TimeoutExpiredException( "Timeout expired before lock was possible." );
    }
}

[[nodiscard]] inline bool Lock::tryLock() noexcept {
    return lock_value == 0 && __sync_bool_compare_and_swap( &lock_value, 0, 1 );
}

void Lock::unlock() noexcept {
    lock_value = 0;
    if( waiter_count )
        simple_futex( &lock_value, FUTEX_WAKE, 1 );
}


Semaphore::Semaphore( uint32_t initial_value, uint32_t spinlock_time_ns ) noexcept
    : value( initial_value ), spin_time( spinlock_time_ns ) {}

void Semaphore::take() noexcept {
    if( tryTake() )
        return;

    struct timespec start_time{};
    clock_gettime( CLOCK_MONOTONIC, &start_time );

    while( true ) {
        // only read loop to decrease cache invalidation by CMPXCHG
        // atomic swap does not execute unless lock value is observed as 0
        while( true ) {
            if( tryTake() )
                return;

            struct timespec now{};
            clock_gettime( CLOCK_MONOTONIC, &now );

            if( time_diff_ns( &now, &start_time) >= spin_time )
                break;
        }

        __sync_add_and_fetch( &waiter_count, 1 );
        simple_futex(&value, FUTEX_WAIT, 0 );
        __sync_sub_and_fetch( &waiter_count, 1 );
    }
}

void Semaphore::take( uint32_t timeout_ns ) {
    if( tryTake() )
        return;

    struct timespec start_time{}, now{};
    clock_gettime( CLOCK_MONOTONIC, &start_time );

    while( true ) {
        // only read loop to decrease cache invalidation by CMPXCHG
        // atomic swap does not execute unless lock value is observed as 0
        while( true ) {
            if( tryTake() )
                return;

            clock_gettime( CLOCK_MONOTONIC, &now );

            if( time_diff_ns( &now, &start_time) >= std::min( spin_time, timeout_ns ) )
                break;
        }

        long time_diff = time_diff_ns( &now, &start_time );

        if( time_diff >= timeout_ns )
            throw TimeoutExpiredException( "Timeout expired before take was possible." );

        // time remaining for sleep
        time_diff -= timeout_ns;

        // store back to now struct
        now.tv_sec = time_diff / 1000000;
        now.tv_nsec = ( time_diff % 1000000 ) * 1000;

        __sync_add_and_fetch( &waiter_count, 1 );
        simple_futex( &value, FUTEX_WAIT, 0, &now );
        __sync_sub_and_fetch( &waiter_count, 1 );

        // measure time if timeout expired
        clock_gettime( CLOCK_MONOTONIC, &now );
        if( time_diff_ns( &now, &start_time ) >= timeout_ns )
            throw TimeoutExpiredException( "Timeout expired before take was possible." );
    }
}

[[nodiscard]] bool Semaphore::tryTake() noexcept {
    while( true ) {
        uint32_t temp = value;
        if( temp > 0 ) {
            if( __sync_bool_compare_and_swap( &value, temp, temp - 1 ) )
                return true;
        }
        else return false;
    }
}

void Semaphore::give() noexcept {
    __sync_fetch_and_add( &value, 1 );
    if( waiter_count )
        simple_futex( &value, FUTEX_WAKE, 1 );

}


void Condition::wait( Lock &lock ) noexcept {
    uint32_t current = __sync_add_and_fetch( &waiters, 1 );

    lock.unlock();

    simple_futex( &waiters, FUTEX_WAIT, current );
    __sync_sub_and_fetch( &waiters, 1 );

    lock.lock();
}

void Condition::notify() noexcept {
    simple_futex( &waiters, FUTEX_WAKE, 1 );
}

void Condition::notify_all() noexcept {
    simple_futex( &waiters, FUTEX_WAKE, INT32_MAX );
}
