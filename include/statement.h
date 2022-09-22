#ifndef ASYNC_BULK_TEST_STATEMENT_H
#define ASYNC_BULK_TEST_STATEMENT_H

#include "statement_data.h"

#include <utility>
#include <vector>
#include <string>

#include <spdlog/spdlog.h>
#include <fmt/core.h>

namespace taos {

    /**
     * Convert ...args to std::string, and write them to iter.
     * 
     * @param iter     the iter to write.
     * @param first    the first arg.
     * @param args     the following args.
     */
    template<class Iterator, class First, class ...Args>
    void do_convert(Iterator iter, const First &first, const Args &...args) {
        *iter = fmt::to_string(first);
        if constexpr (sizeof...(args)) {
            do_convert(++iter, args...);
        }
    }

    /**
     * Convert ...args to std::vector<std::string>.
     * 
     * @tparam Args     the arg types.
     * @param args      the args.
     * @return          the std::vector<std::string>.
     */
    template<class ...Args>
    std::vector<std::string> string_values(const Args &...args) {
        if constexpr (!sizeof...(args)) {
            return {};
        }

        std::vector<std::string> v(sizeof...(args));
        do_convert(v.begin(), args...);
        return v;
    }
    
    /**
     * Represent an insert statement.
     * 
     * @tparam Parent the parent statement.
     */
    template<class Parent>
    class insert_statement : public Parent {
        std::shared_ptr<table_data> table;
    public:
        explicit insert_statement(std::shared_ptr<table_data> table, std::shared_ptr<statement_data> data) :
                table(std::move(table)), Parent(std::move(data)) {}
        
        template<class ...Args>
        insert_statement &value(const Args &...args) {
            if constexpr (sizeof...(args)) {
                table->insert_into(string_values(args...));
            }
            return *this;
        }
    };
    
    /**
     * Represent a statement.
     */
    class statement {

    protected:
        explicit statement(std::shared_ptr<statement_data> data);

    public:
        statement() = default;

        static statement builder();
        
        insert_statement<statement> insert_into(const std::string &table);
        
        std::string to_string() const;

        void clear();

        bool empty() const;

        size_t rows() const;

    protected:
        std::shared_ptr<statement_data> data = std::make_shared<statement_data>();
    };

}

#endif //ASYNC_BULK_TEST_STATEMENT_H
