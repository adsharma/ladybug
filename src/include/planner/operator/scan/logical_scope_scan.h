#pragma once

#include "planner/operator/logical_operator.h"

namespace lbug {
namespace planner {

class LogicalScopeScan final : public LogicalOperator {
public:
    explicit LogicalScopeScan(binder::expression_vector scopeExpressions)
        : LogicalOperator{LogicalOperatorType::SCOPE_SCAN},
          scopeExpressions{std::move(scopeExpressions)} {}

    void computeFactorizedSchema() override;
    void computeFlatSchema() override;

    std::string getExpressionsForPrinting() const override { return "SCOPE_SCAN"; }

    const binder::expression_vector& getScopeExpressions() const { return scopeExpressions; }

    std::unique_ptr<LogicalOperator> copy() override {
        return std::make_unique<LogicalScopeScan>(
            binder::expression_vector{scopeExpressions});
    }

private:
    binder::expression_vector scopeExpressions;
};

} // namespace planner
} // namespace lbug
