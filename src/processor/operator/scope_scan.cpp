#include "processor/operator/scope_scan.h"

#include "common/data_chunk/data_chunk_state.h"
#include "processor/execution_context.h"

namespace lbug {
namespace processor {

void ScopeScan::initLocalStateInternal(ResultSet* /*resultSet*/, ExecutionContext* /*context*/) {
    returnedOneRow = false;
}

bool ScopeScan::getNextTuplesInternal(ExecutionContext* /*context*/) {
    if (returnedOneRow) {
        return false;
    }
    auto chunk = resultSet->getDataChunk(0);
    chunk->state->getSelVectorUnsafe().setToUnfiltered(1);
    chunk->state->getSelVectorUnsafe().setSelSize(1);
    returnedOneRow = true;
    metrics->numOutputTuple.increase(1);
    return true;
}

} // namespace processor
} // namespace lbug
