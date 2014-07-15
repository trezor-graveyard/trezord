#include <hidapi.h>

#include <netinet/in.h>

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace trezord
{
namespace wire
{

typedef std::vector<std::string> device_path_list;

template<typename F>
device_path_list
enumerate_devices(F filter)
{
    device_path_list list;
    hid_device_info *infos = hid_enumerate(0x00, 0x00);
    hid_device_info *i;

    for (i = infos; i != nullptr; i = i->next) {
        if (filter(i)) {
            list.push_back(i->path);
        }
    }

    hid_free_enumeration(infos);
    return list;
}

struct device
{
    typedef std::uint8_t char_type;
    typedef std::size_t size_type;

    typedef std::runtime_error open_error;
    typedef std::runtime_error read_error;
    typedef std::runtime_error write_error;

    device(const device&) = delete;
    device &operator=(const device&) = delete;

    device(const std::string &path)
    {
        hid = hid_open_path(path.c_str());
        if (!hid) {
            throw open_error("HID device open failed");
        }

        hid_set_nonblocking(hid, 0); // always block on read

        unsigned char uart[] = {0x41, 0x01}; // enable UART
        unsigned char txrx[] = {0x43, 0x03}; // purge TX/RX FIFOs
        hid_send_feature_report(hid, uart, 2);
        hid_send_feature_report(hid, txrx, 2);
    }

    ~device() { hid_close(hid); }

    void
    read_buffered(char_type *data,
                  size_type len)
    {
        if (read_buffer.empty()) {
            buffer_report();
        }
        size_type n = read_report_from_buffer(data, len);
        if (n < len) {
            read_buffered(data + n, len - n);
        }
    }

    void
    write(const char_type *data,
          size_type len)
    {
        size_type n = write_report(data, len);
        if (n < len) {
            write(data + n, len - n);
        }
    }

private:

    void
    buffer_report()
    {
        using namespace std;

        report_type report;
        int r = hid_read(hid, report.data(), report.size());

        if (r < 0) {
            throw read_error("HID device read failed");
        }
        if (r > 0) {
            // copy to the buffer, skip the report number
            char_type rn = report[0];
            size_type n = min(static_cast<size_type>(rn),
                              static_cast<size_type>(r - 1));
            copy(report.begin() + 1,
                 report.begin() + n,
                 back_inserter(read_buffer));
        }
    }

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

    size_type
    write_report(const char_type *data,
                 size_type len)
    {
        using namespace std;

        report_type report;
        report[0] = report.size() - 1; // rest is 0x00

        size_type n = min(report.size() - 1, len);
        copy(data, data + n, report.begin() + 1); // copy behind report number

        int r = hid_write(hid, report.data(), report.size());
        if (r < 0) {
            throw write_error("HID device write failed");
        }
        if (r < report.size()) {
            throw write_error("HID device write was insufficient");
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
            throw header_read_error("header bytes are malformed");
        }

        device.read_buffered(buf, 6);

        id = ntohs((buf[0] << 0) | (buf[1] << 8));
        size = ntohl((buf[2] << 0) | (buf[3] << 8) |
                     (buf[4] << 16) | (buf[5] << 24));

        // 1MB of the message size treshold
        static const std::uint32_t max_size = 1024 * 1024;
        if (size > max_size) {
            throw header_read_error("message is too big");
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
