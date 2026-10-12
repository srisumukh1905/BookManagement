// Roll No: MT26213 | Name: Srisumukh
#include "bookmgmt/MT26213_Journal.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string issn,
                 int issuesPerYear, std::string publisher, int year,
                 Money unitPrice, int subscriptionYears)
    : Resource(std::move(id), std::move(title), std::move(publisher),
               year, unitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (issuesPerYear_ < 1) {
        throw std::invalid_argument("issues per year must be >= 1");
    }

    if (subscriptionYears_ < 1) {
        throw std::invalid_argument("subscription years must be >= 1");
    }
}

Money Journal::costFor(int copies) const {
    return costForAtPrice(copies, unitPrice());
}

Money Journal::costForAtPrice(int copies, Money offeredUnitPrice) const {
    requirePositive(copies);
    if (offeredUnitPrice.isNegative())
        throw std::invalid_argument("price must not be negative");

    const Money totalCost = offeredUnitPrice * copies * subscriptionYears_;
    return applyPrintBulkDiscount(totalCost, copies);
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt
