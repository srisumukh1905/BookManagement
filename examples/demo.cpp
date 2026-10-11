// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<Journal>("J001", "Journal of Systems", "2049-3630", 12,
                             "Open Press", 2025, Money::of(300), 2);
    catalog.emplace<Journal>("J002", "Journal of Networks", "2049-3631", 8,
                             "Open Press", 2025, Money::of(100));
    catalog.emplace<EBook>("EB001", "Practical C++",
                           std::vector<std::string>{"A. Author"},
                           "978-1111111111", "Open Press", 2025,
                           Money::of(10), "https://books.example/cpp",
                           EBookFormat::EPUB, true,
                           LicenseModel::AnnualSubscription, Money::of(50));
    catalog.emplace<AudioBook>("AB001", "C++ Audio Course",
                               "Alex Narrator", 180, "Audio Press", 2025,
                               Money::of(12), "https://audio.example/course",
                               LicenseModel::AnnualSubscription, Money::of(30));
    catalog.emplace<Thesis>("T001", "Thermal Research", "IIIT Delhi",
                            "MTech", "Dr. Example", "IIIT Delhi", 2026);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);

    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");
    std::cout << "\n=== Details of J001 ===\n" << catalog.get("J001");
    std::cout << "\n=== Details of EB001 ===\n" << catalog.get("EB001");
    std::cout << "\n=== Details of AB001 ===\n" << catalog.get("AB001");
    std::cout << "\n=== Details of T001 ===\n" << catalog.get("T001");

    Budget budget(Money::of(30000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(9800)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(14000)});
    budget.setQuota(ResourceCategory::Journal, {5, Money::of(4000)});
    budget.setTitleLimit(ResourceCategory::Journal, 1);
    budget.setQuota(ResourceCategory::EBook, {10, Money::of(1000)});
    budget.setQuota(ResourceCategory::AudioBook, {10, Money::of(1000)});
    budget.setQuota(ResourceCategory::Thesis, {10, Money{}});

    AcquisitionManager acq(catalog, budget);
    acq.setTaxRates(5, 18);
    std::cout << "Tax rates: print " << acq.printTaxRate()
              << "%, electronic " << acq.electronicTaxRate() << "%\n";

    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "10 copies of B001 = " << acq.quote("B001", 10)
              << "  (10% print bulk discount)\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "60 seats of R001  = " << acq.quote("R001", 60)
              << "  (seats beyond 50 at half price)\n";
    std::cout << "2 copies of J001  = " << acq.quote("J001", 2)
              << "  (two-year subscription)\n";
    std::cout << "3 seats of EB001  = " << acq.quote("EB001", 3)
              << "  (including platform fee)\n";
    std::cout << "2 seats of AB001  = " << acq.quote("AB001", 2)
              << "  (including platform fee)\n";
    std::cout << "1 copy of T001    = " << acq.quote("T001", 1)
              << "  (free thesis)\n";

    acq.processBatch({
        {"B001", 4},   // pre-tax 1800, post-tax 1890
        {"B002", 5},   // post-tax 7560 -> Book spend 9450
        {"B001", 1},   // rejected: post-tax cost exceeds remaining Book quota
        {"R001", 20},  // post-tax 5900
        {"R002", 25},  // rejected: only 20 electronic seats remain
        {"R002", 15},  // post-tax 7080 -> electronic spend 12980
        {"R002", 5},   // rejected: post-tax cost exceeds spend quota
        {"X999", 1},   // rejected: unknown id
        {"J001", 2},   // approved: two copies for two years
        {"J002", 1},   // rejected: second distinct Journal title
        {"EB001", 3},  // approved: three EBook seats
        {"AB001", 2},  // approved: two audiobook seats
        {"T001", 1},   // approved: free thesis
    });

    std::cout << "\n=== Cancellation demo ===\n";
    const PurchaseRecord& cancelled = acq.cancel(9);
    std::cout << "Cancelled order #" << cancelled.cancelsOrderNo
              << "; refunded post-tax amount: "
              << Money::fromMinor(-cancelled.cost.minorUnits()) << "\n";

    std::cout << "\n=== Department budgets demo (Q9) ===\n";
    Budget& computerScience =
        acq.addDepartment("Computer Science", Money::of(3000));
    computerScience.setQuota(
        ResourceCategory::Book, {5, Money::of(2500)});

    Budget& physics = acq.addDepartment("Physics", Money::of(1000));
    physics.setQuota(ResourceCategory::Book, {1, Money::of(500)});

    const auto departmentResults = acq.processBatch({
        {"B001", 2, "Computer Science"},
        {"B001", 1, "Physics"},
        {"B001", 1, "Physics"} // rejected: Physics quota is one copy
    });

    for (const auto& result : departmentResults) {
        std::cout << "Department request #" << result.orderNo << " ("
                  << result.department << "): "
                  << (result.approved ? "APPROVED" : "REJECTED");
        if (!result.approved) std::cout << " - " << result.reason;
        std::cout << "\n";
    }

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Computer Science department budget ===\n";
    acq.departmentBudget("Computer Science").print(std::cout);

    std::cout << "\n=== Physics department budget ===\n";
    acq.departmentBudget("Physics").print(std::cout);

    std::cout << "\n=== Year-end budget rollover demo (Q10) ===\n";
    Budget thisYear(Money::of(10000));
    thisYear.setQuota(ResourceCategory::Book, {20, Money::of(8000)});
    thisYear.commit(ResourceCategory::Book, 4, Money::of(2000),
                    "Example Book");
    std::cout << "This year's budget:\n";
    thisYear.print(std::cout);

    Budget nextYear = thisYear.nextYear(50);
    std::cout << "\nNext year's budget (50% of unspent amount carried forward):\n";
    nextYear.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    std::cout << "\n=== All-or-nothing batch demo (Q11) ===\n";
    Catalog atomicCatalog;
    atomicCatalog.emplace<Book>(
        "AT1", "Atomic Demo One", std::vector<std::string>{"Demo Author"},
        "ISBN-AT1", "Demo Press", 2025, Money::of(100));
    atomicCatalog.emplace<Book>(
        "AT2", "Atomic Demo Two", std::vector<std::string>{"Demo Author"},
        "ISBN-AT2", "Demo Press", 2025, Money::of(100));

    Budget atomicBudget(Money::of(250));
    atomicBudget.setQuota(ResourceCategory::Book, {3, Money::of(250)});
    AcquisitionManager atomicAcq(atomicCatalog, atomicBudget);

    std::cout << "Attempt 1: one request fits, but the complete batch exceeds budget\n";
    const auto aborted = atomicAcq.processBatch(
        {{"AT1", 1}, {"AT2", 2}}, true);
    for (const auto& result : aborted) {
        std::cout << "  " << result.resourceId << ": "
                  << (result.approved ? "APPROVED" : "REJECTED")
                  << " - " << result.reason << "\n";
    }
    std::cout << "After rejection: spent " << atomicBudget.spent()
              << ", AT1 holdings " << atomicCatalog.holdings("AT1")
              << ", AT2 holdings " << atomicCatalog.holdings("AT2") << "\n";

    std::cout << "Attempt 2: both requests fit\n";
    const auto accepted = atomicAcq.processBatch(
        {{"AT1", 1}, {"AT2", 1}}, true);
    for (const auto& result : accepted) {
        std::cout << "  " << result.resourceId << ": "
                  << (result.approved ? "APPROVED" : "REJECTED") << "\n";
    }
    std::cout << "After approval: spent " << atomicBudget.spent()
              << ", AT1 holdings " << atomicCatalog.holdings("AT1")
              << ", AT2 holdings " << atomicCatalog.holdings("AT2") << "\n";

    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
    return 0;
}
