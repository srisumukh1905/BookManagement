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

## Verification
Build with `cmake --build build`.
Run tests with `./build/bookmgmt_tests`.
The current test suite passes 230/230 checks.

This file will be updated as Q10–Q16 are implemented.
