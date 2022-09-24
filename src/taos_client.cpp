
#include "taos_client.h"
#include <utility>

extern "C" {
struct SDispatcherHolder;

extern SDispatcherHolder *tscDispatcherManager;
SDispatcherHolder *createDispatcherManager(int32_t batchSize, int32_t timeoutMs, bool isThreadLocal);
void destroyDispatcherManager(SDispatcherHolder *holder);
}

struct callback_context {
    taos::callback callback;
    taos::client *client;

    explicit callback_context(taos::callback callback, taos::client *client) : callback(std::move(callback)),
                                                                               client(client) {}
};

taos::client::client(taos::client_policy client_policy) : policy(client_policy) {
    const batch_policy& batch = policy.batch;
    if (batch.enable) {
        policy.max_sessions = std::max(1000 / batch.timeout, policy.max_sessions * batch.batch_size);
    }
}

inline static void configGlobalDispatcher(const taos::batch_policy& policy) {
    if (tscDispatcherManager) {
        destroyDispatcherManager(tscDispatcherManager);
        tscDispatcherManager = nullptr;
    }
    if (policy.enable) {
        tscDispatcherManager = createDispatcherManager(policy.batch_size, policy.timeout, policy.thread_isolate);
    }
}

void taos::client::connect(const char *ip, const char *user, const char *pass, const char *db, uint16_t port) {
    using namespace std::string_literals;
    conn = taos_connect(ip, user, pass, db, port);
    if (conn) {
        configGlobalDispatcher(policy.batch);
    }
}

taos::result taos::client::query(const std::string &s) {
    return taos::result{taos_query(conn, s.c_str())};
}

void taos::client::query(const std::string &s, taos::callback fn) {
    // create a callback context.
    auto context = new callback_context(std::move(fn), this);
    
    // max_sessions control.
    if (++in_flight >= policy.max_sessions) {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock, [this]() {
            return in_flight < policy.max_sessions;
        });
    }
    
    // do actual query.
    taos_query_a(conn, s.c_str(), [](void *arg, TAOS_RES *res, int code) {
        auto context = (callback_context *) arg;
        auto client = context->client;

        context->callback(taos::result{res});
        delete context;
        
        if (--client->in_flight <= client->policy.max_sessions) {
            std::unique_lock<std::mutex> lock(client->mu);
            client->cv.notify_all();
        }
    }, context);
}

taos::client::client(taos::client &&c) noexcept: conn(c.conn) {
    c.conn = nullptr;
}

taos::client::~client() {
    close();
}

void taos::client::close() {
    if (!conn) {
        return;
    }
    if (in_flight) {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock, [this]() {
            return in_flight == 0;
        });
    }
    taos_close(conn);
    conn = nullptr;
}

taos::result taos::client::query(const taos::statement &s) {
    return query(s.to_string());
}

void taos::client::query(const taos::statement &s, taos::callback fn) {
    return query(s.to_string(), std::move(fn));
}

TAOS *taos::client::native_handler() {
    return conn;
}

taos::result::~result() {
    if (res) {
        taos_free_result(res);
    }
}

taos::result::result(taos::result &&r) noexcept: res(r.res) {
    r.res = nullptr;
}

std::string taos::result::error() {
    return res ? taos_errstr(res) : std::string{};
}

int taos::result::code() {
    return res ? taos_errno(res) : 0;
}

TAOS_RES *taos::result::native_handler() {
    return res;
}
