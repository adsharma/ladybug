#include "planner/operator/logical_call_subquery.h"
#include "processor/operator/call_subquery.h"
#include "processor/plan_mapper.h"
#include "processor/result/result_set_descriptor.h"

using namespace lbug::planner;

namespace lbug {
namespace processor {

std::unique_ptr<PhysicalOperator> PlanMapper::mapCallSubquery(
    const LogicalOperator* logicalOperator) {
    auto& logicalCall = logicalOperator->constCast<LogicalCallSubquery>();
    auto* outerSchema = logicalCall.getOuterChild()->getSchema();
    auto* innerSchema = logicalCall.getInnerChild()->getSchema();
    auto scopeOutPos =
        getDataPos(logicalCall.getScopeExpressions(), *outerSchema);
    std::vector<DataPos> innerOutputPositions;
    auto numOuterGroups = outerSchema->getNumGroups();
    for (auto& expr : innerSchema->getExpressionsInScope()) {
        auto [groupPos, valuePos] = innerSchema->getExpressionPos(*expr);
        innerOutputPositions.emplace_back(
            numOuterGroups + groupPos, valuePos);
    }
    auto innerDescriptor =
        std::make_unique<ResultSetDescriptor>(logicalCall.getInnerChild()->getSchema());
    CallSubqueryInfo info(std::move(scopeOutPos), std::move(innerOutputPositions), numOuterGroups,
        std::move(innerDescriptor));
    auto outerChild = mapOperator(logicalCall.getOuterChild().get());
    auto innerChild = mapOperator(logicalCall.getInnerChild().get());
    auto printInfo = std::make_unique<OPPrintInfo>();
    return std::make_unique<CallSubquery>(std::move(info), std::move(outerChild),
        std::move(innerChild), getOperatorID(), std::move(printInfo));
}

} // namespace processor
} // namespace lbug
