#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {ResourceCategory::Book,
                                           ResourceCategory::ElectronicResource,
                                           ResourceCategory::Journal,
                                           ResourceCategory::EBook,
                                           ResourceCategory::AudioBook,
                                           ResourceCategory::Thesis};
}

Budget::Budget(Money total) : total_(total) {
    if (total_.isNegative()) throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative())
        throw std::invalid_argument("quota limits must not be negative");
    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

void Budget::setTitleLimit(ResourceCategory c, int maxTitles) {
    if (maxTitles < 0) {
        throw std::invalid_argument("title limit must not be negative");
    }
    titleLimits_[c] = maxTitles;
}

void Budget::removeTitleLimit(ResourceCategory c) {
    titleLimits_.erase(c);
}

std::optional<int> Budget::titleLimitFor(ResourceCategory c) const {
    auto it = titleLimits_.find(c);
    if (it == titleLimits_.end()) return std::nullopt;
    return it->second;
}

int Budget::distinctTitlesUsed(ResourceCategory c) const {
    auto it = titlesUsed_.find(c);
    return it == titlesUsed_.end() ? 0 : static_cast<int>(it->second.size());
}

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);
    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxSpend - usageFor(c).spent;
}

Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                 const std::string& title,
                                 std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }
    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }
    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) + " unit quota exceeded: requested " +
              std::to_string(units) + ", " + std::to_string(*left) + " remaining";
        return Failure::Quota;
    }
    if (!title.empty()) {
        auto limit = titleLimitFor(c);
        auto used = titlesUsed_.find(c);
        const bool alreadyCounted =
            used != titlesUsed_.end() && used->second.count(title) != 0;

        if (limit && !alreadyCounted && distinctTitlesUsed(c) >= *limit) {
            why = std::string(categoryName(c)) +
                  " distinct-title limit exceeded: limit " +
                  std::to_string(*limit);
            return Failure::Quota;
        }
    }
    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) + " spend quota exceeded: cost " +
              cost.toString() + ", " + left->toString() + " remaining";
        return Failure::Quota;
    }
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " + cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }
    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory c, int units, Money cost,
                           const std::string& title) const {
    std::string why;
    evaluate(c, units, cost, title, why);
    return why;
}

void Budget::commit(ResourceCategory c, int units, Money cost,
                    const std::string& title) {
    std::string why;
    switch (evaluate(c, units, cost, title, why)) {
        case Failure::None: break;
        case Failure::BadInput: throw std::invalid_argument(why);
        case Failure::Quota: throw QuotaExceededError(why);
        case Failure::Overall: throw BudgetExceededError(why);
    }
    Usage& u = usage_[c];
    u.units += units;
    u.spent += cost;
    spent_ += cost;
    if (!title.empty()) titlesUsed_[c].insert(title);
}

void Budget::refund(ResourceCategory c, int units, Money cost,
                    const std::string& title, bool releaseTitle) {
    if (units <= 0) {
        throw std::invalid_argument("refund units must be positive");
    }
    if (cost.isNegative()) {
        throw std::invalid_argument("refund cost must not be negative");
    }

    auto it = usage_.find(c);
    if (it == usage_.end() || units > it->second.units) {
        throw std::invalid_argument("refund exceeds recorded category units");
    }
    if (cost > it->second.spent || cost > spent_) {
        throw std::invalid_argument("refund exceeds recorded spending");
    }

    it->second.units -= units;
    it->second.spent -= cost;
    spent_ -= cost;

    if (releaseTitle && !title.empty()) {
        auto titles = titlesUsed_.find(c);
        if (titles != titlesUsed_.end()) {
            titles->second.erase(title);
            if (titles->second.empty()) titlesUsed_.erase(titles);
        }
    }
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
       << "Spend used/max\n";
    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);
        const std::string units =
            std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
        const std::string spend =
            u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
        os << "  " << std::setw(20) << categoryName(c) << std::setw(18) << units << spend
           << "\n";
    }
}

}  // namespace bookmgmt
