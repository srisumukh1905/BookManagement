// Minimal self-contained test runner (no external framework needed).

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

template <typename Exception, typename Callable>
static void assertThrows(Callable&& fn) {
    bool thrown = false;
    try {
        fn();
    } catch (const Exception&) {
        thrown = true;
    } catch (...) {
    }
    assert(thrown);
}

static void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");;
    assert(Money::of(-3, 50).toString() == "-3.50");;
    assert(Money::fromMinor(7).toString() == "0.07");;
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));;
    assert(Money::of(3) * 4 == Money::of(12));;
    assert(Money::of(1) < Money::of(2));;
    assertThrows<std::invalid_argument>([&]() { (void)(Money::of(1, 100)); });;
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert(b.category() == ResourceCategory::Book);;
    assert(!b.isDigital());;
    assert(b.costFor(3) == Money::of(300));;
    assertThrows<std::invalid_argument>([&]() { (void)(b.costFor(0)); });;
    assert(joinAuthors(b.authors()) == "A, B and C");;

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert(e.isDigital());;
    assert(e.costFor(5) == Money::of(150));;

    assert(e.category() == ResourceCategory::ElectronicResource);;

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert(r.costFor(1) == Money::of(110));;
    std::ostringstream os;
    os << r;
    assert(os.str().find("platform fee: 100.00") != std::string::npos);;

    assertThrows<std::invalid_argument>([&]() { (void)(Book("", "T", {}, "", "", 2000, Money::of(1))); });;
    assertThrows<std::invalid_argument>([&]() { (void)(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1))); });;
}

static void testJournal() {
    Journal j("J1", "Nature", "1234-5678", 12, "Springer", 2024,
              Money::of(500), 3);

    assert(j.category() == ResourceCategory::Journal);;
    assert(!j.isDigital());;
    assert(std::string(categoryName(j.category())) == "Journal");;
    assert(j.issn() == "1234-5678");;
    assert(j.issuesPerYear() == 12);;
    assert(j.subscriptionYears() == 3);;

    // Two copies for three years: 500 x 2 x 3 = 3000.
    assert(j.costFor(2) == Money::of(3000));;
    assertThrows<std::invalid_argument>([&]() { (void)(j.costFor(0)); });;
    assertThrows<std::invalid_argument>([&]() { (void)(j.costFor(-1)); });;

    Journal oneYear("J2", "Testing Journal", "8765-4321", 4,
                    "Example Publisher", 2025, Money::of(125));
    assert(oneYear.subscriptionYears() == 1);;
    assert(oneYear.costFor(2) == Money::of(250));;

    assertThrows<std::invalid_argument>([&]() { (void)(Journal("J3", "Invalid Journal", "1111-2222", 12,
                "Publisher", 2025, Money::of(100), 0)); });;
    assertThrows<std::invalid_argument>([&]() { (void)(Journal("J4", "Invalid Journal", "1111-2222", 12,
                "Publisher", 2025, Money::of(100), -1)); });;

    std::ostringstream details;
    details << j;
    assert(details.str().find("issn: 1234-5678") != std::string::npos);;
    assert(details.str().find("issues per year: 12") != std::string::npos);;
    assert(details.str().find("subscription years: 3") != std::string::npos);;

    Budget budget(Money::of(10000));
    budget.setQuota(ResourceCategory::Journal, {4, Money::of(5000)});
    assert(budget.check(ResourceCategory::Journal, 2, j.costFor(2)).empty());;
    budget.commit(ResourceCategory::Journal, 2, j.costFor(2));
    assert(*budget.unitsRemaining(ResourceCategory::Journal) == 2);;

    std::ostringstream report;
    budget.print(report);
    assert(report.str().find("Journal") != std::string::npos);;
}

static void testEBook() {
    EBook e("EB1", "C++ Fundamentals", {"Alice", "Bob"},
            "978-1234567890", "Example Press", 2025,
            Money::of(10), "https://example.org/book.pdf",
            EBookFormat::PDF, true,
            LicenseModel::AnnualSubscription, Money::of(20));

    assert(e.category() == ResourceCategory::EBook);;
    assert(e.isDigital());;
    assert(e.authors().size() == 2);;
    assert(e.authors()[0] == "Alice");;
    assert(e.isbn() == "978-1234567890");;
    assert(e.format() == EBookFormat::PDF);;
    assert(e.drmProtected());;

    // Inherited pricing: platform fee 20 + 3 seats x 10 = 50.
    assert(e.costFor(3) == Money::of(50));;
    assertThrows<std::invalid_argument>([&]() { (void)(e.costFor(0)); });;

    EBook epub("EB2", "EPUB Book", {"Carol"}, "ISBN-2",
               "Publisher", 2024, Money::of(5), "https://example.org/book.epub",
               EBookFormat::EPUB, false);
    assert(epub.format() == EBookFormat::EPUB);;
    assert(!epub.drmProtected());;
    assert(epub.costFor(2) == Money::of(10));;

    EBook html("EB3", "HTML Book", {"Dan"}, "ISBN-3",
               "Publisher", 2023, Money::of(7), "https://example.org/book.html",
               EBookFormat::HTML, true);
    assert(html.format() == EBookFormat::HTML);;
    assert(std::string(eBookFormatName(html.format())) == "HTML");;

    std::ostringstream details;
    details << e;
    assert(details.str().find("access url: https://example.org/book.pdf") !=
          std::string::npos);;
    assert(details.str().find("platform fee: 20.00") != std::string::npos);;
    assert(details.str().find("authors: Alice and Bob") != std::string::npos);;
    assert(details.str().find("isbn: 978-1234567890") != std::string::npos);;
    assert(details.str().find("file format: PDF") != std::string::npos);;
    assert(details.str().find("drm protected: yes") != std::string::npos);;

    Budget budget(Money::of(1000));
    budget.setQuota(ResourceCategory::ElectronicResource, {10, Money::of(500)});
    budget.setQuota(ResourceCategory::EBook, {3, Money::of(100)});
    assert(budget.check(ResourceCategory::EBook, 2, e.costFor(2)).empty());;
    budget.commit(ResourceCategory::EBook, 2, e.costFor(2));
    assert(*budget.unitsRemaining(ResourceCategory::EBook) == 1);;
    assert(*budget.unitsRemaining(ResourceCategory::ElectronicResource) == 10);;
}

static void testAudioBookAndThesis() {
    AudioBook audio("AB1", "C++ Audio Course", "Alex Narrator", 180,
                    "Audio Press", 2025, Money::of(12),
                    "https://audio.example/course",
                    LicenseModel::AnnualSubscription, Money::of(30));

    assert(audio.category() == ResourceCategory::AudioBook);;
    assert(audio.isDigital());;
    assert(audio.narrator() == "Alex Narrator");;
    assert(audio.durationMinutes() == 180);;
    // Inherited electronic pricing: platform fee 30 + 2 seats x 12.
    assert(audio.costFor(2) == Money::of(54));;
    assertThrows<std::invalid_argument>([&]() { (void)(audio.costFor(0)); });;
    assertThrows<std::invalid_argument>([&]() { (void)(AudioBook("AB2", "Invalid Audio", "Narrator", 0, "Publisher",
                  2025, Money::of(10), "url")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(AudioBook("AB3", "Invalid Audio", "Narrator", -5, "Publisher",
                  2025, Money::of(10), "url")); });;

    std::ostringstream audioDetails;
    audioDetails << audio;
    assert(audioDetails.str().find("narrator: Alex Narrator") !=
          std::string::npos);;
    assert(audioDetails.str().find("duration (minutes): 180") !=
          std::string::npos);;
    assert(audioDetails.str().find("platform fee: 30.00") !=
          std::string::npos);;

    // A thesis defaults to free of cost unless a price is supplied.
    Thesis thesis("T1", "Thermal Research", "IIIT Delhi", "MTech",
                  "Dr. Example", "IIIT Delhi", 2026);
    assert(thesis.category() == ResourceCategory::Thesis);;
    assert(!thesis.isDigital());;
    assert(thesis.university() == "IIIT Delhi");;
    assert(thesis.degree() == "MTech");;
    assert(thesis.supervisor() == "Dr. Example");;
    assert(thesis.unitPrice() == Money{});;
    assert(thesis.costFor(3) == Money{});;
    assertThrows<std::invalid_argument>([&]() { (void)(thesis.costFor(0)); });;

    std::ostringstream thesisDetails;
    thesisDetails << thesis;
    assert(thesisDetails.str().find("university: IIIT Delhi") !=
          std::string::npos);;
    assert(thesisDetails.str().find("degree: MTech") != std::string::npos);;
    assert(thesisDetails.str().find("supervisor: Dr. Example") !=
          std::string::npos);;

    Budget budget(Money::of(1000));
    budget.setQuota(ResourceCategory::AudioBook, {4, Money::of(300)});
    budget.setQuota(ResourceCategory::Thesis, {10, Money{}});
    budget.commit(ResourceCategory::AudioBook, 2, audio.costFor(2));
    budget.commit(ResourceCategory::Thesis, 2, thesis.costFor(2));
    assert(*budget.unitsRemaining(ResourceCategory::AudioBook) == 2);;
    assert(*budget.unitsRemaining(ResourceCategory::Thesis) == 8);;
    assert(budget.spent() == Money::of(54));;
}

static void testHardcoverPricing() {
    Book paperback("PB1", "Paperback Test", {"Author"}, "ISBN-P",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Paperback);

    Book hardcover("HC1", "Hardcover Test", {"Author"}, "ISBN-H",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Hardcover);

    // Paperback pricing remains unchanged.
    assert(paperback.unitPrice() == Money::of(100));;
    assert(paperback.costFor(2) == Money::of(200));;

    // Hardcover costs 20% more, but its listed unit price is unchanged.
    assert(hardcover.unitPrice() == Money::of(100));;
    assert(hardcover.costFor(1) == Money::of(120));;
    assert(hardcover.costFor(2) == Money::of(240));;

    // Verify rounding to the nearest minor currency unit.
    Book rounding("HC2", "Rounding Test", {"Author"}, "ISBN-R",
                  "Publisher", 2025, Money::fromMinor(101), 1,
                  Binding::Hardcover);
    assert(rounding.costFor(1) == Money::fromMinor(121));;

    assertThrows<std::invalid_argument>([&]() { (void)(hardcover.costFor(0)); });;
}

static void testBulkDiscounts() {
    Book paperback("PBQ5", "Bulk Paperback", {"Author"}, "ISBN-P",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Paperback);

    assert(paperback.costFor(9) == Money::of(900));;
    assert(paperback.costFor(10) == Money::of(900));;
    assert(paperback.costFor(11) == Money::of(990));;

    // The hardcover premium is applied before the print bulk discount.
    Book hardcover("HCQ5", "Bulk Hardcover", {"Author"}, "ISBN-H",
                   "Publisher", 2025, Money::of(100), 1,
                   Binding::Hardcover);
    assert(hardcover.costFor(9) == Money::of(1080));;
    assert(hardcover.costFor(10) == Money::of(1080));;
    assert(hardcover.costFor(11) == Money::of(1188));;

    Journal journal("JQ5", "Bulk Journal", "1234-5678", 12,
                    "Publisher", 2025, Money::of(100));
    assert(journal.costFor(9) == Money::of(900));;
    assert(journal.costFor(10) == Money::of(900));;
    assert(journal.costFor(11) == Money::of(990));;

    ElectronicResource digital(
        "ERQ5", "Digital Database", "Publisher", 2025,
        Money::of(10), "https://digital.example",
        LicenseModel::AnnualSubscription, Money::of(20));

    // Platform fee remains unchanged; only seats after seat 50 are half price.
    assert(digital.costFor(1) == Money::of(30));;
    assert(digital.costFor(50) == Money::of(520));;
    assert(digital.costFor(51) == Money::of(525));;
    assert(digital.costFor(60) == Money::of(570));;
    assertThrows<std::invalid_argument>([&]() { (void)(digital.costFor(0)); });;

    EBook ebook("EBQ5", "Bulk EBook", {"Author"}, "ISBN-E",
                "Publisher", 2025, Money::of(10), "https://ebook.example",
                EBookFormat::PDF, false,
                LicenseModel::AnnualSubscription, Money::of(20));
    assert(ebook.costFor(51) == Money::of(525));;

    AudioBook audio("ABQ5", "Bulk Audio", "Narrator", 60,
                    "Publisher", 2025, Money::of(10),
                    "https://audio.example",
                    LicenseModel::AnnualSubscription, Money::of(20));
    assert(audio.costFor(51) == Money::of(525));;
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

    assert(acq.quote("TB1", 2) == Money::of(200));;
    acq.setTaxRates(10, 20);
    assert(acq.quotePreTax("TB1", 2) == Money::of(200));;
    assert(acq.quote("TB1", 2) == Money::of(220));;
    assert(acq.quotePreTax("TR1", 2) == Money::of(40));;
    assert(acq.quote("TR1", 2) == Money::of(48));;

    const PurchaseRecord br = acq.purchase("TB1", 2);
    const PurchaseRecord er = acq.purchase("TR1", 2);
    assert(br.preTaxCost == Money::of(200) && br.cost == Money::of(220));;
    assert(er.preTaxCost == Money::of(40) && er.cost == Money::of(48));;
    assert(acq.totalPreTax() == Money::of(240));;
    assert(acq.totalSpent() == Money::of(268));;

    std::ostringstream report;
    acq.printReport(report);
    assert(report.str().find("Total pre-tax: 240.00") != std::string::npos);;
    assert(report.str().find("Total post-tax: 268.00") != std::string::npos);;

    assertThrows<std::invalid_argument>([&]() { (void)(acq.setTaxRates(-1, 20)); });;
    assertThrows<std::invalid_argument>([&]() { (void)(acq.setTaxRates(10, 101)); });;
    assert(acq.printTaxRate() == 10 && acq.electronicTaxRate() == 20);;

    // Pre-tax cost is 200, but 10% tax makes it 220, over this quota.
    Catalog c2;
    c2.emplace<Book>("QB1", "Quota Book", std::vector<std::string>{"A"},
                     "ISBN-Q", "Publisher", 2024, Money::of(100));
    Budget b2(Money::of(1000));
    b2.setQuota(ResourceCategory::Book, {10, Money::of(215)});
    AcquisitionManager a2(c2, b2);
    a2.setTaxRates(10, 20);
    std::string reason;
    assert(!a2.canPurchase("QB1", 2, &reason));;
    assert(reason.find("220.00") != std::string::npos);;
    assertThrows<QuotaExceededError>([&]() { (void)(a2.purchase("QB1", 2)); });;
    assert(b2.spent() == Money{});;
    assert(c2.holdings("QB1") == 0);;
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

    assert(budget.titleLimitFor(ResourceCategory::Book).value() == 2);;
    assert(budget.distinctTitlesUsed(ResourceCategory::Book) == 0);;

    // Different resource IDs with the same title count as one title.
    assert(acq.purchase("TL1", 3).approved);;
    assert(acq.purchase("TL2", 2).approved);;
    assert(budget.distinctTitlesUsed(ResourceCategory::Book) == 1);;

    // A genuinely different title uses the second title slot.
    assert(acq.purchase("TL3", 1).approved);;
    assert(budget.distinctTitlesUsed(ResourceCategory::Book) == 2);;

    // Another title is rejected even though unit and spending quota remain.
    std::string reason;
    assert(!acq.canPurchase("TL4", 1, &reason));;
    assert(reason.find("distinct-title limit") != std::string::npos);;
    assertThrows<QuotaExceededError>([&]() { (void)(acq.purchase("TL4", 1)); });;
    assert(catalog.holdings("TL4") == 0);;
    assert(budget.distinctTitlesUsed(ResourceCategory::Book) == 2);;

    // A rejected batch item is recorded but doesn't consume a title slot.
    const auto before = acq.history().size();
    auto result = acq.processBatch({{"TL4", 1}});
    assert(result.size() == 1);;
    assert(!result[0].approved);;
    assert(result[0].reason.find("distinct-title limit") != std::string::npos);;
    assert(acq.history().size() == before + 1);;
    assert(budget.distinctTitlesUsed(ResourceCategory::Book) == 2);;

    // Title limits are category-specific.
    assert(acq.purchase("TL5", 1).approved);;
    assert(budget.distinctTitlesUsed(ResourceCategory::EBook) == 0);;
    assert(budget.distinctTitlesUsed(ResourceCategory::ElectronicResource) == 1);;

    assertThrows<std::invalid_argument>([&]() { (void)(budget.setTitleLimit(ResourceCategory::Book, -1)); });;

    budget.removeTitleLimit(ResourceCategory::Book);
    assert(!budget.titleLimitFor(ResourceCategory::Book).has_value());;
    assert(acq.purchase("TL4", 1).approved);;
    assert(budget.distinctTitlesUsed(ResourceCategory::Book) == 3);;
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
    assert(b.spent() == Money::of(220));;
    assert(c.holdings("CX1") == 2);;

    const PurchaseRecord cancelled = acq.cancel(1);
    assert(cancelled.isCancellation);;
    assert(cancelled.cancelsOrderNo == 1);;
    assert(cancelled.quantity == -2);;
    assert(cancelled.preTaxCost == Money::of(-200));;
    assert(cancelled.cost == Money::of(-220));;
    assert(acq.history().size() == 2);;
    assert(!acq.history()[0].isCancellation);;
    assert(b.spent() == Money{});;
    assert(b.usageFor(ResourceCategory::Book).units == 0);;
    assert(c.holdings("CX1") == 0);;
    assert(acq.totalPreTax() == Money{});;
    assert(acq.totalSpent() == Money{});;
    assertThrows<std::invalid_argument>([&]() { (void)(acq.cancel(1)); });;
}
static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    assert(c.size() == 3);;
    assert(c.contains("B1"));;
    assert(c.find("nope") == nullptr);;
    assertThrows<NotFoundError>([&]() { (void)(c.get("nope")); });;
    assertThrows<DuplicateIdError>([&]() { (void)(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1))); });;

    assert(c.searchTitle("clean").size() == 2);;
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);;
    assert(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);;

    assert(c.holdings("B1") == 0);;
    c.addHoldings("B1", 3);
    assert(c.holdings("B1") == 3);;
    assertThrows<std::invalid_argument>([&]() { (void)(c.addHoldings("B1", -5)); });;

    c.remove("R1");
    assert(c.size() == 2);;
    assertThrows<NotFoundError>([&]() { (void)(c.remove("R1")); });;
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    assert(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());;
    assert(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());;   // units
    assert(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());;  // spend
    assert(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());;  // overall
    assert(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());;    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert(b.spent() == Money::of(300));;
    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);;
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));;
    assert(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());;

    assertThrows<QuotaExceededError>([&]() { (void)(b.commit(ResourceCategory::Book, 2, Money::of(10))); });;
    assertThrows<BudgetExceededError>([&]() { (void)(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800))); });;
    assertThrows<std::invalid_argument>([&]() { (void)(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1))); });;
    assert(b.spent() == Money::of(300));;  // failed commits changed nothing
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

    assert(acq.quote("R1", 5) == Money::of(100));;
    std::string why;
    assert(acq.canPurchase("B1", 3, &why) && why.empty());;
    assert(!acq.canPurchase("B1", 4, &why) && !why.empty());;
    assert(!acq.canPurchase("nope", 1, &why));;

    const auto& rec = acq.purchase("B1", 2);
    assert(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);;
    assert(c.holdings("B1") == 2);;

    assertThrows<QuotaExceededError>([&]() { (void)(acq.purchase("B1", 2)); });;
    assertThrows<NotFoundError>([&]() { (void)(acq.purchase("nope", 1)); });;
    assert(acq.history().size() == 1);;  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert(res.size() == 5);;
    assert(res[0].approved && res[0].cost == Money::of(150));;
    assert(!res[1].approved);;  // 1050 > remaining 150
    assert(res[2].approved);;
    assert(!res[3].approved && res[3].reason.find("not found") != std::string::npos);;
    assert(!res[4].approved);;
    assert(acq.totalSpent() == Money::of(450));;
    assert(b.spent() == acq.totalSpent());;
    assert(c.holdings("R1") == 10 && c.holdings("B1") == 3);;
    assert(acq.history().size() == 6);;
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
    assert(acq.canPurchase("D1", 1, "Computer Science", &why));;

    const int csOrderNo =
        acq.purchase("D1", 1, "Computer Science").orderNo;
    assert(cs.spent() == Money::of(100));;
    assert(acq.history().back().department == "Computer Science");;
    assert(!acq.canPurchase("D1", 1, "Computer Science", &why));;

    auto results = acq.processBatch({
        {"D1", 1, "Physics"},
        {"D1", 1, "Physics"},
        {"D1", 1, "Unknown Department"}
    });

    assert(results.size() == 3);;
    assert(results[0].approved && results[0].department == "Physics");;
    assert(results[1].approved && results[1].department == "Physics");;
    assert(!results[2].approved);;
    assert(results[2].reason.find("unknown department") != std::string::npos);;
    assert(physics.spent() == Money::of(200));;
    assert(defaultBudget.spent() == Money{});;
    assert(!acq.canPurchase("D1", 1, "Physics", &why));;

    const auto& cancelled = acq.cancel(csOrderNo);
    assert(cancelled.isCancellation);;
    assert(cancelled.department == "Computer Science");;
    assert(cs.spent() == Money{});;
    assert(physics.spent() == Money::of(200));;
    assert(acq.canPurchase("D1", 1, "Computer Science", &why));;
    assert(c.holdings("D1") == 2);;
}


static void testBudgetRollover() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(500)});
    b.setTitleLimit(ResourceCategory::Book, 3);
    b.commit(ResourceCategory::Book, 2, Money::of(200), "Used Title");

    Budget half = b.nextYear(50);
    assert(half.total() == Money::of(1400));;
    assert(half.spent() == Money{});;
    assert(half.remaining() == Money::of(1400));;
    assert(half.quotaFor(ResourceCategory::Book)->maxUnits == 10);;
    assert(half.quotaFor(ResourceCategory::Book)->maxSpend == Money::of(500));;
    assert(half.usageFor(ResourceCategory::Book).units == 0);;
    assert(half.usageFor(ResourceCategory::Book).spent == Money{});;
    assert(half.titleLimitFor(ResourceCategory::Book).value() == 3);;
    assert(half.distinctTitlesUsed(ResourceCategory::Book) == 0);;
    assert(b.nextYear(0).total() == Money::of(1000));;
    assert(b.nextYear(100).total() == Money::of(1800));;
    assertThrows<std::invalid_argument>([&]() { (void)(b.nextYear(-1)); });;
    assertThrows<std::invalid_argument>([&]() { (void)(b.nextYear(101)); });;
}


static void testBatchAllOrNothing() {
    Catalog c;
    c.emplace<Book>("AO1", "Atomic Book One",
                    std::vector<std::string>{"A"}, "isbn1", "P", 2024,
                    Money::of(100));
    c.emplace<Book>("AO2", "Atomic Book Two",
                    std::vector<std::string>{"B"}, "isbn2", "P", 2024,
                    Money::of(100));

    Budget b(Money::of(250));
    b.setQuota(ResourceCategory::Book, {3, Money::of(250)});
    AcquisitionManager acq(c, b);

    // First request fits, but the second would exceed the remaining budget.
    auto rejected = acq.processBatch({{"AO1", 1}, {"AO2", 2}}, true);
    assert(rejected.size() == 2);;
    assert(!rejected[0].approved && !rejected[1].approved);;
    assert(rejected[0].reason.find("batch aborted") != std::string::npos);;
    assert(!rejected[1].reason.empty());;
    assert(b.spent() == Money{});;
    assert(b.usageFor(ResourceCategory::Book).units == 0);;
    assert(c.holdings("AO1") == 0);;
    assert(c.holdings("AO2") == 0);;

    // A valid batch approves both requests.
    auto approved = acq.processBatch({{"AO1", 1}, {"AO2", 1}}, true);
    assert(approved.size() == 2);;
    assert(approved[0].approved && approved[1].approved);;
    assert(b.spent() == Money::of(200));;
    assert(b.usageFor(ResourceCategory::Book).units == 2);;
    assert(c.holdings("AO1") == 1);;
    assert(c.holdings("AO2") == 1);;
    assert(acq.history().size() == 4);;
}


static void testVendors() {
    Catalog c;
    c.emplace<Book>("V1", "Vendor Choice", std::vector<std::string>{"A"},
                    "isbn", "P", 2025, Money::of(150), 1, Binding::Hardcover);

    Budget b(Money::of(1000));
    AcquisitionManager acq(c, b);
    acq.setTaxRates(10, 18);

    acq.addVendorOffer("V1", "Vendor A", Money::of(100));
    acq.addVendorOffer("V1", "Vendor B", Money::of(80));

    assert(acq.vendorOffersFor("V1").size() == 2);;
    assert(acq.quotePreTax("V1", 1) == Money::of(96));;
    assert(acq.quote("V1", 1) == Money::fromMinor(10560));;
    assert(c.get("V1").unitPrice() == Money::of(150));;

    std::string why;
    assert(acq.canPurchase("V1", 1, &why) && why.empty());;

    const auto& order = acq.purchase("V1", 1);
    const int orderNo = order.orderNo;
    assert(order.approved);;
    assert(order.vendor == "Vendor B");;
    assert(order.preTaxCost == Money::of(96));;
    assert(order.cost == Money::fromMinor(10560));;
    assert(b.spent() == Money::fromMinor(10560));;
    assert(c.holdings("V1") == 1);;
    assert(c.get("V1").unitPrice() == Money::of(150));;

    const auto& cancelled = acq.cancel(orderNo);
    assert(cancelled.isCancellation);;
    assert(cancelled.vendor == "Vendor B");;
    assert(b.spent() == Money{});;
    assert(c.holdings("V1") == 0);;

    // Adding the same vendor updates its offer instead of adding a duplicate.
    acq.addVendorOffer("V1", "Vendor B", Money::of(70));
    assert(acq.vendorOffersFor("V1").size() == 2);;
    assert(acq.quotePreTax("V1", 1) == Money::of(84));;

    // Batch purchases use the same cheapest-vendor calculation.
    auto batch = acq.processBatch({{"V1", 1}}, true);
    assert(batch.size() == 1 && batch[0].approved);;
    assert(batch[0].vendor == "Vendor B");;
    assert(batch[0].preTaxCost == Money::of(84));;
    assert(batch[0].cost == Money::fromMinor(9240));;

    assertThrows<NotFoundError>([&]() { (void)(acq.addVendorOffer("missing", "Vendor", Money::of(1))); });;
    assertThrows<std::invalid_argument>([&]() { (void)(acq.addVendorOffer("V1", "", Money::of(1))); });;
    assertThrows<std::invalid_argument>([&]() { (void)(acq.addVendorOffer("V1", "Negative", Money::fromMinor(-1))); });;
}


static void testCatalogAdvancedSearch() {
    Catalog c;
    c.emplace<Book>("S1", "Search Book", std::vector<std::string>{"Jane Doe", "Alex Morgan"}, "ISBN-001", "Pub", 2018, Money::of(20));
    c.emplace<EBook>("S2", "Digital Search", std::vector<std::string>{"Alex Morgan"}, "ISBN-002", "Pub", 2022, Money::of(10), "url", EBookFormat::PDF, false);
    c.emplace<Journal>("S3", "Search Journal", "ISSN-003", 12, "Pub", 2020, Money::of(30));
    c.emplace<AudioBook>("S4", "Audio Title", "Alex Narrator", 100, "Pub", 2021, Money::of(10), "url");

    assert(c.searchAuthor("aLeX").size() == 2);;
    assert(c.searchAuthor("morgan").size() == 2);;
    assert(c.searchAuthor("narrator").empty());;
    assert(c.searchAuthor("").empty());;

    assert(c.searchIsbnIssn("isbn-001").size() == 1);;
    assert(c.searchIsbnIssn("ISBN-002").size() == 1);;
    assert(c.searchIsbnIssn("issn-003").size() == 1);;
    assert(c.searchIsbnIssn("003").empty());;
    assert(c.searchIsbnIssn("").empty());;

    auto years = c.searchYearRange(2020, 2022);
    assert(years.size() == 3);;
    assert(years[0]->year() == 2022);;
    assert(years[1]->year() == 2020);;
    assert(years[2]->year() == 2021);;
    assert(c.searchYearRange(2022, 2022).size() == 1);;
    assert(c.searchYearRange(2023, 2024).empty());;
    assertThrows<std::invalid_argument>([&]() { (void)(c.searchYearRange(2023, 2022)); });;
}


static void testLending() {
    Catalog c;
    c.emplace<Book>("L1", "Lending Book", std::vector<std::string>{"Author"},
                    "ISBN-L1", "Publisher", 2024, Money::of(100));
    c.emplace<ElectronicResource>(
        "L2", "Licensed Database", "Publisher", 2024, Money::of(10),
        "url", LicenseModel::Perpetual);

    c.addHoldings("L1", 2);
    c.addHoldings("L2", 2);

    LendingManager lending(c);

    assert(lending.availableCopies("L1") == 2);;
    assert(lending.borrowedCopies("L1") == 0);;

    lending.borrow("L1", "Patron-A");
    assert(lending.borrowedCopies("L1") == 1);;
    assert(lending.availableCopies("L1") == 1);;
    lending.borrow("L1", "Patron-B");
    assert(lending.availableCopies("L1") == 0);;

    assertThrows<std::invalid_argument>([&]() { (void)(lending.borrow("L1", "Patron-C")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.borrow("L1", "Patron-A")); });;

    lending.returnCopy("L1", "Patron-A");
    assert(lending.availableCopies("L1") == 1);;
    assert(lending.borrowedCopies("L1") == 1);;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.returnCopy("L1", "Patron-A")); });;

    assertThrows<std::invalid_argument>([&]() { (void)(lending.borrow("L2", "Patron-D")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.availableSeats("L1")); });;

    assert(lending.availableSeats("L2") == 2);;
    assert(lending.activeSessions("L2") == 0);;
    lending.openSession("L2", "Patron-A");
    lending.openSession("L2", "Patron-B");
    assert(lending.activeSessions("L2") == 2);;
    assert(lending.availableSeats("L2") == 0);;

    assertThrows<std::invalid_argument>([&]() { (void)(lending.openSession("L2", "Patron-C")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.openSession("L2", "Patron-A")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.openSession("L1", "Patron-C")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.availableCopies("L2")); });;

    lending.closeSession("L2", "Patron-A");
    assert(lending.activeSessions("L2") == 1);;
    assert(lending.availableSeats("L2") == 1);;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.closeSession("L2", "Patron-A")); });;

    lending.closeSession("L2", "Patron-B");
    assert(lending.availableSeats("L2") == 2);;
    assert(lending.borrowedCopies("L1") == 1);;

    assertThrows<NotFoundError>([&]() { (void)(lending.borrow("missing", "Patron-A")); });;
    assertThrows<NotFoundError>([&]() { (void)(lending.openSession("missing", "Patron-A")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.borrow("L1", "")); });;
    assertThrows<std::invalid_argument>([&]() { (void)(lending.openSession("L2", "")); });;
}


static void testMoneyCurrencies() {
    using bookmgmt::Money;

    const Money inr = Money::of(100, "INR");
    const Money usd = Money::of(100, "USD");

    assert(inr.currencyCode() == "INR");;
    assert(usd.currencyCode() == "USD");;

    assert((Money::of(10, "INR") + Money::of(5, "INR")).minorUnits() == 1500);;
    assert((Money::of(10, "INR") - Money::of(5, "INR")).minorUnits() == 500);;

    assertThrows<std::invalid_argument>([&]() { (void)(inr + usd); });;
    assertThrows<std::invalid_argument>([&]() { (void)(inr - usd); });;

    assertThrows<std::invalid_argument>([&]() { (void)(inr == usd); });;
    assertThrows<std::invalid_argument>([&]() { (void)(inr != usd); });;
    assertThrows<std::invalid_argument>([&]() { (void)(inr < usd); });;
    assertThrows<std::invalid_argument>([&]() { (void)(inr <= usd); });;
    assertThrows<std::invalid_argument>([&]() { (void)(inr > usd); });;
    assertThrows<std::invalid_argument>([&]() { (void)(inr >= usd); });;
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
    testBudgetRollover();
    testBatchAllOrNothing();
    testVendors();
    testCatalogAdvancedSearch();
    testLending();
    testMoneyCurrencies();
    std::cout << "All tests passed\n";
    return 0;
}
