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
#include <utility>

#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <ts/ts.h>
#include <re2/re2.h>

#include <assert.h>
#include "libcaptcha.c"
using namespace std;

#include "util.h"
#include "base64.h"
#include "cookie_parser.h"
#include "challenge_manager.h"
#include "cookiehash.h"
#include "print.h"

#define COOKIE_SIZE 100

//The name of Challenges should be declared here
//the name of challenges should appear in same order they
//FIXME: this is not the best practice, it is better to make
//CHALLENGE_LIST an array of pairs of name and id
//appears in ChallengType enum in challenge_manager.h
const char* ChallengeDefinition::CHALLENGE_LIST[]      = {"sha_inverse", "captcha",      "auth"};
const char* ChallengeDefinition::CHALLENGE_FILE_LIST[] = {"solver.html", "captcha.html", "auth.html"};

// Substrings of the page that needs to be replaced.
const std::string SUB_TOKEN = "$token";
const std::string SUB_TIME  = "$time";
const std::string SUB_URL   = "$url";
const std::string SUB_ZEROS = "$zeros";

template<class T>
static set<T> vector2set(const vector<T>& source) {
    set<T> target;
    for (auto& item : source) target.insert(item);
    return target;
}

/**
  Overload of the load config
  reads all the regular expressions from the database.
  and compile them
*/
void
ChallengeManager::load_config(const std::string& banjax_dir)
{
  //TODO: we should read the auth password from config and store it somewhere
  print::debug("Loading challenger manager conf");
  try
  {
    //now we compile all of them and store them for later use
    for(const auto& ch : cfg["challenges"]) {
      auto host_challenge_spec = make_shared<HostChallengeSpec>();

      host_challenge_spec->name = ch["name"].as<std::string>();
      print::debug("Loading conf for challenge ", host_challenge_spec->name);

      //it is fundamental to establish what type of challenge we are dealing
      //with
      std::string requested_challenge_type = ch["challenge_type"].as<std::string>();
      host_challenge_spec->challenge_type = challenge_type[requested_challenge_type];

      host_challenge_spec->challenge_validity_period = ch["validity_period"].as<unsigned int>();

      // How much failure are we going to tolerate 0 means infinite tolerance
      if (ch["no_of_fails_to_ban"]) {
        host_challenge_spec->fail_tolerance_threshold = ch["no_of_fails_to_ban"].as<unsigned int>();
      }

      // TODO(oschaaf): host name can be configured twice, and could except here
      std::string challenge_file;

      if (ch["challenge"]) {
        challenge_file = ch["challenge"].as<std::string>();
      }

      // If no file is configured, default to hard coded solver_page.
      if (challenge_file.size() == 0) {
        challenge_file.append(challenge_specs[host_challenge_spec->challenge_type]->default_page_filename);
      }

      std::string challenge_path = banjax_dir + "/" + challenge_file;
      TSDebug(BANJAX_PLUGIN_NAME, "Challenge [%s] uses challenge file [%s]",
              host_challenge_spec->name.c_str(), challenge_path.c_str());

      ifstream ifs(challenge_path);
      host_challenge_spec->challenge_stream.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());

      if (host_challenge_spec->challenge_stream.size() == 0) {
        TSDebug(BANJAX_PLUGIN_NAME, "Warning, [%s] looks empty", challenge_path.c_str());
        TSError("Warning, [%s] looks empty", challenge_path.c_str());
      } else {
        TSDebug(BANJAX_PLUGIN_NAME, "Assigning %d bytes of html for [%s]",
                (int)host_challenge_spec->challenge_stream.size(), host_challenge_spec->name.c_str());
      }

      //Auth challenege specific data
      if (ch["password_hash"]) {
        host_challenge_spec->password_hash = ch["password_hash"].as<string>();
      }

      if (ch["magic_word"]) {
        auto& mw = ch["magic_word"];

        if (mw.IsSequence()) {
          host_challenge_spec->magic_words = vector2set(mw.as<vector<string>>());
        }
        else {
          host_challenge_spec->magic_words.insert(mw.as<string>());
        }
      }

      if (ch["magic_word_exceptions"]) {
        auto exprs = ch["magic_word_exceptions"].as<vector<string>>();

        for (const auto& expr : exprs) {
          host_challenge_spec->magic_word_exceptions.emplace_back(expr);
        }
      }

      if (ch["white_listed_ips"]) {
        auto& ips = ch["white_listed_ips"];
        host_challenge_spec->white_listed_ips = vector2set(ips.as<vector<string>>());
      }

      //here we are updating the dictionary that relate each
      //domain to many challenges
      unsigned int domain_count = ch["domains"].size();

      //now we compile all of them and store them for later use
      for(unsigned int i = 0; i < domain_count; i++) {
        string cur_domain = ch["domains"][i].as<std::string>();
        host_challenges[cur_domain].push_back(host_challenge_spec);
      }
    }

    //we use SHA256 to generate a key from the user passphrase
    //we will use half of the hash as we are using AES128
    string challenger_key = cfg["key"].as<std::string>();

    SHA256((const unsigned char*)challenger_key.c_str(), challenger_key.length(), hashed_key);

    number_of_trailing_zeros = cfg["difficulty"].as<unsigned int>();
  }
  catch(YAML::RepresentationException& e) {
    TSDebug(BANJAX_PLUGIN_NAME, "Bad config for filter %s: %s", BANJAX_FILTER_NAME.c_str(), e.what());
    throw;
  }

  TSDebug(BANJAX_PLUGIN_NAME, "Done loading challenger manager conf");

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

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, cookiestr, strlen(cookiestr));
  SHA256_Final(hash, &sha256);

  unsigned int count = 0;

  for (auto h : hash) {
    // The order of nibbles is reversed in a byte.
    if (h &  16) break; else ++count;
    if (h &  32) break; else ++count;
    if (h &  64) break; else ++count;
    if (h & 128) break; else ++count;
    if (h &   1) break; else ++count;
    if (h &   2) break; else ++count;
    if (h &   4) break; else ++count;
    if (h &   8) break; else ++count;
  }

  //{
  //  char outputBuffer[2 * SHA256_DIGEST_LENGTH + 1];
  //  for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
  //    sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
  //  }
  //  outputBuffer[64] = 0;

  //  TSDebug(BANJAX_PLUGIN_NAME, "SHA256 = %s (%d)", outputBuffer, count);
  //}

  return count >= number_of_trailing_zeros;
}

/**
 * Checks if the second part of the cookie indeed SHA256 of the
 *  challenge token +  SHA256(password)
 * @param  cookie the value of the cookie
 * @return  true if the cookie verifies the challenge
 */
bool ChallengeManager::check_auth_validity(const char* cookiestr, const std::string password_hash)
{
  unsigned long cookie_len = strlen(cookiestr);

  if (cookie_len < COOKIE_B64_LENGTH + password_hash.size())
      return false;

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  SHA256_Update(&sha256, cookiestr, COOKIE_B64_LENGTH);
  SHA256_Update(&sha256, password_hash.c_str(), password_hash.size());

  SHA256_Final(hash, &sha256);

  std::string cookiedata
    = Base64::Decode(cookiestr + COOKIE_B64_LENGTH,
                     cookiestr + cookie_len);

  // Return true if the hashes equal
  return memcmp(cookiedata.c_str(), hash, SHA256_DIGEST_LENGTH) == 0;
}

/**
 * Checks if the cookie is valid: sha256, ip, and time
 * @param  cookie the value of the cookie
 * @param  ip     the client ip
 * @return        true if the cookie is valid
 */
bool ChallengeManager::check_cookie(string answer, const TransactionParts& transaction_parts, const HostChallengeSpec& cookied_challenge) {
  string cookie_jar = transaction_parts.at(TransactionMuncher::COOKIE);
  string ip         = transaction_parts.at(TransactionMuncher::IP);

  CookieParser cookie_parser;
  const char* next_cookie = cookie_jar.c_str();

  while((next_cookie = cookie_parser.parse_a_cookie(next_cookie)) != NULL) {
    if (cookie_parser.name == "deflect") {
      // TODO(inetic): No need to actually make a copy.
      std::string captcha_cookie(cookie_parser.value.begin(), cookie_parser.value.end());

      //Here we check the challenge specific requirement of the cookie
      bool challenge_prevailed;
      switch ((unsigned int) cookied_challenge.challenge_type) {
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
      int expected_length = COOKIE_B64_LENGTH;
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
bool ChallengeManager::replace(string &original, const string& from, const string& to){
  size_t start_pos = original.find(from);
  if(start_pos == string::npos)
    return false;
  original.replace(start_pos, from.length(), to);
  return true;
}

string ChallengeManager::generate_html(
             string ip,
             long t,
             string url,
             const TransactionParts& transaction_parts,
             ChallengerExtendedResponse* response_info)
{
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
    return std::string((const char*)gif, (int)gifsize);
  } else if (ChallengeManager::is_captcha_answer(url)) {
    response_info->response_code = 200;
    response_info->set_content_type("text/html");
    std::string url = transaction_parts.at(TransactionMuncher::URL_WITH_HOST);
    size_t found = url.rfind("__validate/");

    std::string answer = url.substr(found + strlen("__validate/"));
    response_info->response_code = 403;

    if (ChallengeManager::check_cookie(answer, transaction_parts, *response_info->responding_challenge)) {
      response_info->response_code = 200;
      uchar cookie[COOKIE_SIZE];
      GenerateCookie((uchar*)"", (uchar*)hashed_key, t, (uchar*)ip.c_str(), cookie);
      TSDebug(BANJAX_PLUGIN_NAME, "Set cookie: [%.*s] based on ip[%s]", (int)strlen((char*)cookie), (char*)cookie, ip.c_str());
      std::string header;
      header.append("deflect=");
      header.append((char*)cookie);
      header.append("; path=/; HttpOnly");
      response_info->set_cookie_header.append(header.c_str());
      return "OK";
    } else {
      //count it as a failure
      if (response_info->responding_challenge->fail_tolerance_threshold)
        response_info->banned_ip = report_failure(response_info->responding_challenge, transaction_parts);

      if (response_info->banned_ip)
        return "Too many failures";
    }

    return "X";
  }

  // If the challenge is solvig SHA inverse image or auth
  // copy the template
  string page = response_info->responding_challenge->challenge_stream;
  // generate the token
  uchar cookie[COOKIE_SIZE];
  GenerateCookie((uchar*)"", (uchar*)hashed_key, t, (uchar*)ip.c_str(), cookie);
  string token((const char*)cookie);

  TSDebug("banjax", "write cookie [%d]->[%s]", (int)COOKIE_SIZE, token.c_str());
  // replace the time
  string t_str = format_validity_time_for_cookie(t);
  replace(page, SUB_TIME, t_str);
  // replace the token
  replace(page, SUB_TOKEN, token);
  // replace the url
  replace(page, SUB_URL, url);
  // set the correct number of zeros
  replace(page, SUB_ZEROS, to_string(number_of_trailing_zeros));

  return page;
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
  const char *format = "%a, %d %b %Y %T GMT";
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

bool ChallengeManager::is_globally_white_listed(const std::string& ip) const {
  if (!global_white_list) return false;
  return bool(global_white_list->is_white_listed(ip));
}

/**
   overloaded execute to execute the filter, It calls cookie checker
   and if it fails ask for responding by the filter.
*/
FilterResponse
ChallengeManager::on_http_request(const TransactionParts& transaction_parts)
{
  // look up if this host is serving captcha's or not
  TSDebug(BANJAX_PLUGIN_NAME, "Host to be challenged %s", transaction_parts.at(TransactionMuncher::HOST).c_str());
  auto challenges_it = host_challenges.find(transaction_parts.at(TransactionMuncher::HOST));

  if (challenges_it == host_challenges.end()) {
    // No challenge for this host
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

  TSDebug(BANJAX_PLUGIN_NAME, "cookie_value: %s", transaction_parts.at(TransactionMuncher::COOKIE).c_str());

  auto custom_response = [=](const shared_ptr<HostChallengeSpec>& challenge) {
    return FilterResponse(FilterResponse::I_RESPOND,
        new ChallengerExtendedResponse([&](auto... xs) { return this->generate_response(xs...); },
                                       challenge));
  };

  const auto ip = transaction_parts.at(TransactionMuncher::IP);

  for(const auto& cur_challenge : challenges_it->second) {

    if (cur_challenge->white_listed_ips.count(ip)) {
      print::debug("Challenge ", cur_challenge->name, " bypassed for ip: ", ip);
      return FilterResponse(FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
    }

    switch(cur_challenge->challenge_type)
    {
      case ChallengeDefinition::CHALLENGE_CAPTCHA:
        {
          if (is_globally_white_listed(ip)) {
            break;
          }

          TSDebug(BANJAX_PLUGIN_NAME, "captch url is %s", transaction_parts.at(TransactionMuncher::URL_WITH_HOST).c_str());
          if (ChallengeManager::is_captcha_url(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
          //FIXME: This opens the door to attack edge using the captcha url, we probably need to
          //count captcha urls as failures as well.
            return custom_response(cur_challenge);
          } else if (is_captcha_answer(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
            return custom_response(cur_challenge);
          }

          if (ChallengeManager::check_cookie("", transaction_parts, *cur_challenge)) {
            report_success(ip);
            //rather go to next challenge
            continue;
          } else {
            //record challenge failure
            FilterResponse failure_response = custom_response(cur_challenge);
            if (cur_challenge->fail_tolerance_threshold)
              failure_response.response_data->banned_ip = report_failure(cur_challenge, transaction_parts);

          return failure_response;
        }
        break;
      }

      case ChallengeDefinition::CHALLENGE_SHA_INVERSE:
        if (is_globally_white_listed(ip)) {
          break;
        }

        if(!ChallengeManager::check_cookie("", transaction_parts, *cur_challenge))
          {
            TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, sending challenge");

            //record challenge failure
            FilterResponse failure_response = custom_response(cur_challenge);

            auto response_data = failure_response.response_data;

            if (cur_challenge->fail_tolerance_threshold)
              response_data->banned_ip = report_failure(cur_challenge, transaction_parts);

            // We need to clear out the cookie here, to make sure switching from
            // challenge type (captcha->computational) doesn't end up in an infinite reload
            response_data->set_cookie_header.append("deflect=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly");

            return failure_response;
          }
        break;

      case ChallengeDefinition::CHALLENGE_AUTH:
        if(ChallengeManager::check_cookie("", transaction_parts, *cur_challenge)) {
          // Success response in auth means don't serve
          report_success(ip);
          return FilterResponse(FilterResponse::SERVE_FRESH);
        }
        else {
          TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, looking for magic word");

          if (needs_authentication(transaction_parts.at(TransactionMuncher::URL_WITH_HOST), *cur_challenge)) {
            FilterResponse failure_response = custom_response(cur_challenge);

            if (cur_challenge->fail_tolerance_threshold) {
              failure_response.response_data->banned_ip = report_failure(cur_challenge, transaction_parts);
            }

            // We need to clear out the cookie here, to make sure switching from
            // challenge type (captcha->computational) doesn't end up in an infinite reload
            failure_response.response_data->set_cookie_header.append("deflect=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly");
            return failure_response;
          }
          else {
            // This is a normal client reading website not participating in
            // auth challenge, so just go ahead without comment
            break;
          }
        }
        break;
    }
  }

  report_success(ip);
  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
}

bool ChallengeManager::needs_authentication(const std::string& url, const HostChallengeSpec& challenge) const {
    // If the url of the content contains 'magic_word', then the content is protected
    // unless the url also contains a word from 'magic_word_exceptions'.
    bool is_protected = false;

    for (auto& word : challenge.magic_words) {
        if (RE2::PartialMatch(url, word)) {
            is_protected = true;
            break;
        }
    }

    if (!is_protected) return false;

    for (auto& unprotected : challenge.magic_word_exceptions) {
        if (RE2::FullMatch(url, unprotected)) {
            return false;
        }
    }

    return true;
}

std::string ChallengeManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  ChallengerExtendedResponse* extended_response = (ChallengerExtendedResponse*) response_info.response_data;

  if (extended_response->banned_ip) {
    return too_many_failures_message;
  }

  long time_validity = time(NULL) + extended_response->responding_challenge->challenge_validity_period;

  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::IP).c_str());
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::URL_WITH_HOST).c_str());
  TSDebug("banjax", "%s", transaction_parts.at(TransactionMuncher::HOST).c_str());

  string buf_str;

  return generate_html(transaction_parts.at(TransactionMuncher::IP),
                       time_validity,
                       extended_response->alternative_url.empty()
                         ? transaction_parts.at(TransactionMuncher::URL_WITH_HOST)
                         : extended_response->alternative_url,
                       transaction_parts,
                       extended_response);
}

/**
 * Should be called upon failure of providing solution. Checks the ip_database
 * for current number of failure of solution for an ip, increament and store it
 * report to swabber in case of excessive failure
 *
 * @return true if no_of_failures exceeded the threshold
 */
bool
ChallengeManager::report_failure(const std::shared_ptr<HostChallengeSpec>& failed_challenge, const TransactionParts& transaction_parts)
{
  std::string client_ip = transaction_parts.at(TransactionMuncher::IP);
  std::string failed_host = transaction_parts.at(TransactionMuncher::HOST);

  std::pair<bool,FilterState> cur_ip_state;
  bool banned(false);
  cur_ip_state =  ip_database->get_ip_state(client_ip, CHALLENGER_FILTER_ID);
  if (cur_ip_state.first == false) //we failed to read so we can't judge
    return banned;

  if (cur_ip_state.second.size() == 0) {
    cur_ip_state.second.resize(1);
    cur_ip_state.second[0] = 1;
  } else {
    cur_ip_state.second[0]++; //incremet failure
  }

  if (cur_ip_state.second[0] >= failed_challenge->fail_tolerance_threshold) {
    banned = true;
    TransactionParts ats_record_parts = transaction_parts;

    string banning_reason = "failed challenge " + failed_challenge->name + " for host " + failed_host  + " " + to_string(cur_ip_state.second[0]) + " times, " +
      encapsulate_in_quotes(ats_record_parts[TransactionMuncher::URL]) + ", " +
      ats_record_parts[TransactionMuncher::HOST] + ", " +
      encapsulate_in_quotes(ats_record_parts[TransactionMuncher::UA]);

    swabber_interface->ban(client_ip.c_str(), banning_reason);
    //reset the number of failures for future
    //we are not clearing the state cause it is not for sure that
    //swabber ban the ip due to possible failure of acquiring lock
    //cur_ip_state.detail.no_of_failures = 0;
  }
  else { //only report if we haven't report to swabber cause otherwise it nulifies the work of swabber which has forgiven the ip and delete it from db
    ip_database->set_ip_state(client_ip, CHALLENGER_FILTER_ID, cur_ip_state.second);
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
  FilterState cur_ip_state(1);
  ip_database->set_ip_state(client_ip, CHALLENGER_FILTER_ID, cur_ip_state);
}
