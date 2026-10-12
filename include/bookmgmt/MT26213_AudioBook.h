// Roll No: MT26213 | Name: Srisumukh
#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

// AudioBook uses ElectronicResource because audio content can be
// licensed digitally, with the existing per-seat pricing model.
class AudioBook : public ElectronicResource {
public:
    AudioBook(std::string id, std::string title, std::string narrator,
              int durationMinutes, std::string publisher, int year,
              Money pricePerSeat, std::string accessUrl,
              LicenseModel license = LicenseModel::AnnualSubscription,
              Money platformFee = Money{});

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

    ResourceCategory category() const override {
        return ResourceCategory::AudioBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

}  // namespace bookmgmt
