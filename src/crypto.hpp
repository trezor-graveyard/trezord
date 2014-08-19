#ifdef _MSC_VER // trezor-crypto gets compiled as C++ on MSVC
extern "C++" {
#else
extern "C" {
#endif
#include <ecdsa.h>
}

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

// openssl marked as deprecated since osx 10.7
#ifdef __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

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

namespace ssl
{

typedef std::unique_ptr<BIO, decltype(&BIO_free)> BIO_ptr;
typedef std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> PKEY_ptr;
typedef std::unique_ptr<X509, decltype(&X509_free)> X509_ptr;

BIO_ptr
string_to_bio(std::string const &str)
{
    void *data = const_cast<char *>(str.data());
    BIO_ptr bio{BIO_new_mem_buf(data, str.size()), &BIO_free};
    if (!bio) {
        throw std::runtime_error{"BIO_new_mem_buf failed"};
    }
    return bio;
}

void
load_privkey(SSL_CTX *ctx, std::string const &str)
{
    LOG(INFO) << "loading ssl private key";
    BIO_ptr bio{string_to_bio(str)};
    PKEY_ptr pkey{PEM_read_bio_PrivateKey(bio.get(), 0, 0, 0), &EVP_PKEY_free};
    if (!pkey) {
        throw std::runtime_error{"PEM_read_bio_PrivateKey failed"};
    }
    SSL_CTX_use_PrivateKey(ctx, pkey.get());
}

void
load_cert(SSL_CTX *ctx, std::string const &str)
{
    LOG(INFO) << "loading ssl certificate";
    BIO_ptr bio{string_to_bio(str)};
    X509_ptr cert{PEM_read_bio_X509(bio.get(), 0, 0, 0), &X509_free};
    if (!cert) {
        throw std::runtime_error{"PEM_read_bio_X509 failed"};
    }
    SSL_CTX_use_certificate(ctx, cert.get());
}

}

}
}
