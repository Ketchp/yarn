#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
#include <list>
#include <linux/futex.h>
#include <functional>


/**
 * @brief Library namespace.
 *
 * Contains synchronisation primitives similar to pthread.
 * @todo Implement monitor, fairLock, fairSemaphore, fairMonitor.
 * @todo Implement threadPool
 */
namespace yarn {
    /**
     * Wrapper for simple functionalities of FUTEX sys-call.
     * @param uaddr Address of lock associated with block.
     * @param futex_op FUTEX operation.
     * @param val Operation dependent. (Expected value/number of wake-ups)
     * @param timeout Blocking timeout.
     * @return Status of call.
     */
    inline long
    _simple_futex( const uint32_t *uaddr,
                   int futex_op,
                   uint32_t val,
                   const struct timespec *timeout = nullptr );


    /**
     * @brief Exception thrown from function that take timeout argument.
     *
     * @note Exception is never raised before specified timeout but can be delayed unspecified time after expiring.
     */
    class TimeoutExpiredException: public std::exception {
    public:
        /**
         * Exception constructor.
         * @param [in] error_msg Error message specific to class throwing the exception.
         */
        explicit TimeoutExpiredException( const char *error_msg );
        /**
         * @return Exception c-string message.
         */
        [[nodiscard]] const char *what() const noexcept override;
    protected:
        const std::string error_msg; /**< Error message. */
    };


    /**
     * @brief Mechanism for maintaining mutual exclusivity.
     *
     * Simple owning synchronisation primitive for protecting shared resources.
     * @note
     * This Lock is same as pthreads mutex.
     * There is no benefit in using this implementation over pthreads library.
     * @par
     * You can specify time Lock tries to acquire lock in spin-loop
     * before yielding CPU to other process. Although it may seem counterintuitive to do spin-loop,
     * but on non congested Locks it is better to wait 1us for unlock,
     * than switching thread context twice
     * (as a rule of thumb context switch takes ~1.5ms + cache needs to be invalidated).
     * Spin-loop is read only to prevent cache invalidation by atomic test-and-set.
     */
    class Lock {
    public:
        /**
         * Constructor of lock.
         * @param [in] spinlock_time_us Time lock tries to lock in spin before yielding CPU.
         */
        explicit Lock( uint32_t spinlock_time_us = 4 ) noexcept;

        Lock( const Lock & ) = delete;

        Lock &operator=( const Lock & ) = delete;

        /**
         * Acquires lock, if necessary blocks until lock is released by other thread.
         */
        void lock() noexcept;

        /**
         * Tries to lock, if lock isn't acquired by timeout, exception is raised.
         * @param timeout_us
         * @throws yarn::TimeoutExpiredException
         */
        void lock( uint32_t timeout_us );

        /**
         * Tries to lock a lock(non-blocking).
         * @returns true if lock was acquired
         * @note Can be used if thread can choose to do something else if lock is locked.
         */
        [[nodiscard]] bool tryLock() noexcept;

        /**
         * Unlocks lock.
         * @warning There is no error checking for case when you unlock lock owned by other thread.
         * @warning There is no error checking for double unlock or unlocking unlocked lock.
         */
        void unlock() noexcept;
    protected:
        uint32_t lock_value = 0;   /**< Lock state, 1: locked, 0: unlocked. */
        uint32_t waiter_count = 0; /**< Number of waiters is recorded to prevent unnecessary futex sys-calls. */
        uint32_t spin_time;        /**< Time in ns, lock spins before yielding CPU. */
    };


    /**
     * @brief Simple counting unbounded semaphore with public value.
     *
     * Semaphore is counting mechanism used for thread synchronisation.
     * In simple terms semaphore is unsigned integral value with increment(\a give) and decrement(\a take) operations.
     * What makes semaphore superior to simple \a uint32_t is that operations are implicitly atomic and operation \a take
     * on semaphore will block calling thread. This blocking mechanism allows implementation of more sophisticated
     * synchronisation mechanisms as passing the baton.
     * @note Similar to posix semaphores. No benefit in using this implementation.
     * @note For taking semaphore when value = 0, same holds as for Lock; We first try spin-loop.
     */
    class Semaphore {
    public:
        /**
         * Constructor of Semaphore.
         * @param [in] initial_value
         * @param [in] spinlock_time_ns Time that semaphore takes in spinlock before yielding CPU.
         */
        explicit Semaphore( uint32_t initial_value = 0, uint32_t spinlock_time_ns = 4 ) noexcept;

        Semaphore( const Semaphore & ) = delete;

        Semaphore &operator=( const Semaphore & ) = delete;

        /**
         * Decrements semaphore. If semaphore value is 0, blocks until other thread calls give.
         */
        void take() noexcept;

        /**
         * Same as Semaphore::take() but if take does not succeed after timeout, exception is raised.
         * @param [in] timeout_ns
         * @throws yarn::TimeoutExpiredException
         */
        void take( uint32_t timeout_ns );

        /**
         * Tries, taking semaphore immediately.
         * @return true if semaphore was taken successfully.
         */
        [[nodiscard]] bool tryTake() noexcept;

        /**
         * Increments semaphore value.
         */
        void give() noexcept;

        /**
         * Value of semaphore.
         * @warning Be careful when accessing this value to avoid races.
         */
        uint32_t value;
    protected:
        uint32_t waiter_count = 0;  /**< Number of waiters to prevent unnecessary futex sys-calls */
        uint32_t spin_time;         /**< Time in ns spent in spin-loop before yielding CPU. */
    };

    /**
     * @brief Ideal for implementing monitor synchronisation.
     * @note No benefit in using yarn::Condition over pthread cond_t.
     * @note Use note, when checking or setting control variables that controls wake up of threads or signaling,
     * lock must be acquired to prevent race conditions.
     * @note Single lock can be used with multiple Conditions (to minimise unnecessary wake-ups).
     */
    class Condition {
    public:
        Condition() noexcept = default;

        Condition( const Condition & ) = delete;

        Condition &operator=( const Condition & ) = delete;

        /**
         * Releases lock, and waits for signal or signal_all call.
         * Lock is again acquired after return from wait.
         * @warning Implementation of wait, can cause spurious wake-ups,
         * so it is vital to re-check the condition after wake.
         */
        void wait( Lock &lock ) noexcept;

        /**
         * Wakes up exactly one thread.
         */
        void signal() noexcept;

        /**
         * Wakes up all waiting threads.
         */
        void signal_all() noexcept;
    protected:
        uint32_t waiters = 0;  /**< Number of waiters on this condition. */
    };


    /**
     * @brief Implementation of monitor similar to pythons monitor.
     *
     * Similar to condition variable used but checking of control conditions happens inside Monitor automatically.
     */
    class Monitor {
    public:
        Monitor() = default;

        Monitor( const Monitor & ) = delete;

        Monitor &operator=( const Monitor & ) = delete;

        /**
         * Acquires monitors lock.
         */
        void lock() noexcept;

        /**
         * Tries to acquire monitors lock.
         */
        [[nodiscard]] bool tryLock() noexcept;

        /**
         * Suspends caller until predicate evaluates to true.
         * All predicates are checked and if any evaluates to true
         * @tparam Callable_T Predicate type.
         * @param predicate Callable object that returns true when wait should end.
         */
        template <typename Callable_T>
        void wait_for( Callable_T predicate ) {
            static_assert( std::is_nothrow_invocable_r_v<bool, Callable_T>,
                    "Predicate must be callable with return type bool." );

            const auto &node = waiters.emplace_back( predicate, 0 );
            const auto node_it = --waiters.end();

            for( auto it = waiters.begin(); it != waiters.end(); ++it ) {
                if( !it->predicate() ) {
                    if( it == node_it )
                        return;
                    continue;
                }

                if( it == node_it )
                    silent_unlock();
                else {
                    it->lock = 1;
                    _simple_futex(&it->lock, FUTEX_WAKE, 1);
                }

                _simple_futex( &node.lock, FUTEX_WAIT, 0 );

                // we woke up, so we can erase current node
                waiters.erase( node_it );

                return;
            }
        }

        /**
         * Releases all waiters without checking predicates.
         */
        void signal_all() noexcept;

        /**
         * Reevaluates all predicates; if some predicate is true, it will gain lock and wake up.
         * If no predicate is true, lock is released.
         * @note This implementation assures that waiting threads are served before new accesses to monitor
         * in well defined order(if more waiting threads compete for lock; first will wake up thread that
         * went to sleep first).
         */
        void unlock() noexcept;

        /**
         * Releases lock without evaluating predicates; no waiting thread is woken up.
         * @note This allows only new accesses to monitor to happen.
         */
        void silent_unlock() noexcept;

    protected:
        /**
         * @brief Internal representation of waiter.
         *
         * Representation of waiter with predicate and lock determining wake-up.
         */
        struct LockNode {
            LockNode( const LockNode & ) = delete;
            LockNode &operator=( const LockNode & ) = delete;

            /**
             * Predicate evaluated for release.
             */
            std::function<bool()> predicate;

            /**
             * State of waiter; 0- waiter should wait, 1- waiter should be blocked.
             */
            uint32_t lock;
        };

    private:
        uint32_t monitor_lock = 0;
        uint32_t lock_waiters = 0;
        std::list<LockNode> waiters;
        uint32_t spin_time = 4; // us
    };
}
