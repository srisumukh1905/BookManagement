// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}

static void testJournal() {
    Journal j("J1", "Nature", "1234-5678", 12, "Springer", 2024,
              Money::of(500), 3);

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());
    CHECK(std::string(categoryName(j.category())) == "Journal");
    CHECK(j.issn() == "1234-5678");
    CHECK(j.issuesPerYear() == 12);
    CHECK(j.subscriptionYears() == 3);

    // Two copies for three years: 500 x 2 x 3 = 3000.
    CHECK(j.costFor(2) == Money::of(3000));
    CHECK_THROWS(j.costFor(0), std::invalid_argument);
    CHECK_THROWS(j.costFor(-1), std::invalid_argument);

    Journal oneYear("J2", "Testing Journal", "8765-4321", 4,
                    "Example Publisher", 2025, Money::of(125));
    CHECK(oneYear.subscriptionYears() == 1);
    CHECK(oneYear.costFor(2) == Money::of(250));

    CHECK_THROWS(
        Journal("J3", "Invalid Journal", "1111-2222", 12,
                "Publisher", 2025, Money::of(100), 0),
        std::invalid_argument);
    CHECK_THROWS(
        Journal("J4", "Invalid Journal", "1111-2222", 12,
                "Publisher", 2025, Money::of(100), -1),
        std::invalid_argument);

    std::ostringstream details;
    details << j;
    CHECK(details.str().find("issn: 1234-5678") != std::string::npos);
    CHECK(details.str().find("issues per year: 12") != std::string::npos);
    CHECK(details.str().find("subscription years: 3") != std::string::npos);

    Budget budget(Money::of(10000));
    budget.setQuota(ResourceCategory::Journal, {4, Money::of(5000)});
    CHECK(budget.check(ResourceCategory::Journal, 2, j.costFor(2)).empty());
    budget.commit(ResourceCategory::Journal, 2, j.costFor(2));
    CHECK(*budget.unitsRemaining(ResourceCategory::Journal) == 2);

    std::ostringstream report;
    budget.print(report);
    CHECK(report.str().find("Journal") != std::string::npos);
}

static void testEBook() {
    EBook e("EB1", "C++ Fundamentals", {"Alice", "Bob"},
            "978-1234567890", "Example Press", 2025,
            Money::of(10), "https://example.org/book.pdf",
            EBookFormat::PDF, true,
            LicenseModel::AnnualSubscription, Money::of(20));

    CHECK(e.category() == ResourceCategory::EBook);
    CHECK(e.isDigital());
    CHECK(e.authors().size() == 2);
    CHECK(e.authors()[0] == "Alice");
    CHECK(e.isbn() == "978-1234567890");
    CHECK(e.format() == EBookFormat::PDF);
    CHECK(e.drmProtected());

    // Inherited pricing: platform fee 20 + 3 seats x 10 = 50.
    CHECK(e.costFor(3) == Money::of(50));
    CHECK_THROWS(e.costFor(0), std::invalid_argument);

    EBook epub("EB2", "EPUB Book", {"Carol"}, "ISBN-2",
               "Publisher", 2024, Money::of(5), "https://example.org/book.epub",
               EBookFormat::EPUB, false);
    CHECK(epub.format() == EBookFormat::EPUB);
    CHECK(!epub.drmProtected());
    CHECK(epub.costFor(2) == Money::of(10));

    EBook html("EB3", "HTML Book", {"Dan"}, "ISBN-3",
               "Publisher", 2023, Money::of(7), "https://example.org/book.html",
               EBookFormat::HTML, true);
    CHECK(html.format() == EBookFormat::HTML);
    CHECK(std::string(eBookFormatName(html.format())) == "HTML");

    std::ostringstream details;
    details << e;
    CHECK(details.str().find("access url: https://example.org/book.pdf") !=
          std::string::npos);
    CHECK(details.str().find("platform fee: 20.00") != std::string::npos);
    CHECK(details.str().find("authors: Alice and Bob") != std::string::npos);
    CHECK(details.str().find("isbn: 978-1234567890") != std::string::npos);
    CHECK(details.str().find("file format: PDF") != std::string::npos);
    CHECK(details.str().find("drm protected: yes") != std::string::npos);

    Budget budget(Money::of(1000));
    budget.setQuota(ResourceCategory::ElectronicResource, {10, Money::of(500)});
    budget.setQuota(ResourceCategory::EBook, {3, Money::of(100)});
    CHECK(budget.check(ResourceCategory::EBook, 2, e.costFor(2)).empty());
    budget.commit(ResourceCategory::EBook, 2, e.costFor(2));
    CHECK(*budget.unitsRemaining(ResourceCategory::EBook) == 1);
    CHECK(*budget.unitsRemaining(ResourceCategory::ElectronicResource) == 10);
}

static void testAudioBookAndThesis() {
    AudioBook audio("AB1", "C++ Audio Course", "Alex Narrator", 180,
                    "Audio Press", 2025, Money::of(12),
                    "https://audio.example/course",
                    LicenseModel::AnnualSubscription, Money::of(30));

    CHECK(audio.category() == ResourceCategory::AudioBook);
    CHECK(audio.isDigital());
    CHECK(audio.narrator() == "Alex Narrator");
    CHECK(audio.durationMinutes() == 180);
    // Inherited electronic pricing: platform fee 30 + 2 seats x 12.
    CHECK(audio.costFor(2) == Money::of(54));
    CHECK_THROWS(audio.costFor(0), std::invalid_argument);
    CHECK_THROWS(
        AudioBook("AB2", "Invalid Audio", "Narrator", 0, "Publisher",
                  2025, Money::of(10), "url"),
        std::invalid_argument);
    CHECK_THROWS(
        AudioBook("AB3", "Invalid Audio", "Narrator", -5, "Publisher",
                  2025, Money::of(10), "url"),
        std::invalid_argument);

    std::ostringstream audioDetails;
    audioDetails << audio;
    CHECK(audioDetails.str().find("narrator: Alex Narrator") !=
          std::string::npos);
    CHECK(audioDetails.str().find("duration (minutes): 180") !=
          std::string::npos);
    CHECK(audioDetails.str().find("platform fee: 30.00") !=
          std::string::npos);

    // A thesis defaults to free of cost unless a price is supplied.
    Thesis thesis("T1", "Thermal Research", "IIIT Delhi", "MTech",
                  "Dr. Example", "IIIT Delhi", 2026);
    CHECK(thesis.category() == ResourceCategory::Thesis);
    CHECK(!thesis.isDigital());
    CHECK(thesis.university() == "IIIT Delhi");
    CHECK(thesis.degree() == "MTech");
    CHECK(thesis.supervisor() == "Dr. Example");
    CHECK(thesis.unitPrice() == Money{});
    CHECK(thesis.costFor(3) == Money{});
    CHECK_THROWS(thesis.costFor(0), std::invalid_argument);

    std::ostringstream thesisDetails;
    thesisDetails << thesis;
    CHECK(thesisDetails.str().find("university: IIIT Delhi") !=
          std::string::npos);
    CHECK(thesisDetails.str().find("degree: MTech") != std::string::npos);
    CHECK(thesisDetails.str().find("supervisor: Dr. Example") !=
          std::string::npos);

    Budget budget(Money::of(1000));
    budget.setQuota(ResourceCategory::AudioBook, {4, Money::of(300)});
    budget.setQuota(ResourceCategory::Thesis, {10, Money{}});
    budget.commit(ResourceCategory::AudioBook, 2, audio.costFor(2));
    budget.commit(ResourceCategory::Thesis, 2, thesis.costFor(2));
    CHECK(*budget.unitsRemaining(ResourceCategory::AudioBook) == 2);
    CHECK(*budget.unitsRemaining(ResourceCategory::Thesis) == 8);
    CHECK(budget.spent() == Money::of(54));
}

static void testHardcoverPricing() {
    Book paperback("PB1", "Paperback Test", {"Author"}, "ISBN-P",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Paperback);

    Book hardcover("HC1", "Hardcover Test", {"Author"}, "ISBN-H",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Hardcover);

    // Paperback pricing remains unchanged.
    CHECK(paperback.unitPrice() == Money::of(100));
    CHECK(paperback.costFor(2) == Money::of(200));

    // Hardcover costs 20% more, but its listed unit price is unchanged.
    CHECK(hardcover.unitPrice() == Money::of(100));
    CHECK(hardcover.costFor(1) == Money::of(120));
    CHECK(hardcover.costFor(2) == Money::of(240));

    // Verify rounding to the nearest minor currency unit.
    Book rounding("HC2", "Rounding Test", {"Author"}, "ISBN-R",
                  "Publisher", 2025, Money::fromMinor(101), 1,
                  Binding::Hardcover);
    CHECK(rounding.costFor(1) == Money::fromMinor(121));

    CHECK_THROWS(hardcover.costFor(0), std::invalid_argument);
}

static void testBulkDiscounts() {
    Book paperback("PBQ5", "Bulk Paperback", {"Author"}, "ISBN-P",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Paperback);

    CHECK(paperback.costFor(9) == Money::of(900));
    CHECK(paperback.costFor(10) == Money::of(900));
    CHECK(paperback.costFor(11) == Money::of(990));

    // The hardcover premium is applied before the print bulk discount.
    Book hardcover("HCQ5", "Bulk Hardcover", {"Author"}, "ISBN-H",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Hardcover);
    CHECK(hardcover.costFor(9) == Money::of(1080));
    CHECK(hardcover.costFor(10) == Money::of(1080));
    CHECK(hardcover.costFor(11) == Money::of(1188));

    Journal journal("JQ5", "Bulk Journal", "1234-5678", 12,
                    "Publisher", 2025, Money::of(100));
    CHECK(journal.costFor(9) == Money::of(900));
    CHECK(journal.costFor(10) == Money::of(900));
    CHECK(journal.costFor(11) == Money::of(990));

    ElectronicResource digital(
        "ERQ5", "Digital Database", "Publisher", 2025,
        Money::of(10), "https://digital.example",
        LicenseModel::AnnualSubscription, Money::of(20));

    // Platform fee remains unchanged; only seats after seat 50 are half price.
    CHECK(digital.costFor(1) == Money::of(30));
    CHECK(digital.costFor(50) == Money::of(520));
    CHECK(digital.costFor(51) == Money::of(525));
    CHECK(digital.costFor(60) == Money::of(570));
    CHECK_THROWS(digital.costFor(0), std::invalid_argument);

    EBook ebook("EBQ5", "Bulk EBook", {"Author"}, "ISBN-E",
                "Publisher", 2025, Money::of(10), "https://ebook.example",
                EBookFormat::PDF, false,
                LicenseModel::AnnualSubscription, Money::of(20));
    CHECK(ebook.costFor(51) == Money::of(525));

    AudioBook audio("ABQ5", "Bulk Audio", "Narrator", 60,
                    "Publisher", 2025, Money::of(10),
                    "https://audio.example",
                    LicenseModel::AnnualSubscription, Money::of(20));
    CHECK(audio.costFor(51) == Money::of(525));
}


static void testTaxes() {
    Catalog c;
    c.emplace<Book>("TB1", "Tax Book", std::vector<std::string>{"A"},
                    "ISBN", "Publisher", 2024, Money::of(100));
    c.emplace<ElectronicResource>(
        "TR1", "Tax DB", "Publisher", 2025, Money::of(10), "url",
        LicenseModel::AnnualSubscription, Money::of(20));

    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(500)});
    b.setQuota(ResourceCategory::ElectronicResource, {10, Money::of(500)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("TB1", 2) == Money::of(200));
    acq.setTaxRates(10, 20);
    CHECK(acq.quotePreTax("TB1", 2) == Money::of(200));
    CHECK(acq.quote("TB1", 2) == Money::of(220));
    CHECK(acq.quotePreTax("TR1", 2) == Money::of(40));
    CHECK(acq.quote("TR1", 2) == Money::of(48));

    const PurchaseRecord br = acq.purchase("TB1", 2);
    const PurchaseRecord er = acq.purchase("TR1", 2);
    CHECK(br.preTaxCost == Money::of(200) && br.cost == Money::of(220));
    CHECK(er.preTaxCost == Money::of(40) && er.cost == Money::of(48));
    CHECK(acq.totalPreTax() == Money::of(240));
    CHECK(acq.totalSpent() == Money::of(268));

    std::ostringstream report;
    acq.printReport(report);
    CHECK(report.str().find("Total pre-tax: 240.00") != std::string::npos);
    CHECK(report.str().find("Total post-tax: 268.00") != std::string::npos);

    CHECK_THROWS(acq.setTaxRates(-1, 20), std::invalid_argument);
    CHECK_THROWS(acq.setTaxRates(10, 101), std::invalid_argument);
    CHECK(acq.printTaxRate() == 10 && acq.electronicTaxRate() == 20);

    // Pre-tax cost is 200, but 10% tax makes it 220, over this quota.
    Catalog c2;
    c2.emplace<Book>("QB1", "Quota Book", std::vector<std::string>{"A"},
                     "ISBN-Q", "Publisher", 2024, Money::of(100));
    Budget b2(Money::of(1000));
    b2.setQuota(ResourceCategory::Book, {10, Money::of(215)});
    AcquisitionManager a2(c2, b2);
    a2.setTaxRates(10, 20);
    std::string reason;
    CHECK(!a2.canPurchase("QB1", 2, &reason));
    CHECK(reason.find("220.00") != std::string::npos);
    CHECK_THROWS(a2.purchase("QB1", 2), QuotaExceededError);
    CHECK(b2.spent() == Money{});
    CHECK(c2.holdings("QB1") == 0);
}

static void testTitleLimits() {
    Catalog catalog;
    catalog.emplace<Book>("TL1", "Alpha", std::vector<std::string>{"A"},
                          "ISBN-1", "Publisher", 2024, Money::of(10));
    catalog.emplace<Book>("TL2", "Alpha", std::vector<std::string>{"B"},
                          "ISBN-2", "Publisher", 2025, Money::of(10));
    catalog.emplace<Book>("TL3", "Beta", std::vector<std::string>{"C"},
                          "ISBN-3", "Publisher", 2025, Money::of(10));
    catalog.emplace<Book>("TL4", "Gamma", std::vector<std::string>{"D"},
                          "ISBN-4", "Publisher", 2026, Money::of(10));
    catalog.emplace<ElectronicResource>(
        "TL5", "Digital Title", "Publisher", 2025, Money::of(5), "url");

    Budget budget(Money::of(1000));
    budget.setQuota(ResourceCategory::Book, {100, Money::of(900)});
    budget.setTitleLimit(ResourceCategory::Book, 2);

    AcquisitionManager acq(catalog, budget);

    CHECK(budget.titleLimitFor(ResourceCategory::Book).value() == 2);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::Book) == 0);

    // Different resource IDs with the same title count as one title.
    CHECK(acq.purchase("TL1", 3).approved);
    CHECK(acq.purchase("TL2", 2).approved);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::Book) == 1);

    // A genuinely different title uses the second title slot.
    CHECK(acq.purchase("TL3", 1).approved);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::Book) == 2);

    // Another title is rejected even though unit and spending quota remain.
    std::string reason;
    CHECK(!acq.canPurchase("TL4", 1, &reason));
    CHECK(reason.find("distinct-title limit") != std::string::npos);
    CHECK_THROWS(acq.purchase("TL4", 1), QuotaExceededError);
    CHECK(catalog.holdings("TL4") == 0);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::Book) == 2);

    // A rejected batch item is recorded but doesn't consume a title slot.
    const auto before = acq.history().size();
    auto result = acq.processBatch({{"TL4", 1}});
    CHECK(result.size() == 1);
    CHECK(!result[0].approved);
    CHECK(result[0].reason.find("distinct-title limit") != std::string::npos);
    CHECK(acq.history().size() == before + 1);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::Book) == 2);

    // Title limits are category-specific.
    CHECK(acq.purchase("TL5", 1).approved);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::EBook) == 0);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::ElectronicResource) == 1);

    CHECK_THROWS(budget.setTitleLimit(ResourceCategory::Book, -1),
                 std::invalid_argument);

    budget.removeTitleLimit(ResourceCategory::Book);
    CHECK(!budget.titleLimitFor(ResourceCategory::Book).has_value());
    CHECK(acq.purchase("TL4", 1).approved);
    CHECK(budget.distinctTitlesUsed(ResourceCategory::Book) == 3);
}


static void testCancellation() {
    Catalog c;
    c.emplace<Book>("CX1", "Cancel Me", std::vector<std::string>{"A"},
                    "ISBN-CX", "Publisher", 2025, Money::of(100));
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(500)});
    AcquisitionManager acq(c, b);
    acq.setTaxRates(10, 20);

    acq.purchase("CX1", 2);
    CHECK(b.spent() == Money::of(220));
    CHECK(c.holdings("CX1") == 2);

    const PurchaseRecord cancelled = acq.cancel(1);
    CHECK(cancelled.isCancellation);
    CHECK(cancelled.cancelsOrderNo == 1);
    CHECK(cancelled.quantity == -2);
    CHECK(cancelled.preTaxCost == Money::of(-200));
    CHECK(cancelled.cost == Money::of(-220));
    CHECK(acq.history().size() == 2);
    CHECK(!acq.history()[0].isCancellation);
    CHECK(b.spent() == Money{});
    CHECK(b.usageFor(ResourceCategory::Book).units == 0);
    CHECK(c.holdings("CX1") == 0);
    CHECK(acq.totalPreTax() == Money{});
    CHECK(acq.totalSpent() == Money{});
    CHECK_THROWS(acq.cancel(1), std::invalid_argument);
}
static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}


static void testDepartmentBudgets() {
    Catalog c;
    c.emplace<Book>("D1", "Shared Title", std::vector<std::string>{"A"},
                    "isbn", "P", 2024, Money::of(100));

    Budget defaultBudget(Money::of(1000));
    AcquisitionManager acq(c, defaultBudget);

    Budget& cs = acq.addDepartment("Computer Science", Money::of(100));
    Budget& physics = acq.addDepartment("Physics", Money::of(300));

    cs.setQuota(ResourceCategory::Book, {1, Money::of(100)});
    physics.setQuota(ResourceCategory::Book, {2, Money::of(200)});
    cs.setTitleLimit(ResourceCategory::Book, 1);
    physics.setTitleLimit(ResourceCategory::Book, 1);

    std::string why;
    CHECK(acq.canPurchase("D1", 1, "Computer Science", &why));

    const int csOrderNo =
        acq.purchase("D1", 1, "Computer Science").orderNo;
    CHECK(cs.spent() == Money::of(100));
    CHECK(acq.history().back().department == "Computer Science");
    CHECK(!acq.canPurchase("D1", 1, "Computer Science", &why));

    auto results = acq.processBatch({
        {"D1", 1, "Physics"},
        {"D1", 1, "Physics"},
        {"D1", 1, "Unknown Department"}
    });

    CHECK(results.size() == 3);
    CHECK(results[0].approved && results[0].department == "Physics");
    CHECK(results[1].approved && results[1].department == "Physics");
    CHECK(!results[2].approved);
    CHECK(results[2].reason.find("unknown department") != std::string::npos);
    CHECK(physics.spent() == Money::of(200));
    CHECK(defaultBudget.spent() == Money{});
    CHECK(!acq.canPurchase("D1", 1, "Physics", &why));

    const auto& cancelled = acq.cancel(csOrderNo);
    CHECK(cancelled.isCancellation);
    CHECK(cancelled.department == "Computer Science");
    CHECK(cs.spent() == Money{});
    CHECK(physics.spent() == Money::of(200));
    CHECK(acq.canPurchase("D1", 1, "Computer Science", &why));
    CHECK(c.holdings("D1") == 2);
}

int main() {
    testMoney();
    testResourcesAndCost();
    testHardcoverPricing();
    testBulkDiscounts();
    testTaxes();
    testTitleLimits();
    testCancellation();
    testJournal();
    testEBook();
    testAudioBookAndThesis();
    testCatalog();
    testBudget();
    testAcquisition();
    testDepartmentBudgets();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
