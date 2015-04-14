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

#include <hidapi.h>

namespace trezord
{
namespace hid
{

static std::unique_ptr< utils::async_executor > hid_executor;

// Init/exit

void
init()
{
    hid_init();
    hid_executor.reset(new utils::async_executor());
}

void
exit()
{
    hid_exit();
    hid_executor.reset();
}

// Enumeration

hid_device_info *
enumerate(unsigned short vendor_id, unsigned short product_id)
{
    return hid_executor->await([=] {
            return hid_enumerate(vendor_id, product_id);
        });
}

void
free_enumeration(hid_device_info *devs)
{
    return hid_executor->await([=] {
            return hid_free_enumeration(devs);
        });
}

// Open/close

hid_device *
open_path(char const *path)
{
    return hid_executor->await([=] { return hid_open_path(path); });
}

void
close(hid_device *device)
{
    return hid_executor->await([=] { return hid_close(device); });
}

// Communication

int
write(hid_device *device, unsigned char const *data, size_t length)
{
    return hid_executor->await([=] {
            return hid_write(device, data, length);
        });
}

int
read(hid_device *device, unsigned char *data, size_t length)
{
    return hid_executor->await([=] {
            return hid_read(device, data, length);
        });
}

int
read_timeout(hid_device *device, unsigned char *data, size_t length, int milliseconds)
{
    return hid_executor->await([=] {
            return hid_read_timeout(device, data, length, milliseconds);
        });
}

int
send_feature_report(hid_device *device, unsigned char const *data, size_t length)
{
    return hid_executor->await([=] {
            return hid_send_feature_report(device, data, length);
        });
}

}
}
