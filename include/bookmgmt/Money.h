#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    static Money fromMinor(std::int64_t minor, std::string currencyCode = "INR");
    static Money of(std::int64_t major, int minor = 0,
                    std::string currencyCode = "INR");
    static Money of(std::int64_t major, std::string currencyCode);

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currencyCode() const { return currencyCode_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(Money o);
    Money& operator-=(Money o);
    Money& operator*=(std::int64_t k) {
        minor_ *= k;
        return *this;
    }

    friend Money operator+(Money a, Money b) { return a += b; }
    friend Money operator-(Money a, Money b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend bool operator==(Money a, Money b);
    friend bool operator!=(Money a, Money b) { return !(a == b); }
    friend bool operator<(Money a, Money b);
    friend bool operator<=(Money a, Money b);
    friend bool operator>(Money a, Money b) { return b < a; }
    friend bool operator>=(Money a, Money b) { return b <= a; }

private:
    Money(std::int64_t minor, std::string currencyCode);
    void requireSameCurrency(const Money& other) const;

    std::int64_t minor_ = 0;
    std::string currencyCode_ = "INR";
};

std::ostream& operator<<(std::ostream& os, Money m);

}  // namespace bookmgmt
