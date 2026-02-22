#include "binder/binder_scope.h"

namespace lbug {
namespace binder {

std::vector<std::pair<std::string, std::shared_ptr<Expression>>>
BinderScope::getExpressionPairs() const {
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> result;
    for (common::idx_t i = 0; i < expressions.size(); i++) {
        for (auto& [name, idx] : nameToExprIdx) {
            if (idx == i) {
                result.emplace_back(name, expressions[i]);
                break;
            }
        }
    }
    return result;
}

void BinderScope::addExpression(const std::string& varName,
    std::shared_ptr<Expression> expression) {
    nameToExprIdx.insert({varName, expressions.size()});
    expressions.push_back(std::move(expression));
}

void BinderScope::replaceExpression(const std::string& oldName, const std::string& newName,
    std::shared_ptr<Expression> expression) {
    KU_ASSERT(nameToExprIdx.contains(oldName));
    auto idx = nameToExprIdx.at(oldName);
    expressions[idx] = std::move(expression);
    nameToExprIdx.erase(oldName);
    nameToExprIdx.insert({newName, idx});
}

void BinderScope::clear() {
    expressions.clear();
    nameToExprIdx.clear();
}

} // namespace binder
} // namespace lbug
