/*
 * Functions deal with the js challenge
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Ben: July 2013, Initial version
 *  Otto: Nov 2013: Captcha challenge
 *  Vmon: Nov 2013: merging Otto's code
 */
#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <streambuf>
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
#include "libcaptcha.c"
using namespace std;

#include "base64.h"
#include "cookie_parser.h"
#include "challenge_manager.h"
#include "cookiehash.h"

//The name of Challenges should be declared here
//the name of challenges should appear in same order they 
//appears in ChallengType enum in challenge_manager.h
const char* ChallengeDefinition::CHALLENGE_LIST[] = {"sha_inverse",
                                                      "captcha"};
const char* ChallengeDefinition::CHALLENGE_FILE_LIST[] = {"solver.html",
                                     "captcha.html"};

string ChallengeManager::zeros_in_javascript = "00"; // needs to be in accordance with the number above

std::string ChallengeManager::sub_token = "$token";
std::string ChallengeManager::sub_time = "$time";
std::string ChallengeManager::sub_url = "$url";
std::string ChallengeManager::sub_zeros = "$zeros";

// TODO(oschaaf): configuration
const char* CAPTCHA_SECRET = "12345";

/**
  Overload of the load config
  reads all the regular expressions from the database.
  and compile them
*/
void
ChallengeManager::load_config(libconfig::Setting& cfg, const std::string& banjax_dir)
{
   try
   {
     const libconfig::Setting &challenger_hosts = cfg["hosts"];
     unsigned int count = challenger_hosts.getLength();

     //now we compile all of them and store them for later use
     for(unsigned int i = 0; i < count; i++) {
       libconfig::Setting& host_config = challenger_hosts[i];
       HostChallengeSpec* host_challenge_spec = new HostChallengeSpec;
       
       host_challenge_spec->host_name = (const char*)host_config["name"];

       //it is fundamental to establish what type of challenge we are dealing
       //with
       std::string requested_challenge_type = host_config["challenge_type"];
       host_challenge_spec->challenge_type = challenge_type[requested_challenge_type];

       //how much failure are we going to tolerate
       //0 means infinite tolerance
       if (host_config.exists("no_of_fails_to_ban"))
         host_challenge_spec->fail_tolerance_threshold = (unsigned int)host_config["no_of_fails_to_ban"];
       // TODO(oschaaf): host name can be configured twice, and could except here
       std::string challenge_file;
       if (host_config.exists("challenge"))
         challenge_file = (const char*)host_config["challenge"];

       // If no file is configured, default to hard coded solver_page.
       if (challenge_file.size() == 0) {
         challenge_file.append(challenge_specs[host_challenge_spec->challenge_type]->default_page_filename);
       }
       
       std::string challenge_path = banjax_dir + "/" + challenge_file;
       TSDebug(BANJAX_PLUGIN_NAME, "Host [%s] uses challenge file [%s]",
               host_challenge_spec->host_name.c_str(), challenge_path.c_str());
       
       ifstream ifs(challenge_path);
       host_challenge_spec->challenge_stream.assign((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
       
       if (host_challenge_spec->challenge_stream.size() == 0) {
         TSDebug(BANJAX_PLUGIN_NAME, "Warning, [%s] looks empty", challenge_path.c_str());
         TSError("Warning, [%s] looks empty", challenge_path.c_str());
       } else {  
         TSDebug(BANJAX_PLUGIN_NAME, "Assigning %d bytes of html for [%s]",
                 (int)host_challenge_spec->challenge_stream.size(), host_challenge_spec->host_name.c_str());
       }

       //add it to the host map
       host_settings_[host_challenge_spec->host_name] = host_challenge_spec;
       
     }

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
   catch(const libconfig::SettingNotFoundException &nfex) {
     TSDebug(BANJAX_PLUGIN_NAME, "Bad config for filter %s", BANJAX_FILTER_NAME.c_str());
   }

   // load the page
    //ifstream ifs("../challenger/solver.html");
   //TODO: Should not re-read the fiel upon each request
   //We need to read the whole string from the database infact
  //  ifstream ifs(ChallengeManager::solver_page.c_str());
  //  solver_page.assign( (istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());

  // // set the time in the correct format
  // stringstream ss(challenge_html.c_str());
  // string page( (istreambuf_iterator<char>(ss) ), (istreambuf_iterator<char>()) );
  // TSDebug(BANJAX_PLUGIN_NAME,
  //         "ChallengeManager::generate_html lookup for host [%s] found %d bytes of html.",
  //         host_header.c_str(), (int)page.size());


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
  string result = Base64::Encode((char*)encrypted_token);
 
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

  //TSDebug(BANJAX_PLUGIN_NAME, "SHA256 = %s", outputBuffer);

  for(unsigned i=0; i<number_of_trailing_zeros/4; i++){
    if(outputBuffer[i] != '0'){
      //TSDebug(BANJAX_PLUGIN_NAME, "i= %d, out = %c", i, outputBuffer[i]);
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

  TSDebug(BANJAX_PLUGIN_NAME, "cookie_val: '%s'",cookie_parser.val_start);

  // check the SHA256 of the cookie
  if(!check_sha(cookie_parser.val_start, cookie_parser.val_end)){
    TSDebug(BANJAX_PLUGIN_NAME, "ChMng: SHA256 not valid");
    return false;
  }

  // separate token from user-found value (token has length 22 in base64 encoding)
  string token = Base64::Decode(cookie_parser.val_start, cookie_parser.val_end);

    // unencode token and separate ip from time
  unsigned char original[AES_BLOCK_SIZE];
  AES_decrypt((const unsigned char*)token.c_str(), original, &dec_key);

  // check the ip against the client's ip
  vector<string> x = split(ip, '.');
  for(unsigned int i=0; i<x.size(); i++){
    if ( ((int) *(original+i)) != atoi(x[i].c_str()) ){
      TSDebug(BANJAX_PLUGIN_NAME, "ChMng: ip not valid");
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
    TSDebug(BANJAX_PLUGIN_NAME, "ChMng: time not valid");
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

void ChallengeManager::generate_html(string ip, long t, string url, string host_header,                                       const TransactionParts& transaction_parts,
                                       FilterExtendedResponse* response_info, string& page){
  
  if (ChallengeManager::is_captcha_url(url)) {
    unsigned char text[6];
    memset(text, 0, 6);
    
    unsigned char im[70*200];
    unsigned char gif[gifsize];
    captcha(im,text);
    makegif(im,gif);
    
    uchar cookie[100];
    // TODO(oschaaf): 120 seconds for users to answer. configuration!
    time_t curtime=time(NULL)+120;
    GenerateCookie((uchar*)text, (uchar*)CAPTCHA_SECRET, curtime, (uchar*)ip.c_str(), cookie);
    TSDebug(BANJAX_PLUGIN_NAME, "generated captcha [%.*s], cookie: [%s]", 6, (const char*)text, (const char*)cookie);
    response_info->response_code = 200;
    response_info->set_content_type("image/gif");
    std::string header;
    header.append("dflct_c_token=");
    header.append((char*)cookie);
    header.append("; path=/; HttpOnly");
    response_info->set_cookie_header.append(header.c_str());
    page = std::string((const char*)gif, (int)gifsize);
    return;
  } else if (ChallengeManager::is_captcha_answer(url)) {
    response_info->response_code = 200;
    response_info->set_content_type("text/html");
    std::string url = transaction_parts.at(TransactionMuncher::URL_WITH_HOST);
    size_t found = url.rfind("__validate/");

    std::string cookie = transaction_parts.at(TransactionMuncher::COOKIE);
    std::string answer = url.substr(found + strlen("__validate/"));
    int result = -100;
    response_info->response_code = 403;
    CookieParser cookie_parser;
    const char* next_cookie = cookie.c_str();
    while((next_cookie = cookie_parser.parse_a_cookie(next_cookie)) != NULL) {
      if (!(memcmp(cookie_parser.str, "dflct_c_token", (int)(cookie_parser.nam_end - cookie_parser.str)))) {
        std::string captcha_cookie(cookie_parser.val_start, cookie_parser.val_end - cookie_parser.val_start);
        TSDebug(BANJAX_PLUGIN_NAME, "Challenge cookie: [%s] based on ip[%s]", captcha_cookie.c_str(), transaction_parts.at(TransactionMuncher::IP).c_str());
        time_t curtime = time(NULL);
        result = ValidateCookie((uchar *)answer.c_str(), (uchar*)CAPTCHA_SECRET,
                                curtime, (uchar*)ip.c_str(), (uchar *)captcha_cookie.c_str());        
        if (result == 1) {
          response_info->response_code = 200;
          uchar cookie[100];
          // TODO(oschaaf): 2 hour validity. configuration!
          GenerateCookie((uchar*)"", (uchar*)CAPTCHA_SECRET, curtime + 7200, (uchar*)ip.c_str(), cookie);
          TSDebug(BANJAX_PLUGIN_NAME, "Set cookie: [%.*s] based on ip[%s]", 100, (char*)cookie, ip.c_str());
          std::string header;
          header.append("dflct_c_token=");
          header.append((char*)cookie);
          header.append("; path=/; HttpOnly");
          response_info->set_cookie_header.append(header.c_str());

          //no return
          page = "OK";
          return;
        }
        break;
      }
    }
    TSDebug(BANJAX_PLUGIN_NAME, "Intercept captcha answer: [%s], cookie: [%s], validate: [%d]", answer.c_str(), cookie_parser.val_start, result);    
    
    page = "X";
    return;
  }
  
  //if the challenge is solvig SHA inverse image
  //copy the template
  page = host_settings_[transaction_parts.at(TransactionMuncher::HOST)]->challenge_stream;
  // generate the token
  string token = ChallengeManager::generate_token(ip, t);
  HostSettingsMap::iterator it = host_settings_.find(host_header);
  // TODO(oschaaf): see if we can avoid the copy here
  string challenge_html;
  if (it != host_settings_.end()) {
    //HostChallengeSpec* setting = it->second; //this looks redundant for 
    //now as we don't do anything related to the host but in future this will
    //be used for validity time, etc.

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
  }
}

bool ChallengeManager::is_captcha_url(const std::string& url) {
  size_t found = url.rfind("__captcha");
  return found != std::string::npos;
}
bool ChallengeManager::is_captcha_answer(const std::string& url) {
  size_t found = url.rfind("__validate/");
  return found != std::string::npos;
}

/**
   overloaded execute to execute the filter, It calls cookie checker
   and if it fails ask for responding by the filter.
*/
FilterResponse 
ChallengeManager::execute(const TransactionParts& transaction_parts)
{
  // look up if this host is serving captcha's or not
  HostSettingsMap::iterator it = host_settings_.find(transaction_parts.at(TransactionMuncher::HOST));
  if (it == host_settings_.end())
    //no challenge for this host
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);

  /*
   * checking the cookie and serving the js challenge if does not pass
   */
  TSDebug(BANJAX_PLUGIN_NAME, "cookie_value: %s", transaction_parts.at(TransactionMuncher::COOKIE).c_str());

  switch((unsigned int)(it->second->challenge_type)) 
    {
    case ChallengeDefinition::CHALLENGE_CAPTCHA:
      {
        if (ChallengeManager::is_captcha_url(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
          return FilterResponse(static_cast<ResponseGenerator>(&ChallengeManager::generate_response));
        } else if (is_captcha_answer(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
          return FilterResponse(static_cast<ResponseGenerator>(&ChallengeManager::generate_response));
        }
        
        CookieParser cookie_parser;
        std::string cookie = transaction_parts.at(TransactionMuncher::COOKIE);
        const char* next_cookie = cookie.c_str();
        int result = 100;
        time_t curtime = time(NULL);
        while((next_cookie = cookie_parser.parse_a_cookie(next_cookie)) != NULL) {
          if (!(memcmp(cookie_parser.str, "dflct_c_token", (int)(cookie_parser.nam_end - cookie_parser.str)))) {
            std::string captcha_cookie(cookie_parser.val_start, cookie_parser.val_end - cookie_parser.val_start);
            TSDebug(BANJAX_PLUGIN_NAME, "Challenge cookie: [%s] based on ip[%s]", captcha_cookie.c_str(), transaction_parts.at(TransactionMuncher::IP).c_str());
            result = ValidateCookie((uchar*)"", (uchar*)CAPTCHA_SECRET,
                                    curtime, (uchar*)transaction_parts.at(TransactionMuncher::IP).c_str(),
                                    (uchar*)captcha_cookie.c_str());
            break;
          }
        }
        if (result == 1) {
          report_success(transaction_parts.at(TransactionMuncher::IP));
          return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
        } else {
          //record challenge failure
          FilterResponse failure_response(static_cast<ResponseGenerator>(&ChallengeManager::generate_response));
          if (it->second->fail_tolerance_threshold)
            ((FilterExtendedResponse*)(failure_response.response_data))->banned_ip = report_failure(transaction_parts.at(TransactionMuncher::IP), ((unsigned int)(it->second)->fail_tolerance_threshold));
            
          return failure_response;
        }
        break;
      }

    case ChallengeDefinition::CHALLENGE_SHA_INVERSE:
      if(!ChallengeManager::check_cookie(transaction_parts.at(TransactionMuncher::COOKIE), transaction_parts.at(TransactionMuncher::IP)))
        {
          TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, sending challenge");
          //record challenge failure
          FilterResponse failure_response(static_cast<ResponseGenerator>(&ChallengeManager::generate_response));
          if (it->second->fail_tolerance_threshold)
            ((FilterExtendedResponse*)(failure_response.response_data))->banned_ip = report_failure(transaction_parts.at(TransactionMuncher::IP), it->second->fail_tolerance_threshold);
            
          return failure_response;
        }
    }

  report_success(transaction_parts.at(TransactionMuncher::IP));
  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
}

std::string ChallengeManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{

  if (((FilterExtendedResponse*)(response_info.response_data))->banned_ip) {
    char* too_many_failures_response = new char[too_many_failures_message_length+1];
    memcpy((void*)too_many_failures_response, (const void*)too_many_failures_message.c_str(), (too_many_failures_message_length+1)*sizeof(char));
    return too_many_failures_response;
  }

  long time_validity = time(NULL) + cookie_life_time; // TODO: one day validity for now, should be changed

  string buf_str; 
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::IP).c_str());
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::URL_WITH_HOST).c_str());
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::HOST).c_str());

  (void) response_info;
  TransactionParts test_muncher;
  generate_html(transaction_parts.at(TransactionMuncher::IP), time_validity, 
                transaction_parts.at(TransactionMuncher::URL_WITH_HOST),
                transaction_parts.at(TransactionMuncher::HOST), transaction_parts,
                //NULL, buf_str);//, 
            ((FilterExtendedResponse*)(response_info.response_data)), buf_str);
  return buf_str;

}

/**
 * Should be called upon failure of providing solution. Checks the ip_database
 * for current number of failure of solution for an ip, increament and store it
 * report to swabber in case of excessive failure
 *
 * @param client_ip: string representing the failed requester ip
 *
 * @return true if no_of_failures exceeded the threshold
 */
bool
ChallengeManager::report_failure(std::string client_ip, unsigned int host_failure_threshold)
{

  ChallengerStateUnion cur_ip_state;
  bool banned(false);
  cur_ip_state.state_allocator =  ip_database->get_ip_state(client_ip, CHALLENGER_FILTER_ID);
  cur_ip_state.detail.no_of_failures++; //incremet failure

  if (cur_ip_state.detail.no_of_failures >= host_failure_threshold) {
    banned = true;
    swabber_interface->ban(client_ip.c_str(), "excessive challenge failure");
    //reset the number of failures for future
    cur_ip_state.detail.no_of_failures = 0;
  }

  ip_database->set_ip_state(client_ip, CHALLENGER_FILTER_ID, cur_ip_state.state_allocator);
  return banned;

}

/**
 * Should be called upon successful solution of a challenge to wipe up the
 * the ip's failure record
 *
 * @param client_ip: string representing the failed requester ip
 */
void 
ChallengeManager::report_success(std::string client_ip)
{
  ChallengerStateUnion cur_ip_state;
  ip_database->set_ip_state(client_ip, CHALLENGER_FILTER_ID, cur_ip_state.state_allocator);

}
