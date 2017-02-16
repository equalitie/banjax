#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <string.h>
#include <string>

#include "../include/cookiehash.h"
#include "../include/base64.h"

// Generate a cookie hash over :
// captcha, the captcha string, null terminated
// secret (fixed length secret SECRET_LENGTH)
// valid_till_timestamp, a time_t value at which the generated hash expires
// remoteaddress, the remote address ,null terminated
//
// output:
// obuf, the resulting hash of length HASH_LENGTH
//
void GenerateCookieHash(uchar *captcha,uchar *secret,time_t valid_till_timestamp,uchar *remoteaddress,uchar *obuf)
{
  uchar ibuf[200]; // should be safe
  uchar *ibufp=ibuf;
  int tlen;

  memcpy(ibufp,&valid_till_timestamp,sizeof(valid_till_timestamp));
  ibufp+=sizeof(valid_till_timestamp);
  memcpy(ibufp,remoteaddress,tlen=strlen((char*)remoteaddress));
  ibufp+=tlen;
  memcpy(ibufp,captcha,tlen=strlen((char*)captcha));
  ibufp+=tlen;
  unsigned int totallen=ibufp-ibuf;
  unsigned int retlen=HASH_LENGTH;
  HMAC(EVP_sha1(),secret,SECRET_LENGTH,ibuf,totallen,obuf,&retlen);
}

// Generate a cookie over :
// captcha, the captcha string, null terminated
// secret (fixed length secret SECRET_LENGTH)
// valid_till_timestamp, a time_t value at which the generated hash expires
// remoteaddress, the remote address ,null terminated
//
// output
// cookiestring_out, a hex value string presentation of the contatenation of the hash of the above and the valid_till_timestamp
// valid_till_timestamp is the only parameter which cannot be extracted from the context and should be saved in the cookie
int GenerateCookie(uchar *captcha,uchar *secret,time_t valid_till_timestamp,uchar *remoteaddress,uchar *cookiestring_out)
{
  uchar hash[HASH_LENGTH];
  uchar cookie[COOKIE_LENGTH];

  GenerateCookieHash(captcha,secret,valid_till_timestamp,remoteaddress,hash);
  memcpy(cookie,hash,HASH_LENGTH);
  memcpy(cookie+HASH_LENGTH,&valid_till_timestamp,sizeof(time_t));
  // optimize
  std::string ci=std::string((char *)cookie,COOKIE_LENGTH);
  std::string co=Base64::Encode(ci);
  strcpy((char *)cookiestring_out,co.c_str());
  return 1;
}

// validates a cookie over :
// captcha, the captcha string, null terminated
// secret (fixed length secret SECRET_LENGTH)
// current_timestamp, a time_t value which must be compared to the valid_until saved in the cookiestring
// remoteaddress, the remote address ,null terminated
// cookiestring, a cookiestring generated by GenerateCookie
//
// output
// the function returns an integer
// -3 Length mismatch
// -1 hash mismatch
// -2 expired
// 1 ok
int ValidateCookie(uchar *captcha,uchar *secret,time_t current_timestamp,uchar *remoteaddress,uchar *cookiestring)
{
  char cookie[COOKIE_LENGTH];
  char hash[HASH_LENGTH];
  if (strlen((char*)cookiestring)!=COOKIE_B64_LENGTH)
    return -3;
  std::string cookiedata=Base64::Decode((const char *)cookiestring,(const char *)(cookiestring+strlen((char *) cookiestring)));
  memcpy(cookie,cookiedata.c_str(),COOKIE_LENGTH);
  time_t *valid_until_timestamp=(time_t *)(cookie+HASH_LENGTH);
  GenerateCookieHash(captcha,secret,*valid_until_timestamp,remoteaddress,(uchar*)hash);
  if (memcmp(cookie,hash,HASH_LENGTH))
  {
    return -1;
  }
  if ((long)(*valid_until_timestamp)<current_timestamp)
  {
    return -2;
  }
  return 1;

}

/*
uchar *secret="12345678901234567890123456789012"; //32 bytes of secret should be enough

int main()
{
  uchar cookie[100];
  time_t curtime=time(NULL);
  for(int c=0;c<1000000;c++)
  {
    GenerateCookie("hallo",secret,curtime,"127.0.0.1",cookie);
    int ret=ValidateCookie("hallo",secret,curtime,"127.0.0.1",cookie);
    int fail1=ValidateCookie("hallo2",secret,curtime,"127.0.0.1",cookie);
    int fail2=ValidateCookie("hallo",secret,curtime,"125.0.0.1",cookie);
    int fail3=ValidateCookie("hallo",secret,curtime+100,"127.0.0.1",cookie);
    int fail4=ValidateCookie("hallo",secret,curtime,"127.0.0.1",cookie);

    if (ret!=1 || fail1!=-1 || fail2!=-1 || fail3!=-2 || fail4!=1)
    {
      printf("%d %d %d %d %d\n%ld %ld\n ",ret,fail1,fail2,fail3,fail4,curtime,curtime+100);
    }
  }
  time_t ct2=time(NULL);
  printf("%ld\n",ct2-curtime);

}

*/
