/*
 * Functions deal with the js challenge
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Ben: July 2013, Initial version
 */
#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <cmath>

#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>

#include <ts/ts.h>

 using namespace std;

#include "cookie_parser.h"
#include "challenge_manager.h"

// TODO: this should be loaded from the db
unsigned char ChallengeManager::key[] = "abcdefghijklmnop";
unsigned ChallengeManager::number_of_trailing_zeros = 8; //TODO: only support multiples of 4 for now
string ChallengeManager::zeros_in_javascript = "00"; // needs to be in accordance with the number above
std::string ChallengeManager::solver_page = "/usr/local/etc/trafficserver/solver.html";//"/home/vmon/doc/code/deflect/ats_lab/deflect/banjax/challenger/solver.html";

std::string ChallengeManager::sub_token = "$token";
std::string ChallengeManager::sub_time = "$time";
std::string ChallengeManager::sub_url = "$url";
std::string ChallengeManager::sub_zeros = "$zeros";

/**
 * Sets the class parameters
 * @param mKey              AES key
 * @param nb_trailing_zeros number of zeros in the SHA256
 */
void ChallengeManager::set_parameters(string mKey, unsigned nb_trailing_zeros){
  strcpy( (char*) key, mKey.c_str() );
  number_of_trailing_zeros = nb_trailing_zeros;
  // TODO: will need to be changed when we modify the js
  zeros_in_javascript = string(nb_trailing_zeros / 4, '0');
}

/**
 * Splits a string according to a delimiter
 */
vector<string> ChallengeManager::split(const string &s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

/**
 * Generates the token from the client ip and the cookie's validity
 * @param  ip client ip
 * @param  t  time until which the cookie will be valid
 * @return    the encrypted token
 */
string ChallengeManager::generate_token(string ip, long t){
  const unsigned text_length = 8;

    // concatenate the ip and the time
  vector<string> x = split(ip, '.');
  if(x.size() != 4){
        // TODO: change this to ATS logging
        // cerr << "ChallengeManager::generate_token : ip has " << x.size() << " elements.\n";
  }

  unsigned char * uncrypted = new unsigned char[text_length];
  *(uncrypted+text_length) = '\0';

    // add the ip
  for(unsigned int i=0; i<x.size(); i++){
    *(uncrypted+i) = (unsigned char) atoi(x[i].c_str());
  }
    // add the time
  for(int i=7; i>=4; i--){
    *(uncrypted+i) = (unsigned char) t%256;
    t >>= 8;
  }

    // encode the token and transform it in base64
  string result = base64_encode(encrypt_token(uncrypted));
  delete[] uncrypted;

    // cout << "base64 = " << result << endl;
    // cout << "token length = " << result.size() << endl;
 
  //TSDebug("banjax", "token = %s", result.c_str());
  //TSDebug("banjax", "token len = %d", (int) result.length());

  return result;
}

/**
 * Encrypts the token using AES128. Assumes that the token has a length of 16 bytes.
 * @param  uncrypted original token (8 bytes)
 * @return           encrypted token (16 bytes)
 */
string ChallengeManager::encrypt_token(unsigned char uncrypted[]){
  unsigned char enc_out[80*sizeof(char)];
  *(enc_out+16) = '\0';

  AES_KEY enc_key;

  AES_set_encrypt_key(key, 128, &enc_key);
  AES_encrypt(uncrypted, enc_out, &enc_key);

    // // some testing: print original and encoded
    // printf("original:\t");
    // for(int i=0;*(uncrypted+i)!=0x00;i++)
    //     printf("%X ",*(uncrypted+i));
    // printf("\nencrypted:\t");
    // for(int i=0;*(enc_out+i)!=0x00;i++)
    //     printf("%X ",*(enc_out+i));
    // printf("\n");

  string result(reinterpret_cast<const char*>(enc_out));

  return result;
}

/**
 * Decrypt the token
 * @param  token the encrypted token
 * @return       the decrypted token 
 */
unsigned char * ChallengeManager::decrypt_token(string token){
  unsigned char enc_out[16*sizeof(char)];
  unsigned char * dec_out = new unsigned char[80*sizeof(char)];

  strcpy( (char*) enc_out, token.c_str() );

  AES_KEY dec_key;

  AES_set_decrypt_key(key, 128, &dec_key);
  AES_decrypt(enc_out, dec_out, &dec_key);

    // // some tests
    // int i;
    // printf("\nencrypted:\t");
    // for(i=0;*(enc_out+i)!=0x00;i++)
    //     printf("%X ",*(enc_out+i));
    // printf("\n");
    // // cout << "encrypted length = " << i << endl;
    // printf("decrypted:\t");
    // for(i=0;*(dec_out+i)!=0x00;i++)
    //     printf("%X ",*(dec_out+i));
    // printf("\n");
    // // cout << "decrypted length = " << i << endl;

  return dec_out;
}

/**
 * Checks if the SHA256 of the cookie has the correct number of zeros
 * @param  cookie the value of the cookie
 * @return        true if the SHA256 of the cookie verifies the challenge
 */
bool ChallengeManager::check_sha(const char* cookiestr, const char* cookie_val_end){

  char outputBuffer[65];

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, cookiestr, cookie_val_end - cookiestr);
  SHA256_Final(hash, &sha256);

  for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
  }
  outputBuffer[64] = 0;

  //TSDebug("banjax", "SHA256 = %s", outputBuffer);

  for(unsigned i=0; i<number_of_trailing_zeros/4; i++){
    if(outputBuffer[i] != '0'){
      //TSDebug("banjax", "i= %d, out = %c", i, outputBuffer[i]);
      return false;
    }
  } 

    // printf("sha:\t");
    // for(int i=0; i<SHA256_DIGEST_LENGTH; i++)
    //     printf("%X ",*(hash+i));
    // printf("\n");  

  //unsigned last = hash[SHA256_DIGEST_LENGTH-1];
  //for(unsigned int i=0; i<number_of_trailing_zeros; i++){
  //  if(last%2 != 0) return false;
  //  last >>= 1;
  //}

  return true;
}

/**
 * Checks if the cookie is valid: sha256, ip, and time
 * @param  cookie the value of the cookie
 * @param  ip     the client ip
 * @return        true if the cookie is valid
 */
bool ChallengeManager::check_cookie(string cookie_jar, string ip){


  //let find the deflect cookie inside the jar
  CookieParser cookie_parser;
  const char* next_cookie = cookie_jar.c_str();
  while((next_cookie = cookie_parser.parse_a_cookie(next_cookie)) != NULL)
        if (!(memcmp(cookie_parser.str, "deflect", cookie_parser.nam_end - cookie_parser.str)))
      break;

  TSDebug("banjax", "cookie_val: '%s'",cookie_parser.val_start);

  // check the SHA256 of the cookie
  if(!check_sha(cookie_parser.val_start, cookie_parser.val_end)){
    TSDebug("banjax", "ChMng: SHA256 not valid");
    return false;
  }

    // separate token from user-found value (token has length 22 in base64 encoding)
  string token = base64_decode(cookie_parser.val_start, cookie_parser.val_end);

    // unencode token and separate ip from time
  unsigned char * original = decrypt_token(token);

    // check the ip against the client's ip
  vector<string> x = split(ip, '.');
  for(unsigned int i=0; i<x.size(); i++){
    if ( ((int) *(original+i)) != atoi(x[i].c_str()) ){
      TSDebug("banjax", "ChMng: ip not valid");
      return false;
    }
  }

  // check the time
  long token_time = 0;
  for(int i=4; i<8; i++){
    token_time <<= 8;
    token_time += (long) *(original+i);
  }
  if(token_time < time(NULL)){
    TSDebug("banjax", "ChMng: time not valid");
    return false;
  }

  return true;

}

/**
 * Replaces a substring by another
 * @param original string to be modified
 * @param from     substing to be replaced
 * @param to       what to replace by
 */
bool ChallengeManager::replace(string &original, string &from, string &to){
  size_t start_pos = original.find(from);
  if(start_pos == string::npos)
    return false;
  original.replace(start_pos, from.length(), to);
  return true;
}

string ChallengeManager::generate_html(string ip, long t, string url){
  
  // generate the token
  string token = ChallengeManager::generate_token(ip, t);

  // load the page
  //ifstream ifs("../challenger/solver.html");
  //TODO: Should not re-read the fiel upon each request
  //We need to read the whole string from the database infact
  ifstream ifs(ChallengeManager::solver_page.c_str());
  string page( (istreambuf_iterator<char>(ifs) ), (istreambuf_iterator<char>()) );

  // set the time in the correct format
  time_t rawtime = (time_t) t;
  struct tm *timeinfo;
  char buffer [30];
  timeinfo = gmtime(&rawtime);
  const char *format = "%a, %d %b %G %T GMT";
  strftime(buffer, 30, format, timeinfo);
  string t_str(buffer);

  // replace the time
  replace(page, sub_time, t_str);
  // replace the token
  replace(page, sub_token, token);
  // replace the url
  replace(page, sub_url, url);
  // set the correct number of zeros
  replace(page, sub_zeros, zeros_in_javascript);

  return page;
}

const char ChallengeManager::b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const char ChallengeManager::reverse_table[128] = {
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

string ChallengeManager::base64_encode(const string &bindata)
{
  if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
    throw length_error("Converting too large a string to base64.");
  }

  const size_t binlen = bindata.size();
  // Use = signs so the end is properly padded.
  //string retval((((binlen + 2) / 3) * 4), '=');
  string retval(ceil(((binlen) / 3.0) * 4.0), '=');
  size_t outpos = 0;
  int bits_collected = 0;
  unsigned int accumulator = 0;
  const string::const_iterator binend = bindata.end();

  for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
    accumulator = (accumulator << 8) | (*i & 0xffu);
    bits_collected += 8;
    while (bits_collected >= 6) {
      bits_collected -= 6;
      retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
    }
  }
   if (bits_collected > 0) { // Any trailing bits that are missing.
    assert(bits_collected < 6);
    accumulator <<= 6 - bits_collected;
    retval[outpos++] = b64_table[accumulator & 0x3fu];
   }
   assert(outpos >= (retval.size() - 2));
   assert(outpos <= retval.size());
   return retval;
}

string ChallengeManager::base64_decode(const char* data, const char* data_end)
{
  string retval;
  int bits_collected = 0;
  unsigned int accumulator = 0;

  for (; data != data_end; data++) {
    const int c = *data;
    if (isspace(c) || c == '=') {
         // Skip whitespace and padding. Be liberal in what you accept.
      continue;
    }
    if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
      throw invalid_argument("This contains characters not legal in a base64 encoded string.");
    }
    accumulator = (accumulator << 6) | reverse_table[c];
    bits_collected += 6;
    if (bits_collected >= 8) {
      bits_collected -= 8;
      retval += (char)((accumulator >> bits_collected) & 0xffu);
    }
  }
  return retval;
}
