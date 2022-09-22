#include "statement.h"


size_t taos::statement::rows() const {
    return data->rows();
}

bool taos::statement::empty() const {
    return data->empty();
}

void taos::statement::clear() {
    data = std::make_shared<statement_data>();
}

std::string taos::statement::to_string() const {
    return data->to_string();
}

taos::insert_statement<taos::statement> taos::statement::insert_into(const std::string &table) {
    return insert_statement<statement>(data->insert_into(table), data);
}

taos::statement taos::statement::builder() {
    return {};
}

taos::statement::statement(std::shared_ptr<statement_data> data) : data(std::move(data)) {}
