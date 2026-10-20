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
#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace ini {

/**
* Simple parser for flat ini data.
*
* Does not support sections or quoted values. Supports Windows and Unix line endings.
*/
extern std::unique_ptr<std::unordered_map<std::string, std::string>> deserialize(const std::string contents);

/**
* Outputs simple flat ini data.
*
* Does not currently attempt to quote values (multi-line strings and strings with trailing whitespace will break this.)
*/
extern std::string serialize(const std::unordered_map<std::string, std::string> &contents);

};
