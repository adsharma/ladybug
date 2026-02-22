#include "planner/operator/logical_call_subquery.h"

namespace lbug {
namespace planner {

void LogicalCallSubquery::computeFlatSchema() {
    copyChildSchema(1);
}

void LogicalCallSubquery::computeFactorizedSchema() {
    copyChildSchema(1);
}

} // namespace planner
} // namespace lbug
