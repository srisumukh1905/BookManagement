#include "bookmgmt/Money.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Money::Money(std::int64_t minor, std::string currencyCode)
    : minor_(minor), currencyCode_(std::move(currencyCode)) {
    if (currencyCode_.empty()) {
        throw std::invalid_argument("currency code cannot be empty");
    }
}

Money Money::fromMinor(std::int64_t minor, std::string currencyCode) {
    return Money(minor, std::move(currencyCode));
}

Money Money::of(std::int64_t major, int minor, std::string currencyCode) {
    if (minor < 0 || minor > 99) {
        throw std::invalid_argument("minor part must be in 0..99");
    }
    const std::int64_t sign = major < 0 ? -1 : 1;
    return Money(major * 100 + sign * minor, std::move(currencyCode));
}

Money Money::of(std::int64_t major, std::string currencyCode) {
    return of(major, 0, std::move(currencyCode));
}

void Money::requireSameCurrency(const Money& other) const {
    if (currencyCode_ != other.currencyCode_) {
        throw std::invalid_argument(
            "currency mismatch: " + currencyCode_ + " and " +
            other.currencyCode_);
    }
}

Money& Money::operator+=(Money other) {
    requireSameCurrency(other);
    minor_ += other.minor_;
    return *this;
}

Money& Money::operator-=(Money other) {
    requireSameCurrency(other);
    minor_ -= other.minor_;
    return *this;
}

bool operator==(Money a, Money b) {
    a.requireSameCurrency(b);
    return a.minor_ == b.minor_;
}

bool operator<(Money a, Money b) {
    a.requireSameCurrency(b);
    return a.minor_ < b.minor_;
}

bool operator<=(Money a, Money b) {
    a.requireSameCurrency(b);
    return a.minor_ <= b.minor_;
}

std::string Money::toString() const {
    const std::int64_t absMinor = minor_ < 0 ? -minor_ : minor_;
    std::string s = std::to_string(absMinor / 100);
    const auto frac = absMinor % 100;
    s += '.';
    if (frac < 10) s += '0';
    s += std::to_string(frac);
    return minor_ < 0 ? "-" + s : s;
}

std::ostream& operator<<(std::ostream& os, Money m) {
    return os << m.toString();
}

}  // namespace bookmgmt
