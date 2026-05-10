/*
 * This file is part of plptools.
 *
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
 *  Copyright (c) 2026 Tom Sutcliffe
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "config.h"

#include <cassert>
#include <utility>


template<typename T>
class Optional {
public:

    Optional()
    : hasValue_(false) {}

    Optional(const T& value)
    : hasValue_(true) {
        *ptr() = value;
    }

    Optional(T&& value)
    : hasValue_(true) {
        *ptr() = std::move(value);
    }

    ~Optional() {
        reset();
    }

    Optional(const Optional& other)
    : hasValue_(other.hasValue_) {
        if (!hasValue_) {
            return;
        }
        new(ptr()) T(*other.ptr());

    }

    Optional(Optional&& other)
    : hasValue_(other.hasValue_) {
        if (!hasValue_) {
            return;
        }
        new(ptr()) T(std::move(*other.ptr()));
        other.hasValue_ = false;
    }

    Optional& operator=(const Optional& other) {

        reset();

        if (!other.hasValue_) {
            return *this;
        }
        
        new(ptr()) T(*other.ptr());
        hasValue_ = true;
        
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept {

        reset();

        if (!other.hasValue_) {
            return *this;
        }

        new(ptr()) T(std::move(*other.ptr()));
        hasValue_ = true;
        other.hasValue_ = false;
        
        return *this;
    }

    Optional& operator=(const T& value) {

        reset();

        new(ptr()) T(value);
        hasValue_ = true;

        return *this;
    }

    Optional& operator=(const T&& value) {

        reset();

        new(ptr()) T(std::move(value));
        hasValue_ = true;

        return *this;
    }

    T& value() {
        assert(hasValue_);
        return *ptr();
    }

    const T& value() const {
        assert(hasValue_);
        return *ptr();
    }

    bool hasValue() const {
        return hasValue_;
    }

    void reset() {
        if (hasValue_) {
            ptr()->~T();
            hasValue_ = false;
        }
    }

    T& operator*() {
        return value();
    }

    const T& operator*() const {
        return value();
    }

    T* operator->() {
        return &value();
    }

    const T* operator->() const {
        return &value();
    }

    explicit operator bool() const {
        return hasValue_;
    }
    
private:

    T *ptr() {
        return reinterpret_cast<T*>(value_);
    }

    const T *ptr() const {
        return reinterpret_cast<const T*>(value_);
    }
    
    alignas(T) unsigned char value_[sizeof(T)];

    bool hasValue_;

};
