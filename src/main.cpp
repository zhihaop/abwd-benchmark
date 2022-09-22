#include "taos_client.h"
#include "latch.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <chrono>
#include <string>
#include <future>
#include <utility>
#include <vector>

using namespace std;

struct benchmark_policy {
    int data_size = 1000000;
    int threads = static_cast<int>(thread::hardware_concurrency());
};

taos::client_policy read_policy(std::istream &in);
benchmark_policy read_benchmark_policy(std::istream &in);
void execute_benchmark(benchmark_policy b, taos::client_policy c);

int main() {
    auto b = read_benchmark_policy(cin);
    taos::client_policy c = read_policy(cin);
    execute_benchmark(b, c);
    return 0;
}

taos::client_policy read_policy(istream &in) {
    taos::client_policy policy;
    taos::batch_policy &batch = policy.batch;

    fmt::print("max_sessions ({}): ", policy.max_sessions);
    in >> policy.max_sessions;
    fmt::print("enable_batching ({}): ", static_cast<int>(batch.enable));
    in >> batch.enable;

    if (batch.enable) {
        fmt::print("enable_thread_local_batch ({}): ", static_cast<int>(batch.thread_isolate));
        in >> batch.thread_isolate;
        fmt::print("batch_size ({}): ", batch.batch_size);
        in >> batch.batch_size;
        fmt::print("timeout ({}): ", batch.timeout);
        in >> batch.timeout;
    }
    return policy;
}

benchmark_policy read_benchmark_policy(istream &in) {
    benchmark_policy t;

    fmt::print("data_size ({}): ", t.data_size);
    in >> t.data_size;

    fmt::print("threads ({}): ", t.threads);
    in >> t.threads;
    return t;
}

/**
 * Run the benchmark.
 * 
 * @param b    the benchmark_policy.
 * @param c    the client policy.
 */
void execute_benchmark(benchmark_policy b, taos::client_policy c) {
    auto client = taos::client(c);
    client.connect("127.0.0.1", nullptr, nullptr, "test", 6030);

    // prepare table.
    string table = "bw0001";
    client.query("create stable bulk_write (ts timestamp, v int) tags(g int)");
    client.query(fmt::format("create table {} using bulk_write tags({})", table, 0));
    
    
    // executor will block until finish all the insertion calls. 
    countdown_latch latch(b.data_size);
    std::vector<std::future<void>> executor;
    const auto block_size = b.data_size / b.threads;
    const long ts = chrono::system_clock::now().time_since_epoch() / 1ns - b.data_size;
    
    // start insert.
    auto t1 = chrono::steady_clock::now();
    for (int i = 0; i < b.threads; ++i) {
        const auto offset = i * block_size;
        const auto nxt = i + 1 == b.threads ? b.data_size : (i + 1) * block_size;
        
        auto runnable = [offset, nxt, ts, table, &client, &latch]() {
            for (auto j = offset; j < nxt; ++j) {
                auto sql = fmt::format("insert into {} values ({}, {})", table, ts + j, j);
                client.query(sql, [&latch, sql](taos::result result) {
                    // no need to free TAOS_RES*, because TAOS_RES* is managed by taos::result class.
                    if (result.error() != "success"s) {
                        spdlog::error("sql: {}, error: {}", sql, result.error());
                    }
                    latch.countdown();
                });
            }
        };
        
        auto future = std::async(std::launch::async, runnable);
        executor.emplace_back(std::move(future));
    }

    latch.await();
    auto t2 = chrono::steady_clock::now();

    fmt::print("insert {} data cast {} ms, with threads {}\n", b.data_size, (t2 - t1) / 1ms, b.threads);
}

