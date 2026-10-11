#include "bookmgmt/Acquisition.h"

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

Budget& AcquisitionManager::addDepartment(const std::string& name,
                                          Money total) {
    if (name.empty()) {
        throw std::invalid_argument("department name cannot be empty");
    }

    auto result = departmentBudgets_.emplace(name, Budget(total));
    if (!result.second) {
        throw std::invalid_argument("department already exists: " + name);
    }
    return result.first->second;
}

Budget& AcquisitionManager::departmentBudget(const std::string& name) {
    auto it = departmentBudgets_.find(name);
    if (it == departmentBudgets_.end()) {
        throw std::invalid_argument("unknown department: " + name);
    }
    return it->second;
}

const Budget& AcquisitionManager::departmentBudget(
    const std::string& name) const {
    auto it = departmentBudgets_.find(name);
    if (it == departmentBudgets_.end()) {
        throw std::invalid_argument("unknown department: " + name);
    }
    return it->second;
}

Budget& AcquisitionManager::budgetForRequest(
    const std::string& department) {
    return department.empty() ? budget_ : departmentBudget(department);
}

const Budget& AcquisitionManager::budgetForRequest(
    const std::string& department) const {
    return department.empty() ? budget_ : departmentBudget(department);
}

void AcquisitionManager::setTaxRates(int printPercent, int electronicPercent) {
    if (printPercent < 0 || printPercent > 100 ||
        electronicPercent < 0 || electronicPercent > 100) {
        throw std::invalid_argument("tax rates must be between 0 and 100 percent");
    }

    printTaxPercent_ = printPercent;
    electronicTaxPercent_ = electronicPercent;
}

Money AcquisitionManager::applyTax(const Resource& resource,
                                  Money preTaxCost) const {
    const int rate = resource.isDigital()
                         ? electronicTaxPercent_
                         : printTaxPercent_;

    if (rate == 0 || preTaxCost.isZero()) return preTaxCost;

    // Money is stored in minor currency units. Calculate tax using integers
    // and round to the nearest minor unit, avoiding floating-point arithmetic.
    const std::int64_t minor = preTaxCost.minorUnits();
    const std::int64_t taxMinor =
        (minor / 100) * rate + ((minor % 100) * rate + 50) / 100;

    return preTaxCost + Money::fromMinor(taxMinor);
}

Money AcquisitionManager::quotePreTax(const std::string& id,
                                      int quantity) const {
    return catalog_.get(id).costFor(quantity);
}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    const Resource& resource = catalog_.get(id);
    return applyTax(resource, resource.costFor(quantity));
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason) const {
    return canPurchase(id, quantity, std::string{}, reason);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     const std::string& department,
                                     std::string* reason) const {
    std::string why;

    if (const Resource* resource = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Money preTaxCost = resource->costFor(quantity);
            const Money cost = applyTax(*resource, preTaxCost);

            try {
                why = budgetForRequest(department).check(
                    resource->category(), quantity, cost, resource->title());
            } catch (const std::invalid_argument& error) {
                why = error.what();
            }
        }
    } else {
        why = "resource not found: " + id;
    }

    if (reason) *reason = why;
    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(
    const Resource* resource, const std::string& id, int quantity,
    Money preTaxCost, Money cost, bool approved, std::string reason,
    bool isCancellation, int cancelsOrderNo, std::string department) {
    history_.push_back(PurchaseRecord{
        nextOrderNo_++, id,
        resource ? resource->title() : std::string("(unknown)"),
        resource ? resource->category() : ResourceCategory::Book,
        quantity, preTaxCost, cost, approved, std::move(reason),
        isCancellation, cancelsOrderNo, std::move(department)});
    return history_.back();
}

bool AcquisitionManager::hasCancellation(int orderNo) const {
    for (const auto& entry : history_) {
        if (entry.isCancellation && entry.cancelsOrderNo == orderNo) {
            return true;
        }
    }
    return false;
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id, int quantity) {
    return purchase(id, quantity, std::string{});
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id, int quantity, const std::string& department) {
    const Resource& resource = catalog_.get(id);
    const Money preTaxCost = resource.costFor(quantity);
    const Money cost = applyTax(resource, preTaxCost);

    budgetForRequest(department).commit(
        resource.category(), quantity, cost, resource.title());
    catalog_.addHoldings(id, quantity);

    return record(&resource, id, quantity, preTaxCost, cost, true, {},
                  false, 0, department);
}

const PurchaseRecord& AcquisitionManager::cancel(int orderNo) {
    // Find the original order. Copy it before appending to history, because
    // adding a record may invalidate references into the history vector.
    const PurchaseRecord* found = nullptr;
    for (const auto& entry : history_) {
        if (entry.orderNo == orderNo) {
            found = &entry;
            break;
        }
    }

    if (!found) {
        throw std::invalid_argument("order number not found: " +
                                    std::to_string(orderNo));
    }
    if (found->isCancellation) {
        throw std::invalid_argument("cannot cancel a cancellation record");
    }
    if (!found->approved) {
        throw std::invalid_argument("cannot cancel a rejected order");
    }
    if (hasCancellation(orderNo)) {
        throw std::invalid_argument("order has already been cancelled");
    }

    const PurchaseRecord original = *found;
    const Resource& resource = catalog_.get(original.resourceId);

    if (catalog_.holdings(original.resourceId) < original.quantity) {
        throw std::invalid_argument(
            "not enough holdings remain to cancel this order");
    }

    // Keep the title slot if any other approved, uncancelled order uses the
    // same title in the same category.
    bool releaseTitle = true;
    for (const auto& entry : history_) {
        if (entry.isCancellation || !entry.approved ||
            entry.orderNo == original.orderNo ||
            entry.category != original.category ||
            entry.title != original.title ||
            entry.department != original.department) {
            continue;
        }
        if (!hasCancellation(entry.orderNo)) {
            releaseTitle = false;
            break;
        }
    }

    // Reverse the original post-tax budget commitment and catalogue holdings.
    budgetForRequest(original.department).refund(
        original.category, original.quantity, original.cost,
        original.title, releaseTitle);
    catalog_.addHoldings(original.resourceId, -original.quantity);

    const Money negativePreTax =
        Money::fromMinor(-original.preTaxCost.minorUnits());
    const Money negativeCost = Money::fromMinor(-original.cost.minorUnits());

    // Retain the original order and append a separate reversal record.
    return record(&resource, original.resourceId, -original.quantity,
                  negativePreTax, negativeCost, true,
                  "cancels order #" + std::to_string(original.orderNo),
                  true, original.orderNo, original.department);
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& requests) {
    std::vector<PurchaseRecord> results;
    results.reserve(requests.size());

    for (const auto& request : requests) {
        const Resource* resource = catalog_.find(request.resourceId);
        Money preTaxCost;
        Money cost;
        std::string why;

        if (!resource) {
            why = "resource not found: " + request.resourceId;
        } else if (request.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            preTaxCost = resource->costFor(request.quantity);
            cost = applyTax(*resource, preTaxCost);

            try {
                why = budgetForRequest(request.department).check(
                    resource->category(), request.quantity, cost,
                    resource->title());
            } catch (const std::invalid_argument& error) {
                why = error.what();
            }
        }

        if (why.empty()) {
            results.push_back(
                purchase(request.resourceId, request.quantity,
                         request.department));
        } else {
            results.push_back(record(
                resource, request.resourceId, request.quantity,
                preTaxCost, cost, false, why, false, 0,
                request.department));
        }
    }

    return results;
}

Money AcquisitionManager::totalPreTax() const {
    Money sum;
    for (const auto& entry : history_) {
        if (entry.approved) sum += entry.preTaxCost;
    }
    return sum;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& entry : history_) {
        if (entry.approved) sum += entry.cost;
    }
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";

    for (const auto& entry : history_) {
        const char* status = entry.isCancellation
                                 ? "CANCELLED"
                                 : (entry.approved ? "APPROVED" : "REJECTED");
        os << "  #" << std::setw(3) << std::left << entry.orderNo << " "
           << status << "  " << std::setw(6) << entry.resourceId << " x"
           << std::setw(3) << entry.quantity
           << "  pre-tax " << std::setw(10) << std::right
           << entry.preTaxCost.toString()
           << "  post-tax " << std::setw(10) << entry.cost.toString()
           << std::left << "  " << entry.title;

        if (!entry.department.empty()) {
            os << "  [department: " << entry.department << "]";
        }

        if (entry.isCancellation) {
            os << "\n        cancellation of order #" << entry.cancelsOrderNo;
        } else if (!entry.approved) {
            os << "\n        reason: " << entry.reason;
        }
        os << "\n";
    }

    os << "Total pre-tax: " << totalPreTax() << "\n"
       << "Total post-tax: " << totalSpent() << "\n"
       << "Total spent: " << totalSpent() << "\n";
}

}  // namespace bookmgmt
