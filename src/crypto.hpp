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

#ifdef _MSC_VER // trezor-crypto gets compiled as C++ on MSVC
extern "C++" {
#else
extern "C" {
#endif
#include <ecdsa.h>
#include <secp256k1.h>
}

namespace trezord
{
namespace crypto
{

bool
verify_signature(std::uint8_t const *sig,
                 std::uint8_t const *msg,
                 std::size_t msg_len,
                 std::uint8_t const **keys,
                 std::size_t keys_len)
{
    for (std::size_t i = 0; i < keys_len; i++) {
        int ret = ecdsa_verify(&secp256k1, keys[i], sig, msg, msg_len);
        if (ret == 0) {
            return true;
        }
    }
    return false;
}

}
}
