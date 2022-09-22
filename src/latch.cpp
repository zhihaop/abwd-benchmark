#include "latch.h"

void countdown_latch::countdown() {
    if (--counter == 0) {
        std::unique_lock<std::mutex> lock(mu);
        cv.notify_all();
    }
}

void countdown_latch::await() {
    std::unique_lock<std::mutex> lock(mu);
    cv.wait(lock, [this]() { return counter == 0; });
}

countdown_latch::countdown_latch(int counter) : counter(counter) {}
