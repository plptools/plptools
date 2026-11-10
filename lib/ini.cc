/*
 * This file is part of plptools.
 *
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
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
#include "config.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ini.h"

std::string ini::serialize(const std::unordered_map<std::string, std::string> &contents) {

    // Sort the keys to ensure we generate stable output.
    std::vector<std::string> sortedKeys;
    sortedKeys.reserve(contents.size());
    for (const auto& entry : contents) {
        sortedKeys.push_back(entry.first);
    }
    std::sort(sortedKeys.begin(), sortedKeys.end());

    // Iterate over the keys outputting the ini file line.
    std::ostringstream stream;
    for (const auto& key : sortedKeys) {
        stream << key << " = " << contents.at(key) << "\r\n";
    }

    return stream.str();
}

std::unique_ptr<std::unordered_map<std::string, std::string>> ini::deserialize(const std::string contents) {
    std::unordered_map<std::string, std::string> result;
    std::string currentKey;
    std::string currentValue;
    std::string currentValueWhitespace;

    enum class ParserState {
        kKeyStart,
        kKey,
        kKeyEnd,
        kValueStart,
        kValue,
        kValueWhitespace,
        kValueEnd,
    };

    ParserState state = ParserState::kKeyStart;
    for (char c : contents) {
        switch (state) {
        case ParserState::kKeyStart:
            if (std::isspace(c)) {
                // Ignore leading whitespace.
                continue;
            } else if (std::isalpha(static_cast<unsigned char>(c))) {
                currentKey += c;
                state = ParserState::kKey;
            } else {
                return nullptr;
            }
            break;
        case ParserState::kKey:
            if (c == '=') {
                state = ParserState::kValueStart;
            } else if (std::isspace(c)) {
                state = ParserState::kKeyEnd;
            } else if (std::isalpha(static_cast<unsigned char>(c))) {
                currentKey += c;
            } else {
                return nullptr;
            }
            break;
        case ParserState::kKeyEnd:
            if (c == '=') {
                state = ParserState::kValueStart;
            } else if (!std::isspace(c)) {
                return nullptr;
            }
            break;
        case ParserState::kValueStart:
            if (!std::isspace(c)) {
                currentValue += c;
                state = ParserState::kValue;
            }
            break;
        case ParserState::kValue:
            if (c == '\r') {
                state = ParserState::kValueEnd;
            } else if (c == '\n') {
                result[currentKey] = currentValue;
                currentKey = "";
                currentValue = "";
                currentValueWhitespace = "";
                state = ParserState::kKeyStart;
            } else if (std::isspace(c)) {
                currentValueWhitespace += c;
                state = ParserState::kValueWhitespace;
            } else {
                currentValue += c;
            }
            break;
        case ParserState::kValueWhitespace:
            if (c == '\r') {
                currentValueWhitespace = "";
                state = ParserState::kValueEnd;
            } else if (c == '\n') {
                result[currentKey] = currentValue;
                currentKey = "";
                currentValue = "";
                currentValueWhitespace = "";
                state = ParserState::kKeyStart;
            } else if (std::isspace(c)) {
                currentValueWhitespace += c;
            } else {
                currentValue += currentValueWhitespace;
                currentValue += c;
                currentValueWhitespace = "";
                state = ParserState::kValue;
            }
            break;
        case ParserState::kValueEnd:
            if (c == '\n') {
                result[currentKey] = currentValue;
                currentKey = "";
                currentValue = "";
                currentValueWhitespace = "";
                state = ParserState::kKeyStart;
            } else {
                return nullptr;
            }
            break;
        }
    }

    // We want to be forgiving of a missing trailing new line, so if we're in state kValue, we emit the last value.
    // This also resets the parser state to allow us to perform a final integrity check.
    if (state == ParserState::kValueStart || state == ParserState::kValue || state == ParserState::kValueWhitespace) {
        result[currentKey] = currentValue;
        state = ParserState::kKeyStart;
    }

    // Ensure that we've seen a complete set of lines.
    if (state != ParserState::kKeyStart) {
        return nullptr;
    }

    return std::unique_ptr<std::unordered_map<std::string, std::string>>(
        new std::unordered_map<std::string, std::string>(std::move(result))
    );
}
