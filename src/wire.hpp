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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <boost/thread.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace trezord
{
namespace wire
{

struct device_info
{
    std::uint16_t vendor_id;
    std::uint16_t product_id;
    std::string path;

    bool
    operator==(device_info const &rhs) const
    {
        return (vendor_id == rhs.vendor_id)
            && (product_id == rhs.product_id)
            && (path == rhs.path);
    }
};

typedef std::vector<device_info> device_info_list;

template <typename F>
device_info_list
enumerate_connected_devices(F filter)
{
    device_info_list list;
    auto *infos = hid::enumerate(0x00, 0x00);

    for (auto i = infos; i != nullptr; i = i->next) {
        // skip unsupported devices
        if (!filter(i)) {
            continue;
        }
        // skip foreign interfaces
        if (i->interface_number > 0) {
            CLOG(DEBUG, "wire.enumerate") << "skipping, invalid device";
            continue;
        }
        // skip debug interface
        if (i->usage_page == 0xFF01) {
            CLOG(DEBUG, "wire.enumerate") << "skipping, debug interface";
            continue;
        }
        // skip fido interface
        if (i->usage_page == 0xF1D0) {
            CLOG(DEBUG, "wire.enumerate") << "skipping, fido interface";
            continue;
        }
        list.emplace_back(
            device_info{
                i->vendor_id,
                i->product_id,
                i->path});
    }

    hid::free_enumeration(infos);
    return list;
}

struct device
{
    typedef std::uint8_t char_type;
    typedef std::size_t size_type;

    struct open_error
        : public std::runtime_error
    { using std::runtime_error::runtime_error; };

    struct read_error
        : public std::runtime_error
    { using std::runtime_error::runtime_error; };

    struct write_error
        : public std::runtime_error
    { using std::runtime_error::runtime_error; };

    device(device const&) = delete;
    device &operator=(device const&) = delete;

    device(char const *path)
    {
        hid = hid::open_path(path);
        if (!hid) {
            throw open_error("HID device open failed");
        }
    }

    ~device() { hid::close(hid); }

    void
    read_buffered(char_type *data,
                  size_type len)
    {
        for (;;) {
            if (read_buffer.empty()) {
                buffer_report();
            }
            size_type n = read_report_from_buffer(data, len);
            if (n < len) {
                data += n;
                len -= n;
            } else {
                break;
            }
        }
    }

    void
    write(char_type const *data,
          size_type len)
    {
        for (;;) {
            size_type n = write_report(data, len);
            if (n < len) {
                data += n;
                len -= n;
            } else {
                break;
            }
        }
    }

private:

    size_type
    read_report_from_buffer(char_type *data,
                            size_type len)
    {
        using namespace std;

        size_type n = min(read_buffer.size(), len);
        auto r1 = read_buffer.begin();
        auto r2 = read_buffer.begin() + n;

        copy(r1, r2, data); // copy to data
        read_buffer.erase(r1, r2); // shift from buffer

        return n;
    }

    void
    buffer_report()
    {
        using namespace std;

        report_type report;
        int r;

        do {
            r = hid::read_timeout(hid, report.data(), report.size(), 50);
        } while (r == 0);

        if (r < 0) {
            throw read_error("HID device read failed");
        }
        if (r > 0) {
            // copy to the buffer, skip the report number
            char_type rn = report[0];
            size_type n = min(static_cast<size_type>(rn),
                              static_cast<size_type>(r - 1));
            copy(report.begin() + 1,
                 report.begin() + 1 + n,
                 back_inserter(read_buffer));
        }
    }

    size_type
    write_report(char_type const *data,
                 size_type len)
    {
        using namespace std;

        report_type report;
        report.fill(0x00);
        report[0] = report.size() - 1;

        size_type n = min(report.size() - 1, len);
        copy(data, data + n, report.begin() + 1); // copy behind report number

        int r = hid::write(hid, report.data(), report.size());

        if (r < 0) {
            throw write_error{"HID device write failed"};
        }
        if (r < report.size()) {
            throw write_error{"HID device write was insufficient"};
        }

        return n;
    }

    typedef std::vector<char_type> buffer_type;
    typedef std::array<char_type, 64> report_type;

    hid_device *hid;
    buffer_type read_buffer;
};

struct message
{
    std::uint16_t id;
    std::vector<std::uint8_t> data;

    typedef device::read_error header_read_error;

    void
    read_from(device &device)
    {
        device::char_type buf[6];
        std::uint32_t size;

        device.read_buffered(buf, 1);
        while (buf[0] != '#') {
            device.read_buffered(buf, 1);
        }

        device.read_buffered(buf, 1);
        if (buf[0] != '#') {
            throw header_read_error{"header bytes are malformed"};
        }

        device.read_buffered(buf, 6);

        id = ntohs((buf[0] << 0) | (buf[1] << 8));
        size = ntohl((buf[2] << 0) | (buf[3] << 8) |
                     (buf[4] << 16) | (buf[5] << 24));

        // 1MB of the message size treshold
        static const std::uint32_t max_size = 1024 * 1024;
        if (size > max_size) {
            throw header_read_error{"message is too big"};
        }

        data.resize(size);
        device.read_buffered(data.data(), data.size());
    }

    void
    write_to(device &device) const
    {
        std::size_t buf_size = 8 + data.size();
        device::char_type buf[buf_size];

        buf[0] = '#';
        buf[1] = '#';

        std::uint16_t id_ = htons(id);
        buf[2] = (id_ >> 0) & 0xFF;
        buf[3] = (id_ >> 8) & 0xFF;

        std::uint32_t size_ = htonl(data.size());
        buf[4] = (size_ >> 0) & 0xFF;
        buf[5] = (size_ >> 8) & 0xFF;
        buf[6] = (size_ >> 16) & 0xFF;
        buf[7] = (size_ >> 24) & 0xFF;

        std::copy(data.begin(), data.end(), &buf[8]);
        device.write(buf, buf_size);
    }
};

}
}
