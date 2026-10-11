#pragma once

#include <map>
#include <set>
#include <string>

#include "bookmgmt/Catalog.h"

namespace bookmgmt {

// Tracks borrowed print copies and active sessions on electronic resources.
// Catalogue holdings represent total copies/seats owned or licensed.
class LendingManager {
public:
    explicit LendingManager(Catalog& catalog);

    void borrow(const std::string& resourceId, const std::string& patronId);
    void returnCopy(const std::string& resourceId, const std::string& patronId);

    void openSession(const std::string& resourceId, const std::string& patronId);
    void closeSession(const std::string& resourceId, const std::string& patronId);

    int borrowedCopies(const std::string& resourceId) const;
    int activeSessions(const std::string& resourceId) const;
    int availableCopies(const std::string& resourceId) const;
    int availableSeats(const std::string& resourceId) const;

private:
    const Resource& requireResource(const std::string& resourceId) const;
    static void requirePatron(const std::string& patronId);

    Catalog& catalog_;
    std::map<std::string, std::set<std::string>> borrowedByResource_;
    std::map<std::string, std::set<std::string>> sessionsByResource_;
};

}  // namespace bookmgmt
