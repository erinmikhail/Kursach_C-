#pragma once

#include "api/Logger.hpp"
#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include "core/QueryResult.hpp"
#include <string>

namespace core {

QueryResult run_query(const std::string& query,
                      catalog::Catalog& catalog,
                      Executor& executor,
                      api::Logger& logger);

std::string format_query_result(const QueryResult& result);

} // namespace core
