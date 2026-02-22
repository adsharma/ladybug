#include "planner/operator/logical_call_subquery.h"
#include "processor/operator/call_subquery.h"
#include "processor/plan_mapper.h"

using namespace lbug::planner;

namespace lbug {
namespace processor {

std::unique_ptr<PhysicalOperator> PlanMapper::mapCallSubquery(
    const LogicalOperator* logicalOperator) {
    auto& logicalCall = logicalOperator->constCast<LogicalCallSubquery>();
    auto outerChild = mapOperator(logicalCall.getOuterChild().get());
    auto innerChild = mapOperator(logicalCall.getInnerChild().get());
    auto printInfo = std::make_unique<OPPrintInfo>();
    return std::make_unique<CallSubquery>(std::move(outerChild), std::move(innerChild),
        getOperatorID(), std::move(printInfo));
}

} // namespace processor
} // namespace lbug
