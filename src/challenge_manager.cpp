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
#include <openssl/rand.h>

#include <ts/ts.h>

#include <assert.h>
#include "libcaptcha.c"
using namespace std;

#include "base64.h"
#include "cookie_parser.h"
#include "challenge_manager.h"
#include "cookiehash.h"

#define COOKIE_SIZE 100

//The name of Challenges should be declared here
//the name of challenges should appear in same order they 
//FIXME: this is not the best practice, it is better to make 
//CHALLENGE_LIST an array of pairs of name and id
//appears in ChallengType enum in challenge_manager.h
const char* ChallengeDefinition::CHALLENGE_LIST[] = {"sha_inverse",
                                                     "captcha", "auth"};
const char* ChallengeDefinition::CHALLENGE_FILE_LIST[] = {"solver.html",
                                                          "captcha.html", "auth.html"};

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
ChallengeManager::load_config(libconfig::Setting& cfg, const std::string& banjax_dir)
{
  //TODO: we should read the auth password from config and store it somewhere
   try
   {
     const libconfig::Setting &challenger_hosts = cfg["challenges"];
     unsigned int count = challenger_hosts.getLength();

     //now we compile all of them and store them for later use
     for(unsigned int i = 0; i < count; i++) {
       libconfig::Setting& challenge_config = challenger_hosts[i];
       HostChallengeSpec*  host_challenge_spec = new HostChallengeSpec;
       
       host_challenge_spec->name = (const char*)challenge_config["name"];
      
       const libconfig::Setting &challenged_domains = challenge_config["domains"];

       //it is fundamental to establish what type of challenge we are dealing
       //with
       std::string requested_challenge_type = challenge_config["challenge_type"];
       host_challenge_spec->challenge_type = challenge_type[requested_challenge_type];

       host_challenge_spec->challenge_validity_period = (unsigned int)challenge_config["validity_period"];

       //how much failure are we going to tolerate
       //0 means infinite tolerance
       if (challenge_config.exists("no_of_fails_to_ban"))
         host_challenge_spec->fail_tolerance_threshold = (unsigned int)challenge_config["no_of_fails_to_ban"];
       // TODO(oschaaf): host name can be configured twice, and could except here
       std::string challenge_file;
       if (challenge_config.exists("challenge"))
         challenge_file = (const char*)challenge_config["challenge"];

       // If no file is configured, default to hard coded solver_page.
       if (challenge_file.size() == 0) {
         challenge_file.append(challenge_specs[host_challenge_spec->challenge_type]->default_page_filename);
       }
       
       std::string challenge_path = banjax_dir + "/" + challenge_file;
       TSDebug(BANJAX_PLUGIN_NAME, "Challenge [%s] uses challenge file [%s]",
               host_challenge_spec->name.c_str(), challenge_path.c_str());
       
       ifstream ifs(challenge_path);
       host_challenge_spec->challenge_stream.assign((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
       
       if (host_challenge_spec->challenge_stream.size() == 0) {
         TSDebug(BANJAX_PLUGIN_NAME, "Warning, [%s] looks empty", challenge_path.c_str());
         TSError("Warning, [%s] looks empty", challenge_path.c_str());
       } else {  
         TSDebug(BANJAX_PLUGIN_NAME, "Assigning %d bytes of html for [%s]",
                 (int)host_challenge_spec->challenge_stream.size(), host_challenge_spec->name.c_str());
       }

       //Auth challenege specific data
       if (challenge_config.exists("password_hash"))
         host_challenge_spec->password_hash = (const char*)challenge_config["password_hash"];

       if (challenge_config.exists("magic_word"))
         host_challenge_spec->magic_word = (const char*)challenge_config["magic_word"];

       //add it to the host map
       challenge_settings[host_challenge_spec->name] = host_challenge_spec;
       //here we are updating the dictionary that relate each
       //domain to many challenges
       unsigned int domain_count = challenged_domains.getLength();

       //now we compile all of them and store them for later use
       for(unsigned int i = 0; i < domain_count; i++) {
         string cur_domain = (const char*) challenged_domains[i];
         host_challenges[cur_domain].push_back(host_challenge_spec);

       }
     
     }

     //we use SHA256 to generate a key from the user passphrase
     //we will use half of the hash as we are using AES128
     string challenger_key = cfg["key"];

     SHA256((const unsigned char*)challenger_key.c_str(), challenger_key.length(), hashed_key);
 
     number_of_trailing_zeros = cfg["difficulty"];
     assert(!(number_of_trailing_zeros % 4));
     zeros_in_javascript = string(number_of_trailing_zeros / 4, '0');
 
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
 * Checks if the SHA256 of the cookie has the correct number of zeros
 * @param  cookie the value of the cookie
 * @return        true if the SHA256 of the cookie verifies the challenge
 */
bool ChallengeManager::check_sha(const char* cookiestr){
  char outputBuffer[65];

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, cookiestr, strlen(cookiestr));
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
 * Checks if the second part of the cookie indeed SHA256 of the
 *  challenge token +  SHA256(password) 
 * @param  cookie the value of the cookie
 * @return  true if the cookie verifies the challenge
 */
bool ChallengeManager::check_auth_validity(const char* cookiestr, const std::string password_hash)
{
  static const unsigned int b64token_length = (int)((COOKIE_LENGTH*4+3)/3);
  static const unsigned int b64_sha256_length = (int)((SHA256_DIGEST_LENGTH*4+3)/3)+1;
  static const unsigned int to_be_hashed_length = b64token_length+b64_sha256_length;

  unsigned long cookie_len = strlen((char*)cookiestr);
  if (cookie_len <to_be_hashed_length)
      return false;

  //hash it ourselves
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  char to_be_hashed[to_be_hashed_length];
  memcpy(to_be_hashed, cookiestr, b64token_length);
  memcpy(to_be_hashed+b64token_length, password_hash.c_str(), password_hash.size());
  
  SHA256_Update(&sha256, to_be_hashed, to_be_hashed_length);
  SHA256_Final(hash, &sha256);

  //get the hash from the cookie
  char hashed_solution[SHA256_DIGEST_LENGTH];
  std::string cookiedata=Base64::Decode((const char *)cookiestr+b64token_length, (const char *)(cookiestr+cookie_len));

  memcpy(hashed_solution,cookiedata.c_str(),SHA256_DIGEST_LENGTH);

  //now compare
  if (memcmp(hashed_solution, hash, SHA256_DIGEST_LENGTH))
    return false;
   
  return true;

}

/**
 * Checks if the cookie is valid: sha256, ip, and time
 * @param  cookie the value of the cookie
 * @param  ip     the client ip
 * @return        true if the cookie is valid
 */
bool ChallengeManager::check_cookie(string answer, string cookie_jar, string ip, const HostChallengeSpec& cookied_challenge) {
  CookieParser cookie_parser;
  const char* next_cookie = cookie_jar.c_str();
  
  while((next_cookie = cookie_parser.parse_a_cookie(next_cookie)) != NULL) {
    if (!(memcmp(cookie_parser.str, "deflect", (int)(cookie_parser.nam_end - cookie_parser.str)))) {
      std::string captcha_cookie(cookie_parser.val_start, cookie_parser.val_end - cookie_parser.val_start);
      //Here we check the challenge specific requirement of the cookie
      bool challenge_prevailed;
      switch ((unsigned int)(cookied_challenge.challenge_type)) {
        case ChallengeDefinition::CHALLENGE_SHA_INVERSE:
          challenge_prevailed = check_sha(captcha_cookie.c_str());
          break;
        case ChallengeDefinition::CHALLENGE_AUTH:
          challenge_prevailed = check_auth_validity(captcha_cookie.c_str(), cookied_challenge.password_hash);
          break;
        default:
          challenge_prevailed = true;
      }
      //here we check the general validity of the cookie
      int result = 100 /*something not equal to 1, which means OK*/;
      // see GenerateCookie for the length calculation
      int expected_length = (int)(COOKIE_LENGTH*4+3)/3;
      if (captcha_cookie.size() > (size_t)expected_length) {
        captcha_cookie = captcha_cookie.substr(0, expected_length);
      }
      result = ValidateCookie((uchar *)answer.c_str(), (uchar*)hashed_key,
                              time(NULL), (uchar*)ip.c_str(), (uchar *)captcha_cookie.c_str());
      TSDebug(BANJAX_PLUGIN_NAME, "Challenge cookie: [%s] based on ip[%s] - sha_ok [%s] - result: %d (l:%d)",
              captcha_cookie.c_str(), ip.c_str(),
              challenge_prevailed ? "Y" : "N", result, (int)strlen(captcha_cookie.c_str()));
      return challenge_prevailed && result == 1;
    }
  }
  
  return false;
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

void ChallengeManager::generate_html(string ip, long t, string url,
				     const TransactionParts& transaction_parts,
                                     ChallengerExtendedResponse* response_info, string& page){
  if (ChallengeManager::is_captcha_url(url)) {
    unsigned char text[6];
    memset(text, 0, 6);
    
    unsigned char im[70*200];
    unsigned char gif[gifsize];
    captcha(im,text);
    makegif(im,gif);
    
    uchar cookie[COOKIE_SIZE];
    // TODO(oschaaf): 120 seconds for users to answer. configuration!
    time_t curtime=time(NULL)+120;
    GenerateCookie((uchar*)text, (uchar*)hashed_key, curtime, (uchar*)ip.c_str(), cookie);
    TSDebug(BANJAX_PLUGIN_NAME, "generated captcha [%.*s], cookie: [%s]", 6, (const char*)text, (const char*)cookie);
    response_info->response_code = 200;
    response_info->set_content_type("image/gif");
    std::string header;
    header.append("deflect=");
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
    response_info->response_code = 403;

    if (ChallengeManager::check_cookie(answer.c_str(), cookie, transaction_parts.at(TransactionMuncher::IP), *(response_info->responding_challenge))) {      
      response_info->response_code = 200;
      uchar cookie[COOKIE_SIZE];
      // TODO(oschaaf): 2 hour validity. configuration!
      GenerateCookie((uchar*)"", (uchar*)hashed_key, t, (uchar*)ip.c_str(), cookie);
      TSDebug(BANJAX_PLUGIN_NAME, "Set cookie: [%.*s] based on ip[%s]", (int)strlen((char*)cookie), (char*)cookie, ip.c_str());
      std::string header;
      header.append("deflect=");
      header.append((char*)cookie);
      header.append("; path=/; HttpOnly");
      response_info->set_cookie_header.append(header.c_str());      
      page = "OK";
      return;
    }
    
    page = "X";
    return;
  }
  
  //if the challenge is solvig SHA inverse image or auth
  //copy the template
  page = response_info->responding_challenge->challenge_stream;
  // generate the token
  uchar cookie[COOKIE_SIZE];
  GenerateCookie((uchar*)"", (uchar*)hashed_key, t, (uchar*)ip.c_str(), cookie);
  string token((const char*)cookie);

  TSDebug("banjax", "write cookie [%d]->[%s]", (int)COOKIE_SIZE, token.c_str());
  // replace the time
  string t_str = format_validity_time_for_cookie(t);
  replace(page, sub_time, t_str);
  // replace the token
  replace(page, sub_token, token);
  // replace the url
  replace(page, sub_url, url);
  // set the correct number of zeros
  replace(page, sub_zeros, zeros_in_javascript);
}

/**
 * gets a time in long format in future and turn it into browser and human
 * understandable point in time
 */
string
ChallengeManager::format_validity_time_for_cookie(long validity_time)
{
  // set the time in the correct format
  time_t rawtime = (time_t) validity_time;
  struct tm *timeinfo;
  char buffer [30];
  timeinfo = gmtime(&rawtime);
  const char *format = "%a, %d %b %G %T GMT";
  strftime(buffer, 30, format, timeinfo);
  return string(buffer);
}

bool ChallengeManager::is_captcha_url(const std::string& url) {
  size_t found = url.rfind("__captcha");
  return found != std::string::npos;
}
bool ChallengeManager::is_captcha_answer(const std::string& url) {
  size_t found = url.rfind("__validate/");
  return found != std::string::npos;
}

bool ChallengeManager::url_contains_magic_word(const std::string& url, const std::string& magic_word) {
  size_t found = url.rfind(magic_word);
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
  TSDebug(BANJAX_PLUGIN_NAME, "Host to be challenged %s", transaction_parts.at(TransactionMuncher::HOST).c_str());
  HostChallengeMap::iterator challenges_it = host_challenges.find(transaction_parts.at(TransactionMuncher::HOST));
  if (challenges_it == host_challenges.end())
    //no challenge for this host
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);

  TSDebug(BANJAX_PLUGIN_NAME, "cookie_value: %s", transaction_parts.at(TransactionMuncher::COOKIE).c_str());
  //most likely success response is no comment but the filter like auth
  //can change it
  unsigned int success_response = FilterResponse::GO_AHEAD_NO_COMMENT; 

  for(list<HostChallengeSpec*>::iterator it=challenges_it->second.begin(); it != challenges_it->second.end(); it++) {
    HostChallengeSpec* cur_challenge = *it;
    switch((unsigned int)(cur_challenge->challenge_type)) 
      {
      case ChallengeDefinition::CHALLENGE_CAPTCHA:
        {
          if (ChallengeManager::is_captcha_url(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
          //FIXME: This opens the door to attack edge using the captcha url, we probably need to 
          //count captcha urls as failures as well.
            return FilterResponse(FilterResponse::I_RESPOND, (void*) new ChallengerExtendedResponse(challenger_resopnder, cur_challenge));
          } else if (is_captcha_answer(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
            return FilterResponse(FilterResponse(FilterResponse::I_RESPOND, (void*) new ChallengerExtendedResponse(challenger_resopnder, cur_challenge)));
          }
          if (ChallengeManager::check_cookie("", transaction_parts.at(TransactionMuncher::COOKIE), transaction_parts.at(TransactionMuncher::IP), *(cur_challenge))) {
            report_success(transaction_parts.at(TransactionMuncher::IP));
            //rather go to next challenge
            continue;
        } else {
          //record challenge failure
          FilterResponse failure_response(FilterResponse::I_RESPOND, (void*) new ChallengerExtendedResponse(challenger_resopnder, cur_challenge));
          if (cur_challenge->fail_tolerance_threshold)
            ((FilterExtendedResponse*)(failure_response.response_data))->banned_ip = report_failure(transaction_parts.at(TransactionMuncher::IP), cur_challenge, transaction_parts.at(TransactionMuncher::HOST));
            
          return failure_response;
        }
        break;
      }

      case ChallengeDefinition::CHALLENGE_SHA_INVERSE:
        if(!ChallengeManager::check_cookie("", transaction_parts.at(TransactionMuncher::COOKIE), transaction_parts.at(TransactionMuncher::IP), *(cur_challenge)))
          {
            TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, sending challenge");
            //record challenge failure
            FilterResponse failure_response(FilterResponse::I_RESPOND, (void*) new ChallengerExtendedResponse(challenger_resopnder, cur_challenge));
            if (cur_challenge->fail_tolerance_threshold)
              ((FilterExtendedResponse*)(failure_response.response_data))->banned_ip = report_failure(transaction_parts.at(TransactionMuncher::IP), cur_challenge, transaction_parts.at(TransactionMuncher::HOST));
          // We need to clear out the cookie here, to make sure switching from
          // challenge type (captcha->computational) doesn't end up in an infinite reload
            ((FilterExtendedResponse*)(failure_response.response_data))->set_cookie_header.append("deflect=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly");
            return failure_response;
          }
        break;

      case ChallengeDefinition::CHALLENGE_AUTH:
        if(!ChallengeManager::check_cookie("", transaction_parts.at(TransactionMuncher::COOKIE), transaction_parts.at(TransactionMuncher::IP), *(cur_challenge)))
          {
            TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, looking for magic word");
            //from cache
            //If the url has the magic word to activate the auth challenge
          //    Check if the auth cookie is valid
          //      return FilterResponse(FilterResponse::SERVE_FRESH);
          //    eles
          //      do all failure/banning ritual?
          //record challenge failure
            if (ChallengeManager::url_contains_magic_word(transaction_parts.at(TransactionMuncher::URL_WITH_HOST), cur_challenge->magic_word)) {
              FilterResponse failure_response(FilterResponse::I_RESPOND, (void*) new ChallengerExtendedResponse(challenger_resopnder, cur_challenge));
              if (cur_challenge->fail_tolerance_threshold)
                ((FilterExtendedResponse*)(failure_response.response_data))->banned_ip = report_failure(transaction_parts.at(TransactionMuncher::IP), cur_challenge, transaction_parts.at(TransactionMuncher::HOST));
            // We need to clear out the cookie here, to make sure switching from
            // challenge type (captcha->computational) doesn't end up in an infinite reload
              ((FilterExtendedResponse*)(failure_response.response_data))->set_cookie_header.append("deflect=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly");
              return failure_response;
            }
            else { //This is a normal client reading website not participating in
            //auth challenge, so just go ahead without comment
              break;
            }
          }

        //here means auth cookie was good
        success_response = FilterResponse::SERVE_FRESH; //success response in auth means don't serve
        goto done_with_challenges;
        break;
      }
  }

done_with_challenges:
  report_success(transaction_parts.at(TransactionMuncher::IP));
  return FilterResponse(success_response);

}

std::string ChallengeManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{

  ChallengerExtendedResponse* extended_response =  (ChallengerExtendedResponse*)(response_info.response_data);
  if ((extended_response)->banned_ip) {
    char* too_many_failures_response = new char[too_many_failures_message_length+1];
    memcpy((void*)too_many_failures_response, (const void*)too_many_failures_message.c_str(), (too_many_failures_message_length+1)*sizeof(char));
    return too_many_failures_response;
  }

  long time_validity = time(NULL) + extended_response->responding_challenge->challenge_validity_period; 

  string buf_str; 
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::IP).c_str());
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::URL_WITH_HOST).c_str());
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::HOST).c_str());

  (void) response_info;
  TransactionParts test_muncher;
  generate_html(transaction_parts.at(TransactionMuncher::IP), time_validity, 
                ((extended_response)->alternative_url.empty()) ? transaction_parts.at(TransactionMuncher::URL_WITH_HOST) : ((extended_response)->alternative_url),
                transaction_parts,
                //NULL, buf_str);//, 
                extended_response, buf_str);
  return buf_str;

}

/**
 * Should be called upon failure of providing solution. Checks the ip_database
 * for current number of failure of solution for an ip, increament and store it
 * report to swabber in case of excessive failure
 *
 * @param client_ip: string representing the failed requester ip
 * @param failed_host_spec: the specification of the challenge for the host 
 *        that client failed to solve
 *
 * @return true if no_of_failures exceeded the threshold
 */
bool
ChallengeManager::report_failure(std::string client_ip, HostChallengeSpec* failed_challenge, std::string failed_host)
{
  ChallengerStateUnion cur_ip_state;
  bool banned(false);
  cur_ip_state.state_allocator =  ip_database->get_ip_state(client_ip, CHALLENGER_FILTER_ID);
  cur_ip_state.detail.no_of_failures++; //incremet failure

  if (cur_ip_state.detail.no_of_failures >= failed_challenge->fail_tolerance_threshold) {
    banned = true;
    string banning_reason = "failed challenge " + failed_challenge->name + "of type "+  challenge_specs[failed_challenge->challenge_type]->human_readable_name + " " + "for host " + failed_host  + " " + to_string(cur_ip_state.detail.no_of_failures) + " times";
    swabber_interface->ban(client_ip.c_str(), banning_reason);
    //reset the number of failures for future
    //we are not clearing the state cause it is not for sure that
    //swabber ban the ip due to possible failure of acquiring lock
    //cur_ip_state.detail.no_of_failures = 0;
  }
  else { //only report if we haven't report to swabber cause otherwise it nulifies the work of swabber which has forgiven the ip and delete it from db
    ip_database->set_ip_state(client_ip, CHALLENGER_FILTER_ID, cur_ip_state.state_allocator);
  }

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
