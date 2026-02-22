#include "processor/operator/call_subquery.h"

#include "processor/execution_context.h"
#include "storage/buffer_manager/memory_manager.h"

namespace lbug {
namespace processor {

void CallSubquery::initGlobalState(ExecutionContext* context) {
    children[0]->initGlobalState(context);
    children[1]->initGlobalState(context);
    initGlobalStateInternal(context);
}

void CallSubquery::initLocalState(ResultSet* resultSet_, ExecutionContext* context) {
    resultSet = resultSet_;
    needNewOuterRow = true;
    hasOutputForCurrentOuter = false;
    registerProfilingMetrics(context->profiler);
    children[0]->initLocalState(resultSet, context);
    if (info.innerResultSetDescriptor) {
        innerResultSet = std::make_unique<ResultSet>(info.innerResultSetDescriptor.get(),
            storage::MemoryManager::Get(*context->clientContext));
    }
    if (innerResultSet) {
        children[1]->initLocalState(innerResultSet.get(), context);
    }
    initLocalStateInternal(resultSet_, context);
}

void CallSubquery::finalize(ExecutionContext* context) {
    children[0]->finalize(context);
    children[1]->finalize(context);
    finalizeInternal(context);
}

void CallSubquery::copyScopeToInnerResultSet(ExecutionContext* /*context*/) {
    if (info.scopeOuterPositions.empty() || !innerResultSet) {
        return;
    }
    for (size_t i = 0; i < info.scopeOuterPositions.size(); ++i) {
        auto& srcPos = info.scopeOuterPositions[i];
        auto srcChunk = resultSet->getDataChunk(srcPos.dataChunkPos);
        auto srcRowIdx = srcChunk->state->getSelVector()[0];
        auto* srcVec = resultSet->getValueVector(srcPos).get();
        auto* dstVec = innerResultSet->getValueVector(DataPos(0, i)).get();
        dstVec->copyFromVectorData(0, srcVec, srcRowIdx);
    }
    auto scopeChunk = innerResultSet->getDataChunk(0);
    scopeChunk->state->getSelVectorUnsafe().setToUnfiltered(1);
    scopeChunk->state->getSelVectorUnsafe().setSelSize(1);
}

void CallSubquery::mergeInnerResultsToResultSet(ExecutionContext* /*context*/,
    uint64_t numInnerRows) {
    const uint64_t numOutputRows = numInnerRows > 0 ? numInnerRows : 1;
    for (uint32_t group = 0; group < info.numOuterGroups; ++group) {
        auto chunk = resultSet->getDataChunk(group);
        for (uint32_t v = 0; v < chunk->getNumValueVectors(); ++v) {
            auto* vec = &chunk->getValueVectorMutable(v);
            for (uint64_t r = 1; r < numOutputRows; ++r) {
                vec->copyFromVectorData(r, vec, 0);
            }
        }
        chunk->state->getSelVectorUnsafe().setToUnfiltered(numOutputRows);
        chunk->state->getSelVectorUnsafe().setSelSize(numOutputRows);
    }
    if (numInnerRows > 0 && innerResultSet) {
        const data_chunk_pos_t scopeChunkOffset = info.scopeOuterPositions.empty() ? 0 : 1;
        for (size_t i = 0; i < info.innerOutputPositions.size(); ++i) {
            auto& outPos = info.innerOutputPositions[i];
            auto* dstVec = resultSet->getValueVector(outPos).get();
            DataPos innerPos(outPos.dataChunkPos - info.numOuterGroups + scopeChunkOffset,
                outPos.valueVectorPos);
            auto* srcVec = innerResultSet->getValueVector(innerPos).get();
            auto srcChunk = innerResultSet->getDataChunk(innerPos.dataChunkPos);
            for (uint64_t r = 0; r < numInnerRows; ++r) {
                auto srcRowIdx = srcChunk->state->getSelVector()[r];
                dstVec->copyFromVectorData(r, srcVec, srcRowIdx);
            }
            auto dstChunk = resultSet->getDataChunk(outPos.dataChunkPos);
            dstChunk->state->getSelVectorUnsafe().setToUnfiltered(numInnerRows);
            dstChunk->state->getSelVectorUnsafe().setSelSize(numInnerRows);
        }
    } else if (!info.innerOutputPositions.empty()) {
        for (auto& outPos : info.innerOutputPositions) {
            auto* dstVec = resultSet->getValueVector(outPos).get();
            dstVec->setNull(0, true);
            auto dstChunk = resultSet->getDataChunk(outPos.dataChunkPos);
            dstChunk->state->getSelVectorUnsafe().setToUnfiltered(1);
            dstChunk->state->getSelVectorUnsafe().setSelSize(1);
        }
    }
    metrics->numOutputTuple.increase(numOutputRows);
}

bool CallSubquery::getNextTuplesInternal(ExecutionContext* context) {
    while (true) {
        if (needNewOuterRow) {
            if (!children[0]->getNextTuple(context)) {
                return false;
            }
            copyScopeToInnerResultSet(context);
            if (innerResultSet) {
                children[1]->initLocalState(innerResultSet.get(), context);
            }
            needNewOuterRow = false;
            hasOutputForCurrentOuter = false;
        }
        bool gotInnerBatch = innerResultSet && children[1]->getNextTuple(context);
        uint64_t numInnerRows = 0;
        if (gotInnerBatch && !info.innerOutputPositions.empty()) {
            const data_chunk_pos_t innerChunkIdx =
                info.scopeOuterPositions.empty() ? 0 : 1;
            auto chunk = innerResultSet->getDataChunk(innerChunkIdx);
            numInnerRows = chunk->state->getSelVector().getSelSize();
        } else if (gotInnerBatch) {
            numInnerRows = innerResultSet->getDataChunk(0)->state->getSelVector().getSelSize();
        }
        if (numInnerRows > 0) {
            hasOutputForCurrentOuter = true;
            mergeInnerResultsToResultSet(context, numInnerRows);
            return true;
        }
        if (!hasOutputForCurrentOuter) {
            hasOutputForCurrentOuter = true;
            mergeInnerResultsToResultSet(context, 0);
            return true;
        }
        needNewOuterRow = true;
    }
}

} // namespace processor
} // namespace lbug
