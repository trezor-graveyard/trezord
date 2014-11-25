/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 SatoshiLabs
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

namespace trezord
{
namespace utils
{

std::string
hex_encode(std::string const &str)
{
    try {
        std::ostringstream stream;
        std::ostream_iterator<char> iterator{stream};
        boost::algorithm::hex(str, iterator);
        std::string hex{stream.str()};
        boost::algorithm::to_lower(hex);
        return hex;
    }
    catch (std::exception const &e) {
        throw std::invalid_argument{"cannot encode value to hex"};
    }
}

std::string
hex_decode(std::string const &hex)
{
    try {
        std::ostringstream stream;
        std::ostream_iterator<char> iterator{stream};
        boost::algorithm::unhex(hex, iterator);
        return stream.str();
    }
    catch (std::exception const &e) {
        throw std::invalid_argument{"cannot decode value from hex"};
    }
}

}
}
