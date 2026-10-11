#pragma once
// AcquisitionManager: purchase requests, taxes, budget enforcement and history.

#include <iosfwd>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;  // copies for print, seats for electronic
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money cost;  // final post-tax amount
    bool approved;
    std::string reason;
    bool isCancellation = false;
    int cancelsOrderNo = 0;
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget);

    // Configure whole-number tax percentages (0 through 100).
    void setTaxRates(int printPercent, int electronicPercent);
    int printTaxRate() const { return printTaxPercent_; }
    int electronicTaxRate() const { return electronicTaxPercent_; }

    // Price before tax, without buying.
    Money quotePreTax(const std::string& id, int quantity) const;

    // Final price after the configured tax, without buying.
    Money quote(const std::string& id, int quantity) const;

    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    const PurchaseRecord& purchase(const std::string& id, int quantity);

    // Cancel an approved order once, refunding its post-tax cost and units.
    // The original history record is kept and a separate cancellation
    // record is appended. Throws std::invalid_argument for invalid orders.
    const PurchaseRecord& cancel(int orderNo);

    // Processes requests in order; every outcome is recorded.
    std::vector<PurchaseRecord> processBatch(
        const std::vector<PurchaseRequest>& reqs);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalPreTax() const;
    Money totalSpent() const;  // post-tax spending

    void printReport(std::ostream& os) const;

private:
    Money applyTax(const Resource& resource, Money preTaxCost) const;

    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                           Money preTaxCost, Money cost, bool approved,
                           std::string reason, bool isCancellation = false,
                           int cancelsOrderNo = 0);
    bool hasCancellation(int orderNo) const;

    Catalog& catalog_;
    Budget& budget_;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;

    // Zero rates preserve the original behaviour unless taxes are configured.
    int printTaxPercent_ = 0;
    int electronicTaxPercent_ = 0;
};

}  // namespace bookmgmt
