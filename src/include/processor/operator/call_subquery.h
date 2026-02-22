#pragma once

#include "common/copy_constructors.h"
#include "processor/operator/physical_operator.h"
#include "processor/result/result_set.h"
#include "processor/result/result_set_descriptor.h"

namespace lbug {
namespace processor {

struct CallSubqueryInfo {
    std::vector<DataPos> scopeOuterPositions;
    std::vector<DataPos> innerOutputPositions;
    uint32_t numOuterGroups = 0;
    std::unique_ptr<ResultSetDescriptor> innerResultSetDescriptor;

    CallSubqueryInfo() = default;
    CallSubqueryInfo(std::vector<DataPos> scopeOuterPositions,
        std::vector<DataPos> innerOutputPositions, uint32_t numOuterGroups,
        std::unique_ptr<ResultSetDescriptor> innerResultSetDescriptor)
        : scopeOuterPositions{std::move(scopeOuterPositions)},
          innerOutputPositions{std::move(innerOutputPositions)},
          numOuterGroups{numOuterGroups},
          innerResultSetDescriptor{std::move(innerResultSetDescriptor)} {}
    EXPLICIT_COPY_DEFAULT_MOVE(CallSubqueryInfo);

private:
    CallSubqueryInfo(const CallSubqueryInfo& other)
        : scopeOuterPositions{other.scopeOuterPositions},
          innerOutputPositions{other.innerOutputPositions},
          numOuterGroups{other.numOuterGroups},
          innerResultSetDescriptor{other.innerResultSetDescriptor ? other.innerResultSetDescriptor->copy() : nullptr} {}
};

class LBUG_API CallSubquery final : public PhysicalOperator {
    static constexpr PhysicalOperatorType type_ = PhysicalOperatorType::CALL_SUBQUERY;

public:
    CallSubquery(CallSubqueryInfo info, std::unique_ptr<PhysicalOperator> outerChild,
        std::unique_ptr<PhysicalOperator> innerChild, physical_op_id id,
        std::unique_ptr<OPPrintInfo> printInfo)
        : PhysicalOperator{type_, std::move(outerChild), std::move(innerChild), id,
              std::move(printInfo)},
          info{std::move(info)} {}

    void initGlobalState(ExecutionContext* context) override;
    void initLocalState(ResultSet* resultSet, ExecutionContext* context) override;
    void finalize(ExecutionContext* context) override;

    bool getNextTuplesInternal(ExecutionContext* context) override;

    std::unique_ptr<PhysicalOperator> copy() override {
        return std::make_unique<CallSubquery>(info.copy(), getChild(0)->copy(), getChild(1)->copy(),
            id, printInfo ? printInfo->copy() : nullptr);
    }

private:
    void copyScopeToInnerResultSet(ExecutionContext* context);
    void mergeInnerResultsToResultSet(ExecutionContext* context, uint64_t numInnerRows);

    CallSubqueryInfo info;
    std::unique_ptr<ResultSet> innerResultSet;
    bool needNewOuterRow = true;
    bool hasOutputForCurrentOuter = false;
};

} // namespace processor
} // namespace lbug
