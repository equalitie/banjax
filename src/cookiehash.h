#include <time.h>
#include <openssl/sha.h>

#define uchar unsigned char
#define SECRET_LENGTH SHA256_DIGEST_LENGTH //the secret is a SHA256 of user password so it has 256/8 = 32 bytes
#define HASH_LENGTH 20
#define COOKIE_LENGTH (HASH_LENGTH+sizeof(time_t))
#define COOKIE_B64_LENGTH ((COOKIE_LENGTH+2)/3)*4

int GenerateCookie(uchar *captcha,uchar *secret,time_t valid_till_timestamp,uchar *remoteaddress,uchar *cookiestring_out);
int ValidateCookie(uchar *captcha,uchar *secret,time_t current_timestamp,uchar *remoteaddress,uchar *cookiestring);
