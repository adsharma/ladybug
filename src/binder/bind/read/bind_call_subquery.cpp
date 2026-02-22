#include "binder/binder.h"
#include "binder/query/reading_clause/bound_call_subquery.h"
#include "common/exception/binder.h"
#include "parser/query/reading_clause/call_subquery_clause.h"
#include <format>

using namespace lbug::common;
using namespace lbug::parser;

namespace lbug {
namespace binder {

std::unique_ptr<BoundReadingClause> Binder::bindCallSubquery(
    const ReadingClause& readingClause) {
    auto& call = readingClause.constCast<CallSubqueryClause>();
    const auto& innerQuery = call.getInnerQuery();

    if (innerQuery.getNumUpdatingClauses() > 0) {
        throw BinderException(
            "CALL subquery body must not contain updating clauses.");
    }

    expression_vector scopeExpressions;
    if (call.getImportAll()) {
        scopeExpressions = scope.getExpressions();
    } else {
        for (const auto& name : call.getScopeVariableNames()) {
            if (!scope.contains(name)) {
                throw BinderException(
                    std::format("Variable '{}' is not defined in outer scope.", name));
            }
            scopeExpressions.push_back(scope.getExpression(name));
        }
    }

    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> savedPairs;
    if (!call.getImportAll()) {
        savedPairs = scope.getExpressionPairs();
        scope.clear();
        for (size_t i = 0; i < call.getScopeVariableNames().size(); i++) {
            scope.addExpression(call.getScopeVariableNames()[i], scopeExpressions[i]);
        }
    }

    NormalizedSingleQuery boundInner = bindSingleQuery(innerQuery);

    if (!call.getImportAll()) {
        scope.clear();
        for (auto& [name, expr] : savedPairs) {
            scope.addExpression(name, expr);
        }
    }

    return std::make_unique<BoundCallSubquery>(
        std::move(boundInner), std::move(scopeExpressions));
}

} // namespace binder
} // namespace lbug
