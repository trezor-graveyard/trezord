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
        std::ostream_iterator<char> iterator(stream);
        boost::algorithm::hex(str, iterator);
        std::string hex(stream.str());
        boost::algorithm::to_lower(hex);
        return hex;
    }
    catch (std::exception const &e) {
        throw std::invalid_argument("cannot encode value to hex");
    }
}

std::string
hex_decode(std::string const &hex)
{
    try {
        std::ostringstream stream;
        std::ostream_iterator<char> iterator(stream);
        boost::algorithm::unhex(hex, iterator);
        return stream.str();
    }
    catch (const std::exception &e) {
        throw std::invalid_argument("cannot decode value from hex");
    }
}

}
}
