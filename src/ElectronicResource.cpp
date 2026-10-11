#include "bookmgmt/ElectronicResource.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

const char* licenseName(LicenseModel m) {
    switch (m) {
        case LicenseModel::Perpetual: return "Perpetual";
        case LicenseModel::AnnualSubscription: return "Annual subscription";
    }
    return "Unknown";
}

ElectronicResource::ElectronicResource(std::string id, std::string title,
                                       std::string publisher, int year,
                                       Money pricePerSeat, std::string accessUrl,
                                       LicenseModel license, Money platformFee)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, pricePerSeat),
      accessUrl_(std::move(accessUrl)),
      license_(license),
      platformFee_(platformFee) {
    if (platformFee_.isNegative())
        throw std::invalid_argument("platform fee must not be negative");
}

Money ElectronicResource::costFor(int seats) const {
    return costForAtPrice(seats, unitPrice());
}

Money ElectronicResource::costForAtPrice(
    int seats, Money offeredUnitPrice) const {
    requirePositive(seats);
    if (offeredUnitPrice.isNegative())
        throw std::invalid_argument("price must not be negative");

    const int regularSeats = std::min(seats, 50);
    const int discountedSeats = seats - regularSeats;
    const std::int64_t unitMinor = offeredUnitPrice.minorUnits();
    const std::int64_t extraCount = discountedSeats;

    // Seats after the 50th cost half price. Round the total half-price
    // component to the nearest minor currency unit.
    const std::int64_t discountedMinor =
        (unitMinor / 2) * extraCount +
        ((unitMinor % 2) * extraCount + 1) / 2;

    return platformFee_ + offeredUnitPrice * regularSeats +
           Money::fromMinor(discountedMinor);
}

void ElectronicResource::printDetails(std::ostream& os) const {
    os << "  access url: " << accessUrl_ << "\n"
       << "  license: " << licenseName(license_) << "\n"
       << "  platform fee: " << platformFee_ << "\n";
}

}  // namespace bookmgmt
