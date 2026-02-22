#include "planner/operator/logical_call_subquery.h"

#include "planner/operator/factorization/sink_util.h"

namespace lbug {
namespace planner {

void LogicalCallSubquery::computeFactorizedSchema() {
    auto outerSchema = getOuterChild()->getSchema();
    auto innerSchema = getInnerChild()->getSchema();
    schema = outerSchema->copy();
    SinkOperatorUtil::mergeSchema(*innerSchema, innerSchema->getExpressionsInScope(), *schema);
}

void LogicalCallSubquery::computeFlatSchema() {
    auto outerSchema = getOuterChild()->getSchema();
    auto innerSchema = getInnerChild()->getSchema();
    schema = outerSchema->copy();
    auto innerGroupPos = schema->createGroup();
    for (auto& expression : innerSchema->getExpressionsInScope()) {
        schema->insertToGroupAndScope(expression, innerGroupPos);
    }
}

} // namespace planner
} // namespace lbug
