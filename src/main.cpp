#include "taos_client.h"
#include "latch.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <future>
#include <utility>
#include <vector>

struct benchmark_policy {
    int data_size = 1000000;
    int threads = static_cast<int>(std::thread::hardware_concurrency());
};

taos::client_policy read_policy(std::istream &in);

benchmark_policy read_benchmark_policy(std::istream &in);

void execute_benchmark(benchmark_policy b, taos::client_policy c);

int main() {
    auto b = read_benchmark_policy(std::cin);
    taos::client_policy c = read_policy(std::cin);
    execute_benchmark(b, c);
}

/**
 * Run the benchmark.
 * 
 * @param b    the benchmark_policy.
 * @param c    the client policy.
 */
void execute_benchmark(benchmark_policy b, taos::client_policy c) {
    using namespace std::chrono_literals;
    using namespace std::string_literals;

    auto client = taos::client(c);
    client.connect("127.0.0.1", nullptr, nullptr, nullptr, 6030);

    // create database.
    client.query("create database abwd_test precision 'ns'");
    client.query("use abwd_test");

    // prepare table.
    std::string table = "bw0001";
    client.query("create stable bw (ts timestamp, v int) tags(g int)");
    client.query(fmt::format("create table {} using bw tags({})", table, 0));
    spdlog::info("create table: {}", table);

    // executor will block until finish all the insertion calls. 
    countdown_latch latch(b.data_size);
    std::vector<std::future<void>> executor;
    const auto block_size = b.data_size / b.threads;
    const long ts = std::chrono::system_clock::now().time_since_epoch() / 1ns - b.data_size;
    spdlog::info("start insert {} rows into {} table {} threads", b.data_size, table, b.threads);

    // start insert.
    auto t1 = std::chrono::steady_clock::now();
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
            latch.await();
        };
        auto future = std::async(std::launch::async, runnable);
        executor.emplace_back(std::move(future));
    }

    latch.await();
    auto t2 = std::chrono::steady_clock::now();

    auto millis = (t2 - t1) / 1ms;
    spdlog::info("insert {} data cast {} ms, with threads {}", b.data_size, millis, b.threads);
    spdlog::info("insert throughput: {} ops", b.data_size * 1000.0 / static_cast<double>(millis));
}

/**
 * Read a line from in and convert to T. 
 * If the line is empty, item will not be writen.
 * 
 * @tparam T    the item type.
 * @param in    the std::istream.
 * @param item  the item to read.
 */
template<typename T>
void readline(std::istream &in, T &item) {
    std::string s;
    std::getline(in, s);
    if (s.empty()) {
        spdlog::info("use default value: {}", item);
        return;
    }
    std::stringstream sin(s);
    sin >> item;
    spdlog::info("set value: {}", item);
}


taos::client_policy read_policy(std::istream &in) {
    taos::client_policy policy;
    taos::batch_policy &batch = policy.batch;

    fmt::print("max_sessions ({}): ", policy.max_sessions);
    readline(in, policy.max_sessions);
    fmt::print("enable_batching ({}): ", static_cast<int>(batch.enable));
    readline(in, batch.enable);

    if (batch.enable) {
        fmt::print("enable_thread_local_batch ({}): ", static_cast<int>(batch.thread_isolate));
        readline(in, batch.thread_isolate);
        fmt::print("batch_size ({}): ", batch.batch_size);
        readline(in, batch.batch_size);
        fmt::print("timeout ({}): ", batch.timeout);
        readline(in, batch.timeout);
    }
    return policy;
}

benchmark_policy read_benchmark_policy(std::istream &in) {
    benchmark_policy t;

    fmt::print("data_size ({}): ", t.data_size);
    readline(in, t.data_size);

    fmt::print("threads ({}): ", t.threads);
    readline(in, t.threads);
    return t;
}

