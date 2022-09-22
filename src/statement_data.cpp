#include "statement_data.h"

size_t taos::table_data::estimate_size(const taos::table_data::row &r) {
    if (r.empty()) {
        return 0;
    }

    size_t size = 2;
    size += r[0].size();
    for (size_t i = 1; i < r.size(); ++i) {
        size += 1;
        size += r[i].size();
    }
    return size;
}

taos::table_data::table_data(const std::string &name) : name(name) {}

size_t taos::table_data::estimate_size() const {
    if (data.empty()) {
        return 0;
    }
    size_t size = 0;

    size += name.size() + 1;
    size += VALUES.size() + 1;

    estimate_size(data[0]);
    for (size_t i = 1; i < data.size(); ++i) {
        size += 1;
        size += estimate_size(data[i]);
    }
    return size;
}

size_t taos::table_data::rows() const {
    return data.size();
}

void taos::table_data::write_string(std::string &s, const taos::table_data::row &r) {
    s += '(';
    s += r[0];
    for (size_t i = 1; i < r.size(); ++i) {
        s += ',';
        s += r[i];
    }
    s += ')';
}

void taos::table_data::write_string(std::string &s) const {
    if (data.empty()) {
        return;
    }

    s += name;
    s += ' ';

    s += VALUES;
    s += ' ';

    write_string(s, data[0]);
    for (size_t i = 1; i < data.size(); ++i) {
        s += ' ';
        write_string(s, data[i]);
    }
}

void taos::table_data::insert_into(const std::vector<std::string> &cols) {
    if (!cols.empty()) {
        data.emplace_back(cols);
    }
}

bool taos::table_data::empty() {
    return data.empty();
}

size_t taos::statement_data::estimate_size() const {
    if (empty()) {
        return 0;
    }

    size_t size = INSERT_INTO.size() + 1;
    for (auto &[table, data]: tables) {
        size += data->estimate_size();
        size += 1;
    }
    size -= 1;
    return size;
}

bool taos::statement_data::empty() const {
    if (tables.empty()) {
        return true;
    }

    return std::all_of(tables.begin(), tables.end(), [](auto &&data) {
        return data.second->empty();
    });
}

std::string taos::statement_data::to_string() const {
    if (tables.empty()) {
        return {};
    }

    std::string s;
    s.reserve(estimate_size());


    s += INSERT_INTO;
    s += ' ';
    bool is_first = true;
    for (auto &[table, data]: tables) {
        if (!is_first) {
            s += ' ';
        }
        data->write_string(s);
        is_first = false;
    }
    return s;
}

size_t taos::statement_data::rows() const {
    size_t r = 0;
    for (auto &&[name, table]: tables) {
        r += table->rows();
    }
    return r;
}

std::shared_ptr<taos::table_data> taos::statement_data::insert_into(const std::string &table) {
    auto iter = tables.find(table);
    if (iter != tables.end()) {
        return iter->second;
    }

    return tables[table] = std::make_shared<table_data>(table);
}
