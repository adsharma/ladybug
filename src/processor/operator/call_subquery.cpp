#include "processor/operator/call_subquery.h"

#include "common/exception/runtime.h"
#include "processor/execution_context.h"

namespace lbug {
namespace processor {

bool CallSubquery::getNextTuplesInternal(ExecutionContext* /*context*/) {
    throw common::RuntimeException(
        "CALL (scope) { subquery } physical execution is not yet implemented.");
}

} // namespace processor
} // namespace lbug
