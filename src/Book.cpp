#include "bookmgmt/Book.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

std::string joinAuthors(const std::vector<std::string>& authors) {
    std::string out;
    for (std::size_t i = 0; i < authors.size(); ++i) {
        if (i > 0) out += (i + 1 == authors.size()) ? " and " : ", ";
        out += authors[i];
    }
    return out;
}

Book::Book(std::string id, std::string title, std::vector<std::string> authors,
           std::string isbn, std::string publisher, int year, Money unitPrice,
           int edition, Binding binding)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      edition_(edition),
      binding_(binding) {
    if (edition_ < 1) throw std::invalid_argument("edition must be >= 1");
}

Money Book::costFor(int quantity) const {
    return costForAtPrice(quantity, unitPrice());
}

Money Book::costForAtPrice(int quantity, Money offeredUnitPrice) const {
    requirePositive(quantity);
    if (offeredUnitPrice.isNegative())
        throw std::invalid_argument("price must not be negative");

    Money totalCost = offeredUnitPrice * quantity;

    if (binding_ == Binding::Hardcover) {
        // Apply the 20% hardcover premium before any bulk discount.
        const std::int64_t minor = totalCost.minorUnits();
        const std::int64_t increasedMinor =
            (minor / 100) * 120 + ((minor % 100) * 120 + 50) / 100;
        totalCost = Money::fromMinor(increasedMinor);
    }

    return applyPrintBulkDiscount(totalCost, quantity);
}

void Book::printDetails(std::ostream& os) const {
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  edition: " << edition_ << "\n"
       << "  binding: " << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback")
       << "\n";
}

}  // namespace bookmgmt
