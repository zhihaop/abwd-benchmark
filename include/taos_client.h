#include "taos.h"
#include "statement.h"
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace taos {
    
    /**
     * The result of taos::client::query.
     */
    class result {
        TAOS_RES *res{nullptr};
    public:
        explicit result(TAOS_RES *res) : res(res) {}

        result(const result &r) = delete;

        result(result &&r) noexcept;

        TAOS_RES *native_handler();
        
        /**
         * Get the error string.
         * 
         * @return the error string.
         */
        std::string error();
        
        /**
         * Get the status code.
         * 
         * @return  the status code. 
         */
        int code();

        ~result();
    };

    /**
     * The callback function of async query.
     */
    using callback = std::function<void(result)>;

    struct batch_policy {
        bool enable{true};
        bool thread_isolate{true};
        int batch_size{128};
        int timeout{100};
    };

    struct client_policy {
        batch_policy batch{};
        int max_sessions{256};
    };

    /**
     * The taosc cpp client.
     */
    class client {
        TAOS *conn{nullptr};
        client_policy policy{};

        std::atomic_int in_flight{0};
        std::mutex mu{};
        std::condition_variable cv{};
    public:
        explicit client(client_policy client_policy);

        client(const client &c) = delete;

        client(client &&c) noexcept;

        ~client();

        TAOS *native_handler();
        
        /**
         * Connect to database.
         * 
         * @param ip    the ip address.
         * @param user  the username.
         * @param pass  the password.
         * @param db    the database to use.
         * @param port  the port of taosd.
         */
        void connect(const char *ip, const char *user, const char *pass, const char *db, uint16_t port = 6030);
        
        /**
         * Query the sql string (sync).
         * 
         * @param s     the sql string.
         * @return      the query result.
         */
        taos::result query(const std::string &s);

        /**
         * Query the sql statement (sync).
         * 
         * @param s     the sql statement.
         * @return      the query result.
         */
        taos::result query(const taos::statement &s);

        /**
         * Query the sql string (async), the result will be returned using callback fn.
         * @param s     the sql string.
         * @param fn    the callback function.
         */
        void query(const std::string &s, callback fn);

        /**
         * Query the sql statement (async), the result will be returned using callback fn.
         * @param s     the sql statement.
         * @param fn    the callback function.
         */
        void query(const taos::statement &s, callback fn);
        
        /**
         * Close the taosc client.
         */
        void close();
    };
}