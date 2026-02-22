#include "planner/operator/scan/logical_scope_scan.h"
#include "processor/operator/scope_scan.h"
#include "processor/plan_mapper.h"

using namespace lbug::planner;

namespace lbug {
namespace processor {

std::unique_ptr<PhysicalOperator> PlanMapper::mapScopeScan(
    const LogicalOperator* logicalOperator) {
    auto& logicalScopeScan = logicalOperator->constCast<LogicalScopeScan>();
    KU_ASSERT(logicalScopeScan.getNumChildren() == 1);
    auto child = mapOperator(logicalScopeScan.getChild(0).get());
    auto printInfo = std::make_unique<OPPrintInfo>();
    return std::make_unique<ScopeScan>(std::move(child), getOperatorID(), std::move(printInfo));
}

} // namespace processor
} // namespace lbug
