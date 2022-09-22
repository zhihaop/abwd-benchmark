#ifndef ASYNC_BULK_TEST_LATCH_H
#define ASYNC_BULK_TEST_LATCH_H

#include <atomic>
#include <mutex>
#include <condition_variable>

class countdown_latch {
    std::atomic_int counter;
    std::mutex mu{};
    std::condition_variable cv{};
public:
    explicit countdown_latch(int counter);

    void countdown();

    void await();
};
#endif //ASYNC_BULK_TEST_LATCH_H
