#pragma once

#include "processor/operator/physical_operator.h"

namespace lbug {
namespace processor {

class LBUG_API CallSubquery final : public PhysicalOperator {
    static constexpr PhysicalOperatorType type_ = PhysicalOperatorType::CALL_SUBQUERY;

public:
    CallSubquery(std::unique_ptr<PhysicalOperator> outerChild,
        std::unique_ptr<PhysicalOperator> innerChild, physical_op_id id,
        std::unique_ptr<OPPrintInfo> printInfo)
        : PhysicalOperator{type_, std::move(outerChild), std::move(innerChild), id,
              std::move(printInfo)} {}

    bool getNextTuplesInternal(ExecutionContext* context) override;

    std::unique_ptr<PhysicalOperator> copy() override {
        return std::make_unique<CallSubquery>(getChild(0)->copy(), getChild(1)->copy(), id,
            printInfo ? printInfo->copy() : nullptr);
    }
};

} // namespace processor
} // namespace lbug
