#pragma once
#include <cstdint>
#include <string>
#include <concepts>


namespace yarn {
    /**
     * @class TimeoutExpiredException, exception thrown from function that take timeout argument.
     */
    class TimeoutExpiredException: public std::exception {
    public:
        explicit TimeoutExpiredException(const char *);
        char *what();
    protected:
        std::string error_msg;
    };


    /**
     * @class Lock, simple owning synchronisation primitive for protecting shared resources.
     */
    class Lock {
    public:
        /**
         * Constructor of lock.
         * @param [spinlock_time_ms] Time lock tries to lock in spin before yielding CPU.
         */
        explicit Lock( uint32_t spinlock_time_ms ) noexcept;

        Lock() noexcept = default;

        Lock( const Lock & ) = delete;

        Lock &operator=( const Lock & ) = delete;

        /**
         * Acquires lock, blocks until lock is released by other thread.
         */
        void lock() noexcept;

        /**
         * Tries to lock, if lock isn't acquired by timeout, exception is raised.
         * @param timeout_ms
         */
        void lock( uint32_t timeout_ms );

        /**
         * Tries to lock a lock(non-blocking).
         * @returns true if lock was acquired
         * @note Can be used if thread can choose to do something else if lock is locked.
         */
        [[nodiscard]] bool tryLock() noexcept;

        /**
         * Unlocks lock.
         * @note There is no error checking for case when you unlock lock owned by other thread.
         * @note There is no error checking for double unlock or unlocking unlocked lock.
         */
        void unlock() noexcept;
    protected:
        uint32_t lock_value = 0;
        uint32_t waiter_count = 0;
        uint32_t spin_time = 4;
    };


    /**
     * @class Semaphore, simple counting semaphore with public value.
     */
    class Semaphore {
    public:
        /**
         * Constructor of Semaphore.
         * @param initial_value that semaphore holds.
         * @param spinlock_time_ms time that semaphore takes in spinlock before yielding CPU.
         */
        explicit Semaphore( uint32_t initial_value = 0, uint32_t spinlock_time_ms = 4 ) noexcept;

        Semaphore( const Semaphore & ) = delete;

        Semaphore &operator=( const Semaphore & ) = delete;

        /**
         * Decrements semaphore. If semaphore value is 0, blocks until other thread calls give.
         */
        void take() noexcept;

        /**
         * Same as Semaphore::take() but if take does not succeed after timeout, exception is raised.
         * @param timeout_ms
         */
        void take( uint32_t timeout_ms );

        /**
         * Tries, taking semaphore immediately.
         * @return true if semaphore was taken.
         */
        [[nodiscard]] bool tryTake() noexcept;

        /**
         * Increments semaphore value.
         */
        void give() noexcept;

        uint32_t value;
    protected:
        uint32_t spin_time;
        uint32_t waiter_count = 0;
    };

    /**
     * @class Condition, ideal for implementing monitor synchronisation.
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
         * Releases lock, and waits for notify or notify all call.
         * Lock is again acquired after return from wait.
         */
        void wait( Lock &lock ) noexcept;

        /**
         * Wakes up exactly one thread.
         */
        void notify() noexcept;

        /**
         * Wakes up all waiting threads.
         */
        void notify_all() noexcept;
    protected:
        uint32_t waiters = 0;
    };
}
