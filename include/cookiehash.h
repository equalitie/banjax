#include <time.h>

#define uchar unsigned char
#define SECRET_LENGTH 32
#define HASH_LENGTH 20
#define COOKIE_LENGTH (HASH_LENGTH+sizeof(time_t))

int GenerateCookie(uchar *captcha,uchar *secret,time_t valid_till_timestamp,uchar *remoteaddress,uchar *cookiestring_out);
int ValidateCookie(uchar *captcha,uchar *secret,time_t current_timestamp,uchar *remoteaddress,uchar *cookiestring);
