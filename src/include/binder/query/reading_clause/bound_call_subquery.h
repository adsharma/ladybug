#pragma once

#include "binder/bound_statement_result.h"
#include "binder/query/normalized_single_query.h"
#include "binder/query/reading_clause/bound_reading_clause.h"

namespace lbug {
namespace binder {

class LBUG_API BoundCallSubquery : public BoundReadingClause {
    static constexpr common::ClauseType clauseType_ = common::ClauseType::CALL_SUBQUERY;

public:
    BoundCallSubquery(NormalizedSingleQuery innerQuery, expression_vector scopeExpressions)
        : BoundReadingClause{clauseType_}, innerQuery{std::move(innerQuery)},
          scopeExpressions{std::move(scopeExpressions)} {}

    const NormalizedSingleQuery& getInnerQuery() const { return innerQuery; }
    const expression_vector& getScopeExpressions() const { return scopeExpressions; }
    const BoundStatementResult* getInnerResult() const { return innerQuery.getStatementResult(); }

private:
    NormalizedSingleQuery innerQuery;
    expression_vector scopeExpressions;
};

} // namespace binder
} // namespace lbug
