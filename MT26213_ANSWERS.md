# OOPD Assignment 2 — Answers

## Q1 — Journal
Implemented Journal resources with journal-specific metadata, subscription pricing, and category support.

## Q2 — EBook
Implemented EBook resources with format and DRM information, using the electronic-resource pricing model.

## Q3 — AudioBook and Thesis
Implemented AudioBook and Thesis resource types with their respective metadata and category support.

## Q4 — Hardcover pricing
Hardcover books cost 20% more than the base book price.

## Q5 — Bulk discounts
Implemented the required print bulk discount for orders of at least 10 copies and half-price electronic seats beyond 50.

## Q6 — Taxes
Added configurable print and electronic tax rates. Budget checks use post-tax purchase costs.

## Q7 — Distinct-title limits
Added configurable limits on the number of distinct titles purchased within each resource category.

## Q8 — Order cancellation
Added cancellation records, post-tax refunds, holdings reversal, and protection against cancelling an order more than once.

## Q9 — Department budgets and quotas
Added independent department budgets and quotas. Purchase and batch requests identify the charged department, and cancellation refunds return to that department.

## Q10 — Year-end budget rollover
Added `Budget::nextYear(rolloverPercent)`. The next year's budget retains
the current base allocation and adds the configured percentage of the
current year's unspent amount. Quota and title-limit settings carry forward,
while spending, category usage, and title usage reset.

The rollover percentage must be between 0 and 100 inclusive.

## Q11 — All-or-nothing batch processing
Added an optional `allOrNothing` argument to `processBatch()`. Its default
is `false`, preserving the existing sequential behavior. When enabled, the
manager preflights the entire batch against simulated copies of the budgets.
If any request fails, no purchases are made; otherwise all requests execute.

## Q12 — Vendors
Added vendor offers per catalogue resource. The acquisition manager selects
the cheapest vendor, applies the resource's existing pricing rules, and
records the chosen vendor in purchase and cancellation history. Resources
without vendor offers retain their catalogue pricing.

## Q13 — Catalogue searches
Added case-insensitive author searches across books and e-books, ISBN/ISSN
identifier searches across books, e-books and journals, and inclusive
publication-year range searches. Reversed year ranges are rejected.

## Q14 — Lending
Added a `LendingManager` that tracks borrowed print copies and active
electronic-resource sessions by patron. Borrowing and session opening are
limited by catalogue holdings; returns and session closure release capacity.
Invalid operations, duplicate active use, and unknown resources are rejected.

## Q15 — Currency-aware Money
Extended `Money` with a currency code, defaulting to INR for compatibility
with existing code. Addition, subtraction, and comparisons require matching
currency codes and throw `std::invalid_argument` when currencies differ.
Added tests and a demo for currency-aware amounts and cross-currency rejection.

## Q16 — Standard assertions
Replaced the custom `CHECK` and `CHECK_THROWS` test macros with standard C++ `assert` assertions. Added an `assertThrows` helper to verify that expected exception types are thrown, preserving the exception-checking behavior.

## Verification
Build with `cmake --build build`.
Run tests with `./build/bookmgmt_tests`.
The build succeeds and all tests pass.


AI prompts
- Help me complete this assignment step by step according to the given requirements. Check my existing code before making changes, and help me implement each feature without breaking the functionality that is already working.
- Getting errors or unexpected results in the code. Help me understand what is causing the problem and fix it without unnecessarily changing the rest of the implementation.
- Help me test the functionality I have implemented. Check normal cases, boundary conditions, and invalid inputs, and help me fix any issues found during testing.
- Review the implementation against the assignment requirements. Check whether the different features work together correctly, whether the tests pass, and whether anything important is missing.
- Help me update the assignment documentation to match the final implementation. Check the project status, verify the tests, organize the completed work, and prepare the final submission.
