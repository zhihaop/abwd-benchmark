#ifndef ASYNC_BULK_BENCHMARK_STATEMENT_DATA_H
#define ASYNC_BULK_BENCHMARK_STATEMENT_DATA_H

#include <vector>
#include <memory>
#include <string>
#include <algorithm>

namespace taos {

    /**
     * Store the data of the table.
     */
    class table_data {
    public:
        using row = std::vector<std::string>;

    public:
        explicit table_data(const std::string &name);

        static size_t estimate_size(const row &r);

        size_t estimate_size() const;

        size_t rows() const;

        static void write_string(std::string &s, const row &r);

        void write_string(std::string &s) const;

        bool empty();

        void insert_into(const std::vector<std::string> &cols);

    private:
        constexpr const static std::string_view VALUES{"values"};

    protected:
        const std::string &name;
        std::vector<row> data;
    };

    /**
     * Store the data of the statement.
     */
    class statement_data {
    public:
        size_t estimate_size() const;

        bool empty() const;

        std::string to_string() const;

        size_t rows() const;

        std::shared_ptr<table_data> insert_into(const std::string &table);

    private:
        constexpr const static std::string_view INSERT_INTO{"insert into"};
        std::unordered_map<std::string, std::shared_ptr<table_data>> tables;
    };

}


#endif //ASYNC_BULK_BENCHMARK_STATEMENT_DATA_H
