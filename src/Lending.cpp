#include "bookmgmt/Lending.h"

#include <stdexcept>

namespace bookmgmt {

LendingManager::LendingManager(Catalog& catalog) : catalog_(catalog) {}

const Resource& LendingManager::requireResource(
    const std::string& resourceId) const {
    return catalog_.get(resourceId);
}

void LendingManager::requirePatron(const std::string& patronId) {
    if (patronId.empty()) {
        throw std::invalid_argument("patron ID cannot be empty");
    }
}

void LendingManager::borrow(
    const std::string& resourceId, const std::string& patronId) {
    const Resource& resource = requireResource(resourceId);
    requirePatron(patronId);

    if (resource.isDigital()) {
        throw std::invalid_argument("electronic resources use sessions, not borrowing");
    }

    auto& patrons = borrowedByResource_[resourceId];
    if (patrons.count(patronId) != 0) {
        throw std::invalid_argument("patron already borrowed this resource");
    }
    if (static_cast<int>(patrons.size()) >= catalog_.holdings(resourceId)) {
        throw std::invalid_argument("no available print copies");
    }

    patrons.insert(patronId);
}

void LendingManager::returnCopy(
    const std::string& resourceId, const std::string& patronId) {
    const Resource& resource = requireResource(resourceId);
    requirePatron(patronId);

    if (resource.isDigital()) {
        throw std::invalid_argument("electronic resources use sessions, not returns");
    }

    auto it = borrowedByResource_.find(resourceId);
    if (it == borrowedByResource_.end() ||
        it->second.erase(patronId) == 0) {
        throw std::invalid_argument("patron has no borrowed copy of this resource");
    }

    if (it->second.empty()) borrowedByResource_.erase(it);
}

int LendingManager::borrowedCopies(const std::string& resourceId) const {
    (void)requireResource(resourceId);
    const auto it = borrowedByResource_.find(resourceId);
    return it == borrowedByResource_.end()
        ? 0 : static_cast<int>(it->second.size());
}

int LendingManager::availableCopies(const std::string& resourceId) const {
    const Resource& resource = requireResource(resourceId);
    if (resource.isDigital()) {
        throw std::invalid_argument("resource is electronic, not a print item");
    }
    return catalog_.holdings(resourceId) - borrowedCopies(resourceId);
}

void LendingManager::openSession(
    const std::string& resourceId, const std::string& patronId) {
    const Resource& resource = requireResource(resourceId);
    requirePatron(patronId);

    if (!resource.isDigital()) {
        throw std::invalid_argument("print resources use borrowing, not sessions");
    }

    auto& patrons = sessionsByResource_[resourceId];
    if (patrons.count(patronId) != 0) {
        throw std::invalid_argument("patron already has an active session");
    }
    if (static_cast<int>(patrons.size()) >= catalog_.holdings(resourceId)) {
        throw std::invalid_argument("no available electronic seats");
    }

    patrons.insert(patronId);
}

void LendingManager::closeSession(
    const std::string& resourceId, const std::string& patronId) {
    const Resource& resource = requireResource(resourceId);
    requirePatron(patronId);

    if (!resource.isDigital()) {
        throw std::invalid_argument("print resources do not use sessions");
    }

    auto it = sessionsByResource_.find(resourceId);
    if (it == sessionsByResource_.end() ||
        it->second.erase(patronId) == 0) {
        throw std::invalid_argument("patron has no active session");
    }

    if (it->second.empty()) sessionsByResource_.erase(it);
}

int LendingManager::activeSessions(const std::string& resourceId) const {
    (void)requireResource(resourceId);
    const auto it = sessionsByResource_.find(resourceId);
    return it == sessionsByResource_.end()
        ? 0 : static_cast<int>(it->second.size());
}

int LendingManager::availableSeats(const std::string& resourceId) const {
    const Resource& resource = requireResource(resourceId);
    if (!resource.isDigital()) {
        throw std::invalid_argument("resource is print, not electronic");
    }
    return catalog_.holdings(resourceId) - activeSessions(resourceId);
}

}  // namespace bookmgmt
