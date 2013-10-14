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
#include <algorithm>

#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#include <ts/ts.h>

#include <assert.h>

using namespace std;

#include "cookie_parser.h"
#include "challenge_manager.h"

string ChallengeManager::zeros_in_javascript = "00"; // needs to be in accordance with the number above

std::string ChallengeManager::sub_token = "$token";
std::string ChallengeManager::sub_time = "$time";
std::string ChallengeManager::sub_url = "$url";
std::string ChallengeManager::sub_zeros = "$zeros";

/**
  Overload of the load config
  reads all the regular expressions from the database.
  and compile them
*/
void
ChallengeManager::load_config(libconfig::Setting& cfg)
{
   try
   {
     const libconfig::Setting &challenger_hosts = cfg["hosts"];
     unsigned int count = challenger_hosts.getLength();

     //now we compile all of them and store them for later use
     for(unsigned int i = 0; i < count; i++)
       challenged_hosts.push_back(challenger_hosts[i]);

     //we use SHA256 to generate a key from the user passphrase
     //we will use half of the hash as we are using AES128
     string challenger_key = cfg["key"];
     unsigned char hashed_key[SHA256_DIGEST_LENGTH];
     SHA256((const unsigned char*)challenger_key.c_str(), challenger_key.length(), hashed_key);
     AES_set_encrypt_key(hashed_key, 128, &enc_key);
     AES_set_decrypt_key(hashed_key, 128, &dec_key);
 
     number_of_trailing_zeros = cfg["difficulty"];
     assert(!(number_of_trailing_zeros % 4));
     zeros_in_javascript = string(number_of_trailing_zeros / 4, '0');
 
     cookie_life_time = cfg["cookie_life_time"];
   }
   catch(const libconfig::SettingNotFoundException &nfex)
     {
       TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "Bad config for filter %s", BANJAX_FILTER_NAME.c_str());
     }

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
  //we are not padding for now so we only need to make sure
  //we are using less than AES_BLOCK_SIZE
  //const unsigned token_data_length = 8;


  // concatenate the ip and the time
  //TODO: If we get the ip as the ip struct
  //it is already numerical so we don't need to
  //waste time here
  vector<string> x = split(ip, '.');
  if(x.size() != 4){
        // TODO: change this to ATS logging
        // cerr << "ChallengeManager::generate_token : ip has " << x.size() << " elements.\n";
  }

  unsigned char uncrypted[AES_BLOCK_SIZE], encrypted_token[AES_BLOCK_SIZE+1];
  encrypted_token[AES_BLOCK_SIZE] = '\0';
    // add the ip
  for(unsigned int i=0; i<x.size(); i++){
    *(uncrypted+i) = (unsigned char) atoi(x[i].c_str());
  }
    // add the time
  for(int i=7; i>=4; i--){
    *(uncrypted+i) = (unsigned char) t%256;
    t >>= 8;
  }

  //this might be necessary as we are using ECB mode
  //BYTES_rand(uncrypted + token_data_length, AES_BLOCK_SIZE - token_data_length);
  AES_encrypt(uncrypted, encrypted_token, &enc_key);
  // encode the token and transform it in base64
  string result = base64_encode((char*)encrypted_token);
 
  return result;
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
  unsigned char original[AES_BLOCK_SIZE];
  AES_decrypt((const unsigned char*)token.c_str(), original, &dec_key);

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
  //replace(page, sub_url, url);
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


/**
   overloaded execute to execute the filter, It calls cookie checker
   and if it fails ask for responding by the filter.
*/
FilterResponse 
ChallengeManager::execute(const TransactionParts& transaction_parts)
{
  /*
   * checking the cookie and serving the js challenge if does not pass
   */
  /*  TSDebug("banjax", "Checking for challenge");

    url_str = TSUrlStringGet (bufp, url_loc, &url_length);
    time_validity = time(NULL) + 60*60*24; // TODO: one day validity for now, should be changed
    //buf_str = ChallengeManager::generate_html(client_ip, time_validity, url_str);
    buf_str = "This is a test";
    buf = (char *) TSmalloc(buf_str.length()+1);
    //strcpy(buf, buf_str.c_str());
    
    url_str = TSUrlStringGet (bufp, url_loc, &url_length);
    time_validity = time(NULL) + 60*60*24; // TODO: one day validity for now, should be     
  */ 

  TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "cookie_value: %s", transaction_parts.at(TransactionMuncher::COOKIE).c_str());
  if(!ChallengeManager::check_cookie(transaction_parts.at(TransactionMuncher::COOKIE), transaction_parts.at(TransactionMuncher::IP)))
    {
      TSDebug("banjax", "cookie is not valid, sending challenge");
 
      return FilterResponse(FilterResponse::I_RESPOND);
    }  

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);

}

std::string ChallengeManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{

  long time_validity = time(NULL) + cookie_life_time; // TODO: one day validity for now, should be changed

  return generate_html(transaction_parts.at(TransactionMuncher::IP), time_validity, transaction_parts.at(TransactionMuncher::URL_WITH_HOST));
  /*char* buf = (char *) TSmalloc(buf_str.length()+1);
    strcpy(buf, buf_str.c_str());*/

}
