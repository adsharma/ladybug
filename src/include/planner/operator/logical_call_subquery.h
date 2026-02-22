#pragma once

#include "binder/expression/expression.h"
#include "planner/operator/logical_operator.h"

namespace lbug {
namespace planner {

class LBUG_API LogicalCallSubquery final : public LogicalOperator {
    static constexpr LogicalOperatorType operatorType_ = LogicalOperatorType::CALL_SUBQUERY;

public:
    LogicalCallSubquery(std::shared_ptr<LogicalOperator> outerChild,
        std::shared_ptr<LogicalOperator> innerPlan, binder::expression_vector scopeExpressions)
        : LogicalOperator{operatorType_,
              logical_op_vector_t{std::move(outerChild), std::move(innerPlan)}},
          scopeExpressions{std::move(scopeExpressions)} {}

    std::shared_ptr<LogicalOperator> getOuterChild() const { return getChild(0); }
    std::shared_ptr<LogicalOperator> getInnerChild() const { return getChild(1); }
    const binder::expression_vector& getScopeExpressions() const { return scopeExpressions; }

    void computeFlatSchema() override;
    void computeFactorizedSchema() override;

    std::string getExpressionsForPrinting() const override { return "CALL SUBQUERY"; }

    std::unique_ptr<LogicalOperator> copy() override {
        return std::make_unique<LogicalCallSubquery>(getChild(0)->copy(), getChild(1)->copy(),
            binder::expression_vector{scopeExpressions});
    }

private:
    binder::expression_vector scopeExpressions;
};

} // namespace planner
} // namespace lbug
