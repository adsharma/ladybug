#pragma once

#include "parser/query/reading_clause/reading_clause.h"
#include "parser/query/single_query.h"

namespace lbug {
namespace parser {

class CallSubqueryClause final : public ReadingClause {
    static constexpr common::ClauseType clauseType_ = common::ClauseType::CALL_SUBQUERY;

public:
    CallSubqueryClause(bool importAll, std::vector<std::string> scopeVariableNames,
        SingleQuery innerQuery)
        : ReadingClause{clauseType_}, importAll{importAll},
          scopeVariableNames{std::move(scopeVariableNames)}, innerQuery{std::move(innerQuery)} {}

    bool getImportAll() const { return importAll; }
    const std::vector<std::string>& getScopeVariableNames() const { return scopeVariableNames; }
    const SingleQuery& getInnerQuery() const { return innerQuery; }

private:
    bool importAll;
    std::vector<std::string> scopeVariableNames;
    SingleQuery innerQuery;
};

} // namespace parser
} // namespace lbug
