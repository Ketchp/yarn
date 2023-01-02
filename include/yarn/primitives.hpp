#pragma once
#include <cstdint>


class Lock {
    uint32_t lock_value = 0;
    uint32_t waiter_count = 0;
public:
    void lock() noexcept;
    [[nodiscard]] bool tryLock() noexcept;
    void unlock();
};


class Semaphore {
public:
    uint32_t value;
    void take() noexcept;
    void take( uint32_t val = 1 ) noexcept;
    void give( uint32_t val = 1 ) noexcept;
private:
    [[nodiscard]] bool _tryTake() noexcept;
};
