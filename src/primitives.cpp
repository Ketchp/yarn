#include "primitives.hpp"

#ifdef YARN_DEBUG
#include <cassert>
#endif


#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <cstdlib>

#define MAX_SPIN_NUMBER UINT32_MAX

#ifdef YARN_SINGLE_CPU
// spinlock makes no sense on single core
#define MAX_SPIN_NUMBER 1
#endif

static inline int
futex( const uint32_t *uaddr,
       int futex_op,
       uint32_t val,
       const struct timespec *timeout,
       const uint32_t *uaddr2,
       uint32_t val3 ) {
    return syscall( SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3 );
}


void Lock::lock() noexcept {
    while( true ) {
        // only read loop to decrease cache invalidation by CMPXCHG
        // atomic swap does not execute unless lock value is observed as 0
        for( uint32_t i = 0; i < MAX_SPIN_NUMBER; ++i )
            if( lock_value == 0 && tryLock() )
                return;

        __sync_add_and_fetch( &waiter_count, 1 );
        futex(&lock_value, FUTEX_WAIT, 1, NULL, NULL, 0 );
        __sync_sub_and_fetch( &waiter_count, 1 );
    }
}

[[nodiscard]] inline bool Lock::tryLock() noexcept {
    return __sync_bool_compare_and_swap( &lock_value, 0, 1 );
}

void Lock::unlock() {
#ifdef YARN_DEBUG
    assert( lock_value == 1 );
#endif
    lock_value = 0;
    if( waiter_count )
        futex( &lock_value, FUTEX_WAKE, 1, NULL, NULL, 0 );
}

void Semaphore::take() noexcept {
    while( true ) {
        for( uint32_t i = 0; i < MAX_SPIN_NUMBER; ++i )
            if( _tryTake() )
                return;

        futex(&value, FUTEX_WAIT, 0, NULL, NULL, 0 );
    }
}

void Semaphore::take( uint32_t val ) noexcept {

}

void Semaphore::give( uint32_t val ) noexcept {

}

[[nodiscard]] bool Semaphore::_tryTake() noexcept {
    uint32_t temp = value;
    return temp > 0 && __sync_bool_compare_and_swap( &value, temp, temp - 1 );
}
