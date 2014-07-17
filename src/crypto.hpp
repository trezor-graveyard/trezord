extern "C" {
#include <ecdsa.h>
}

namespace trezord
{
namespace crypto
{

bool
verify_signature(const std::uint8_t *sig,
                 std::size_t siglen,
                 const std::uint8_t *data,
                 std::size_t datalen,
                 const char **keys,
                 std::size_t keyslen)
{
    for (std::size_t i = 0; i < keyslen; i++) {
        int ret = ecdsa_verify((const std::uint8_t *)keys[i], sig, data, (uint32_t)datalen);
        if (ret == 0) {
            return true;
        }
    }
    return false;
}

}
}
