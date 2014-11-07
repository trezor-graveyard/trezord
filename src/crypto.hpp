#ifdef _MSC_VER // trezor-crypto gets compiled as C++ on MSVC
extern "C++" {
#else
extern "C" {
#endif
#include <ecdsa.h>
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
        int ret = ecdsa_verify(keys[i], sig, msg, msg_len);
        if (ret == 0) {
            return true;
        }
    }
    return false;
}

}
}
