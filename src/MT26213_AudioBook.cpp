// Roll No: MT26213 | Name: Srisumukh
#include "bookmgmt/MT26213_AudioBook.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

AudioBook::AudioBook(std::string id, std::string title,
                     std::string narrator, int durationMinutes,
                     std::string publisher, int year,
                     Money pricePerSeat, std::string accessUrl,
                     LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title),
                         std::move(publisher), year, pricePerSeat,
                         std::move(accessUrl), license, platformFee),
      narrator_(std::move(narrator)),
      durationMinutes_(durationMinutes) {
    if (durationMinutes_ <= 0) {
        throw std::invalid_argument("audio duration must be positive");
    }
}

void AudioBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);
    os << "  narrator: " << narrator_ << "\n"
       << "  duration (minutes): " << durationMinutes_ << "\n";
}

}  // namespace bookmgmt
