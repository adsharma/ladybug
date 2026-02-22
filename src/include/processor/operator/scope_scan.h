#pragma once

#include "processor/operator/physical_operator.h"

namespace lbug {
namespace processor {

class ScopeScan final : public PhysicalOperator {
    static constexpr PhysicalOperatorType type_ = PhysicalOperatorType::SCOPE_SCAN;

public:
    explicit ScopeScan(std::unique_ptr<PhysicalOperator> child, physical_op_id id,
        std::unique_ptr<OPPrintInfo> printInfo)
        : PhysicalOperator{type_, std::move(child), id, std::move(printInfo)} {}

    void initLocalStateInternal(ResultSet* resultSet, ExecutionContext* context) override;

    bool getNextTuplesInternal(ExecutionContext* context) override;

    std::unique_ptr<PhysicalOperator> copy() override {
        return std::make_unique<ScopeScan>(getChild(0)->copy(), id,
            printInfo ? printInfo->copy() : nullptr);
    }

private:
    bool returnedOneRow = false;
};

} // namespace processor
} // namespace lbug
