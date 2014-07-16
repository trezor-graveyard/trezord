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

void
make_digest(const EVP_MD *type,
            const std::uint8_t *data,
            std::size_t datalen,
            std::uint8_t *dig,
            std::size_t *diglen)
{
    EVP_MD_CTX ctx;
    unsigned int digleni;

    EVP_MD_CTX_init(&ctx);
    EVP_DigestInit(&ctx, type);
    EVP_DigestUpdate(&ctx, data, datalen);
    EVP_DigestFinal(&ctx, dig, &digleni);
    EVP_MD_CTX_cleanup(&ctx);
    *diglen = digleni;
}

EC_KEY *
read_eckey(const char *buf,
           std::size_t buflen)
{
    BIO *bio;
    EVP_PKEY *evpkey;
    EC_KEY *eckey = 0;

    bio = BIO_new_mem_buf((void*)buf, buflen);
    if (!bio) {
        // FBLOG_FATAL("check_signature()", "BIO_new_mem_buf failed");
        goto ret;
    }
    evpkey = PEM_read_bio_PUBKEY(bio, 0, 0, 0);
    if (!evpkey) {
        // FBLOG_FATAL("check_signature()", "PEM_read_bio_PUBKEY failed");
        goto err0;
    }
    eckey = EVP_PKEY_get1_EC_KEY(evpkey);
    if (!eckey) {
        // FBLOG_FATAL("check_signature()", "EVP_PKEY_get1_EC_KEY failed");
        goto err1;
    }

err1:
    EVP_PKEY_free(evpkey);
err0:
    BIO_free(bio);
ret:
    return eckey;
}

bool
verify_signature(const std::uint8_t *sig,
                 std::size_t siglen,
                 const std::uint8_t *data,
                 std::size_t datalen,
                 const char **keys,
                 std::size_t keyslen)
{
    ECDSA_SIG ecsig;
    const std::uint8_t *sig_r = sig;
    const std::uint8_t *sig_s = sig + siglen / 2;
    ecsig.r = BN_bin2bn(sig_r, siglen / 2, 0);
    ecsig.s = BN_bin2bn(sig_s, siglen / 2, 0);

    std::size_t diglen;
    std::uint8_t dig[EVP_MAX_MD_SIZE];
    make_digest(EVP_sha256(), data, datalen, dig, &diglen);

    for (std::size_t i = 0; i < keyslen; i++) {
        EC_KEY *eckey = read_eckey(keys[i], strlen(keys[i]));
        if (!eckey) {
            continue;
        }

        int ret = ECDSA_do_verify(dig, diglen, &ecsig, eckey);
        EC_KEY_free(eckey);
        if (ret > 0) {
            return true;
        }
    }

    return false;
}

}
}
