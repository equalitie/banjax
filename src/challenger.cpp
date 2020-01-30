/*
 * Functions deal with the js challenge
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Ben: July 2013, Initial version
 *  Otto: Nov 2013: Captcha challenge
 *  Vmon: Nov 2013: merging Otto's code
 */


#include <re2/re2.h>

#include "libcaptcha.c"
#include "base64.h"
#include "cookie.h"
#include "challenger.h"
#include "cookiehash.h"
#include "kafka.h"

using namespace std;

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

/**
  Overload of the load config
  reads all the regular expressions from the database.
  and compile them
*/
void
Challenger::load_config()
{
  //TODO: we should read the auth password from config and store it somewhere
  DEBUG("Loading challenger manager conf");
  try
  {
    //now we compile all of them and store them for later use
    for(const auto& ch : cfg["challenges"]) {
      const shared_ptr<HostChallengeSpec>& host_challenge_spec = parse_single_challenge(ch);
      //here we are updating the dictionary that relate each
      //domain to many challenges
      unsigned int domain_count = ch["domains"].size();

      //now we compile all of them and store them for later use
      for(unsigned int i = 0; i < domain_count; i++) {
        string cur_domain = ch["domains"][i].as<std::string>();
        host_challenges_static[cur_domain].push_back(host_challenge_spec);
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
}

void
Challenger::load_single_dynamic_config(const std::string& domain, const std::string& config_string) {
  print::debug("LOAD SINGLE: ", config_string);
  try
  {
      YAML::Node challenge = YAML::Load(config_string);
      auto challenge_spec = parse_single_challenge(challenge);
      TSMutexLock(host_to_challenge_dynamic_mutex);
      auto on_scope_exit = defer([&] { TSMutexUnlock(host_to_challenge_dynamic_mutex); });
      host_to_challenge_dynamic[domain] = challenge_spec;
  }
  catch(YAML::RepresentationException& e) {
    TSDebug(BANJAX_PLUGIN_NAME, "Challenger::load_single_dynamic_config error: %s", e.what());
    throw;
  }
}

void
Challenger::remove_expired_challenges() {
  time_t current_timestamp = time(NULL);
  TSMutexLock(host_to_challenge_dynamic_mutex);
  auto on_scope_exit = defer([&] { TSMutexUnlock(host_to_challenge_dynamic_mutex); });
  for (auto it = host_to_challenge_dynamic.begin(); it != host_to_challenge_dynamic.end();) {
      // XXX is this really the best way to remove from a map? std::remove_if?
      // XXX move the 60 seconds thing into a config
      print::debug("host: ", it->first, " born: ", it->second->time_added);
      if (current_timestamp - it->second->time_added > 60) {
          it = host_to_challenge_dynamic.erase(it);
      } else {
          ++it;
      }
  }
}

/**
 * Splits a string according to a delimiter
 */
vector<string> Challenger::split(const string &s, char delim) {
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
bool Challenger::check_sha(const char* cookiestr) const {

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
bool Challenger::check_auth_validity(const char* cookiestr, const std::string password_hash)
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
bool Challenger::check_cookie(string answer, const TransactionParts& transaction_parts, const HostChallengeSpec& cookied_challenge) const {
  string cookie_jar = transaction_parts.at(TransactionMuncher::COOKIE);
  string ip         = transaction_parts.at(TransactionMuncher::IP);

  DEBUG("Challenger::check_cookie: cookie_jar = ", cookie_jar);

  boost::string_view cookie_jar_s = cookie_jar;

  while(boost::optional<Cookie> cookie = Cookie::consume(cookie_jar_s)) {
    if (cookie->name == "deflect") {
      // TODO(inetic): No need to actually make a copy.
      std::string captcha_cookie(cookie->value.begin(), cookie->value.end());

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

  DEBUG("Challenger::check_cookie: 'deflect' cookie not found");
  return false;
}

/**
 * Replaces a substring by another
 * @param original string to be modified
 * @param from     substing to be replaced
 * @param to       what to replace by
 */
/* static */
bool Challenger::replace(string &original, const string& from, const string& to){
  size_t start_pos = original.find(from);
  if(start_pos == string::npos)
    return false;
  original.replace(start_pos, from.length(), to);
  return true;
}

string Challenger::generate_html(
             string ip,
             long t,
             string url,
             const TransactionParts& transaction_parts,
             ChallengerExtendedResponse* response_info)
{
  if (Challenger::is_captcha_url(url)) {
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
  } else if (Challenger::is_captcha_answer(url)) {
    response_info->response_code = 200;
    response_info->set_content_type("text/html");
    std::string url = transaction_parts.at(TransactionMuncher::URL_WITH_HOST);
    size_t found = url.rfind("__validate/");

    std::string answer = url.substr(found + strlen("__validate/"));
    response_info->response_code = 403;

    if (Challenger::check_cookie(answer, transaction_parts, *response_info->responding_challenge)) {
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
/* static */
string
Challenger::format_validity_time_for_cookie(long validity_time)
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

/* static */
bool Challenger::is_captcha_url(const std::string& url) {
  size_t found = url.rfind("__captcha");
  return found != std::string::npos;
}
/* static */
bool Challenger::is_captcha_answer(const std::string& url) {
  size_t found = url.rfind("__validate/");
  return found != std::string::npos;
}

bool Challenger::is_globally_white_listed(const std::string& ip) const {
  if (!global_white_list) return false;
  return bool(global_white_list->is_white_listed(ip));
}

/**
   overloaded execute to execute the filter, It calls cookie checker
   and if it fails ask for responding by the filter.
*/
FilterResponse
Challenger::on_http_request(const TransactionParts& transaction_parts)
{
  const auto& host = transaction_parts.at(TransactionMuncher::HOST);

  // look up if this host is serving captcha's or not
  TSDebug(BANJAX_PLUGIN_NAME, "Host to be challenged %s", host.c_str());

  std::list<std::shared_ptr<HostChallengeSpec>> challenges_to_run;

  auto challenges_it = host_challenges_static.find(host);  // <- on-disk challenge gets priority

  // XXX omfg what a mess
  if (challenges_it == host_challenges_static.end()) {
    print::debug("No on-disk challenge for host: ", host);
    if (TSMutexLockTry(host_to_challenge_dynamic_mutex) != TS_SUCCESS) {
      print::debug("Unable to get lock for host_to_challenge_dynamic; skipping challenger");
      return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
    } else {
      auto on_scope_exit = defer([&] { TSMutexUnlock(host_to_challenge_dynamic_mutex); });
      print::debug("Got lock for host_to_challenge_dynamic; checking inside");
      auto dyn_challenges_it = host_to_challenge_dynamic.find(host);
      if (dyn_challenges_it == host_to_challenge_dynamic.end()) {
        print::debug("No from-kafka challenge for host: ", host);
        return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
      }
      challenges_to_run.push_front(dyn_challenges_it->second);
    }
  } else {
    challenges_to_run = challenges_it->second;
  }

  DEBUG("cookie_value: %s", transaction_parts.at(TransactionMuncher::COOKIE));

  auto custom_response = [=](const shared_ptr<HostChallengeSpec>& challenge) {
    return FilterResponse(FilterResponse::I_RESPOND,
        new ChallengerExtendedResponse([&](const TransactionParts& a, const FilterResponse& b) { return this->generate_response(a,b); },
                                       challenge));
  };

  const auto ip = transaction_parts.at(TransactionMuncher::IP);

  DEBUG(">>> Num challenges ", challenges_to_run.size());

  for(const auto& cur_challenge : challenges_to_run) {

    TSDebug(BANJAX_PLUGIN_NAME, "set size: %lu", cur_challenge->white_listed_ips.size());
    for (const auto& ipr : cur_challenge->white_listed_ips) {
      TSDebug(BANJAX_PLUGIN_NAME, "000a");
      if (is_match(ip, ipr)) {
        return FilterResponse(FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
      }
    }

    switch(cur_challenge->challenge_type)
    {
      TSDebug(BANJAX_PLUGIN_NAME, "002");
      case ChallengeDefinition::CHALLENGE_CAPTCHA:
        TSDebug(BANJAX_PLUGIN_NAME, ">>> Running challenge CHALLENGE_CAPTCHA");
        {
          if (is_globally_white_listed(ip)) {
            break;
          }

          TSDebug(BANJAX_PLUGIN_NAME, "captch url is %s", transaction_parts.at(TransactionMuncher::URL_WITH_HOST).c_str());
          if (Challenger::is_captcha_url(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
          //FIXME: This opens the door to attack edge using the captcha url, we probably need to
          //count captcha urls as failures as well.
            return custom_response(cur_challenge);
          } else if (is_captcha_answer(transaction_parts.at(TransactionMuncher::URL_WITH_HOST))) {
            return custom_response(cur_challenge);
          }

          if (Challenger::check_cookie("", transaction_parts, *cur_challenge)) {
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
        TSDebug(BANJAX_PLUGIN_NAME, ">>> Running challenge SHA_INVERSE");
        if (is_globally_white_listed(ip)) {
          break;
        }

        if(!Challenger::check_cookie("", transaction_parts, *cur_challenge))
          {
            TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, sending challenge");

            //record challenge failure
            FilterResponse failure_response = custom_response(cur_challenge);

            auto& response_data = failure_response.response_data;

            if (cur_challenge->fail_tolerance_threshold)
              response_data->banned_ip = report_failure(cur_challenge, transaction_parts);

            // We need to clear out the cookie here, to make sure switching from
            // challenge type (captcha->computational) doesn't end up in an infinite reload
            response_data->set_cookie_header.append("deflect=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly");

            return failure_response;
          }
        break;

      case ChallengeDefinition::CHALLENGE_AUTH:
        TSDebug(BANJAX_PLUGIN_NAME, ">>> Running challenge CHALLENGE_AUTH");
        if(Challenger::check_cookie("", transaction_parts, *cur_challenge)) {
          // Success response in auth means don't serve
          report_success(ip);
          return FilterResponse(FilterResponse::SERVE_FRESH);
        }
        else {
          TSDebug(BANJAX_PLUGIN_NAME, "cookie is not valid, looking for magic word");

          if (needs_authentication(transaction_parts.at(TransactionMuncher::URL_WITH_HOST), *cur_challenge)) {
            TSDebug(BANJAX_PLUGIN_NAME, "needs authentication");
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
            TSDebug(BANJAX_PLUGIN_NAME, "does not need authentication");
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

bool HostChallengeSpec::MagicWord::is_match(const std::string& url) const {
  switch(type) {
    case HostChallengeSpec::MagicWord::regexp:
      return RE2::PartialMatch(url, magic_word);
    case HostChallengeSpec::MagicWord::substr:
      return url.find(magic_word) != std::string::npos;
  }
  return false;
}

bool Challenger::needs_authentication(const std::string& url, const HostChallengeSpec& challenge) const {
    // If the url of the content contains 'magic_word', then the content is protected
    // unless the url also contains a word from 'magic_word_exceptions'.
    bool is_protected = false;

    TSDebug(BANJAX_PLUGIN_NAME, "should be 'checking' here");
    TSDebug(BANJAX_PLUGIN_NAME, "set size %lu", challenge.magic_words.size());
    for (auto& word : challenge.magic_words) {
      TSDebug(BANJAX_PLUGIN_NAME, "checking against %s", word.magic_word.c_str());
      if (word.is_match(url)) {
        is_protected = true;
        TSDebug(BANJAX_PLUGIN_NAME, "MATCHED A MAGIC WORD");
        break;
      }
    }

    if (!is_protected) return false;

    auto end = url.end();
    auto query_start = url.find('?');

    if (query_start != std::string::npos) {
      end = url.begin() + query_start;
    }

    for (auto& unprotected : challenge.magic_word_exceptions) {
      if (std::search(url.begin(), end, unprotected.begin(), unprotected.end()) != end) {
        TSDebug(BANJAX_PLUGIN_NAME, "MATCHED A MAGIC WORD EXCEPTION");
        return false;
      }
    }

    return true;
}

std::string Challenger::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  ChallengerExtendedResponse* extended_response = (ChallengerExtendedResponse*) response_info.response_data.get();

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
 * Should be called upon failure of providing solution. Checks the challenger_ip_db
 * for current number of failure of solution for an ip, increament and store it
 * report to swabber in case of excessive failure
 *
 * @return true if no_of_failures exceeded the threshold
 */
bool
Challenger::report_failure(const std::shared_ptr<HostChallengeSpec>& failed_challenge, const TransactionParts& transaction_parts)
{
  std::string client_ip = transaction_parts.at(TransactionMuncher::IP);
  std::string failed_host = transaction_parts.at(TransactionMuncher::HOST);

  if (kafka_producer != nullptr) {
    print::debug("calling kafka_producer->report_failure()");
    kafka_producer->report_failure(failed_host, client_ip);
  } else {
    print::debug("kafka_producer is null!!!!!!!!!!");
  }


  boost::optional<IpDb::IpState> ip_state = challenger_ip_db->get_ip_state(client_ip);
  if (!ip_state) //we failed to read so we can't judge
    return false;

  (**ip_state)++;

  if (*ip_state >= failed_challenge->fail_tolerance_threshold) {
    TransactionParts ats_record_parts = transaction_parts;

    string banning_reason = "failed challenge " + failed_challenge->name + " for host " + failed_host  + " " + to_string(*ip_state) + " times, " +
      encapsulate_in_quotes(ats_record_parts[TransactionMuncher::URL]) + ", " +
      ats_record_parts[TransactionMuncher::HOST] + ", " +
      encapsulate_in_quotes(ats_record_parts[TransactionMuncher::UA]);

    swabber->ban(client_ip.c_str(), banning_reason);
    //reset the number of failures for future
    //we are not clearing the state cause it is not for sure that
    //swabber ban the ip due to possible failure of acquiring lock
    //ip_state.detail.no_of_failures = 0;
    return true;
  }
  else { //only report if we haven't report to swabber cause otherwise it nulifies the work of swabber which has forgiven the ip and delete it from db
    challenger_ip_db->set_ip_state(client_ip, *ip_state);
    return false;
  }
}

/**
 * Should be called upon successful solution of a challenge to wipe up the
 * the ip's failure record
 *
 * @param client_ip: string representing the failed requester ip
 */
void
Challenger::report_success(std::string client_ip)
{
  IpDb::IpState ip_state(1);
  challenger_ip_db->set_ip_state(client_ip, ip_state);
}


std::shared_ptr<HostChallengeSpec>
Challenger::parse_single_challenge(const YAML::Node& ch) {
  auto host_challenge_spec = make_shared<HostChallengeSpec>();

  host_challenge_spec->name = ch["name"].as<std::string>();
  DEBUG("Loading conf for challenge ", host_challenge_spec->name);

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
    using MagicWord = HostChallengeSpec::MagicWord;

    auto& mw = ch["magic_word"];
    auto& storage = host_challenge_spec->magic_words;

    if (mw.IsSequence()) {
      TSDebug(BANJAX_PLUGIN_NAME, "--- MW is sequence");
      for (auto& entry : mw) {
        TSDebug(BANJAX_PLUGIN_NAME, "--- top of for loop");
        if (entry.IsSequence()) {
          TSDebug(BANJAX_PLUGIN_NAME, "--- entry is sequence");
          auto type = entry[0].as<string>();
          auto word = entry[1].as<string>();
          TSDebug(BANJAX_PLUGIN_NAME, "--- type: %s", type.c_str());
          TSDebug(BANJAX_PLUGIN_NAME, "--- word: %s", word.c_str());

          if (type == "regexp") {
            storage.insert(MagicWord::make_regexp(word));
          }
          else if (type == "substr") {
            storage.insert(MagicWord::make_substr(word));
          }
          else {
            // TODO: In newest versions of YAML we can get
            // the mark from the node.
            throw YAML::Exception(YAML::Mark::null_mark(), "Invalid type for magic word");
          }
        }
        else {
          storage.insert(MagicWord::make_substr(entry.as<string>()));
        }
      }
    }
    else {
      storage.insert(MagicWord::make_substr(mw.as<string>()));
    }
  }

  if (ch["magic_word_exceptions"]) {
    auto exprs = ch["magic_word_exceptions"].as<vector<string>>();

    for (const auto& expr : exprs) {
      host_challenge_spec->magic_word_exceptions.emplace_back(expr);
    }
  }

  if (ch["white_listed_ips"]) {
    auto& white_list = host_challenge_spec->white_listed_ips;

    for (auto ipr : ch["white_listed_ips"].as<vector<string>>()) {
      TSDebug(BANJAX_PLUGIN_NAME, "@$# white list this ip: %s", ipr.c_str());
      white_list.insert(make_mask_for_range(ipr));
    }
  }
  return host_challenge_spec;
}
