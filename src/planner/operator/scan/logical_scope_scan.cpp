#include "planner/operator/scan/logical_scope_scan.h"

namespace lbug {
namespace planner {

void LogicalScopeScan::computeFactorizedSchema() {
    createEmptySchema();
    auto groupPos = schema->createGroup();
    schema->setGroupAsSingleState(groupPos);
    for (auto& expr : scopeExpressions) {
        schema->insertToGroupAndScope(expr, groupPos);
    }
}

void LogicalScopeScan::computeFlatSchema() {
    computeFactorizedSchema();
}

} // namespace planner
} // namespace lbug
