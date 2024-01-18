// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "lock.h"
#include <stdexcept>

/*
 * Lock
 */

Lock::operator bool() const { return isLocked(); }

// Increase lock count
void Lock::addLockLevel() { ++lockCounter_; }

// Decrease lock count
void Lock::removeLockLevel()
{
    if (lockCounter_ == 0)
        throw(std::runtime_error("Lock count is fully unlocked - it cannot be unlocked further.\n"));

    --lockCounter_;
}

// Return whether we are currently locked
bool Lock::isLocked() const { return lockCounter_ > 0; }

/*
 * Locker
 */

Locker::Locker(Lock &lock) : lock_(lock)
{
    lock_.addLockLevel();

    unlocked_ = false;
}

Locker::~Locker()
{
    if (!unlocked_)
        lock_.removeLockLevel();
}

// Manually release the lock
void Locker::unlock()
{
    if (!unlocked_)
        lock_.removeLockLevel();

    unlocked_ = true;
}
