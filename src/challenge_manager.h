/*
 *  Part of regex_ban plugin: all functions for dealing with the js challenge
 *
 *  Ben, July 2013: Initial version.
 *  Vmon, Setp 2013: Migrate to modular filter structure
 */
#ifndef CHALLENGE_MANAGER_H
#define CHALLENGE_MANAGER_H

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>
#include <openssl/sha.h>

#include "swabber_interface.h"
#include "banjax_filter.h"
#include "global_white_list.h"

const size_t AES_KEY_LENGTH = 16;
//const size_t AES_BLOCK_SIZE;

//Challenges should be declared here
//the name of challenges should appear in same order they
//appears in ChallengType enum
/**
   This would have been a namespace instead of class if it was
   allowed to postpone the declaration of const members of namespace
 */
class ChallengeDefinition
{
public:
  static const unsigned int CHALLENGE_COUNT = 3;

  enum ChallengeType {
    CHALLENGE_SHA_INVERSE,
    CHALLENGE_CAPTCHA,
    CHALLENGE_AUTH
  };

  static const char* CHALLENGE_LIST[];
  static const char* CHALLENGE_FILE_LIST[];
};

class ChallengeSpec
{
public:
  std::string human_readable_name;
  std::string default_page_filename;
  ChallengeDefinition::ChallengeType type;

  ChallengeSpec(std::string challenge_name, std::string challenge_default_file, ChallengeDefinition::ChallengeType challenge_type)
    :human_readable_name(challenge_name),
     default_page_filename(challenge_default_file),
     type(challenge_type) {};
};

/**
   this class keep the information related that defines
   the specification of the challenge for each host
 */
class HostChallengeSpec {
private:
  friend class ChallengeManager;

  struct MagicWord {
    enum Type { substr, regexp };
    Type type;
    std::string magic_word;

    static MagicWord make_substr(const std::string& s) { return MagicWord{Type::substr, s}; }
    static MagicWord make_regexp(const std::string& s) { return MagicWord{Type::regexp, s}; }

    bool is_match(const std::string&) const;

    bool operator<(const MagicWord& other) const {
      return std::tie(type, magic_word)
           < std::tie(other.type, other.magic_word);
    }

    bool operator==(const MagicWord& other) const {
      return std::tie(type, magic_word)
          == std::tie(other.type, other.magic_word);
    }
  };
public:
  //Challenge Types

  std::string name; //this is actually redundant as we are indexing by hostname
  ChallengeDefinition::ChallengeType challenge_type;

  std::string challenge_stream;
  unsigned int fail_tolerance_threshold; //if an ip fails to show a solution after no of attemps
  //greater than this threshold, it will be reported to swabber for bannig, threshold
  //of 0 means: infinite failure allowed, don't keep state

  unsigned long challenge_validity_period; //how many second the challenge is valid for this host
  HostChallengeSpec()
    : fail_tolerance_threshold() {};

  // SHA256 of client's password, needed for the auth challenge.
  std::string password_hash;

  // A set of regular expressions, if at least one of them matches
  // the requested URL, then the client shall be requested authorization
  // before serving the content.
  std::set<MagicWord> magic_words;

  std::vector<std::string> magic_word_exceptions;

  // These IPs will not need to participate in any challenge.
  // The content (even if protected by magic_words) shall be
  // served directly without being cached.
  std::set<SubnetRange> white_listed_ips;
};

class ChallengerExtendedResponse : public FilterExtendedResponse
{
public:
  std::shared_ptr<HostChallengeSpec> responding_challenge;
  std::string alternative_url;

  ChallengerExtendedResponse(ResponseGenerator requested_response_generator, std::shared_ptr<HostChallengeSpec> cookied_challenge) :
    FilterExtendedResponse(requested_response_generator),
    responding_challenge(cookied_challenge)
  {};
};

class ChallengeManager : public BanjaxFilter {
protected:
  // MAC key.
  unsigned char hashed_key[SHA256_DIGEST_LENGTH];
  // Number of zeros needed at the end of the SHA hash
  unsigned int number_of_trailing_zeros;

  std::vector<std::string> split(const std::string &, char);

  //Challenge specific validity checks
  /**
   * Checks if the SHA256 of the cookie has the correct number of zeros
   * @param  cookie the value of the cookie
   * @return        true if the SHA256 of the cookie verifies the challenge
   */
  bool check_sha(const char* cookiestr) const;

  /**
   * Checks if the second part of the cookie indeed SHA256 of the
   *  challenge token +  SHA256(password)
   * @param  cookie the value of the cookie
   * @return  true if the cookie verifies the challenge
   */
  static
  bool check_auth_validity(const char* cookiestr, const std::string password_hash);

  static
  bool replace(std::string &original, const std::string& from, const std::string& to);

  //Hosts that challenger needs to check
  std::vector<std::string> challenged_hosts;

  std::string solver_page;

  // base64 encoding functions
  static const char b64_table[65];
  static const char reverse_table[128];

  static bool is_captcha_url(const std::string& url);
  static bool is_captcha_answer(const std::string& url);

  bool needs_authentication(const std::string& url, const HostChallengeSpec&) const;

  std::map<std::string, ChallengeDefinition::ChallengeType> challenge_type;
  std::vector<std::unique_ptr<ChallengeSpec>> challenge_specs;

  typedef std::map<std::string, std::list<std::shared_ptr<HostChallengeSpec>>> HostChallengeMap;
  HostChallengeMap host_challenges;

  //We store the forbidden message at the begining so we can copy it fast
  //everytime. It is being stored here for being used again
  //ATS barf if you give it the message in const memory
  const std::string too_many_failures_message;

  SwabberInterface* swabber_interface;

  const GlobalWhiteList* global_white_list;

  /**
   * Should be called upon failure of providing solution. Checks the ip_database
   * for current number of failure of solution for an ip, increament and store it
   * report to swabber in case of excessive failure
   *
   * @return true if no_of_failures exceeded the threshold
   */
  bool report_failure(const std::shared_ptr<HostChallengeSpec>& failed_challenge, const TransactionParts& transaction_parts);

  /**
   * Should be called upon successful solution of a challenge to wipe up the
   * the ip's failure record
   *
   * @param client_ip: string representing the failed requester ip
   *
   */
  void report_success(std::string client_ip);

  /**
   * Checks if the cookie is valid: sha256, ip, and time
   * @param  cookie the value of the cookie
   * @param  cookied_challenge the challenge structure that the cookie is
   *         supposed to address
   * @return        true if the cookie is valid
   */
  bool check_cookie(std::string answer, const TransactionParts&, const HostChallengeSpec& cookied_challenge) const;

  std::string generate_html(std::string ip, long time, std::string url, const TransactionParts& transaction_parts, ChallengerExtendedResponse* response_info);

  /**
     gets a time in long format in future and turn it into browser and human
     understandable point in time
   */
  static
  std::string format_validity_time_for_cookie(long validity_time);

  bool is_globally_white_listed(const std::string& ip) const;

public:
  /**
     construtor which receives the config object, set the filter
     name and calls the config reader function.

     @param main_root the root of the configuration structure
  */
 ChallengeManager(const std::string& banjax_dir,
                  const FilterConfig& filter_config,
                  IPDatabase* global_ip_database,
                  SwabberInterface* global_swabber_interface,
                  const GlobalWhiteList* global_white_list)
    :BanjaxFilter::BanjaxFilter(banjax_dir, filter_config, CHALLENGER_FILTER_ID, CHALLENGER_FILTER_NAME), solver_page(banjax_dir + "/solver.html"),
    too_many_failures_message("<html><header></header><body>504 Gateway Timeout</body></html>"),
    swabber_interface(global_swabber_interface),
    global_white_list(global_white_list)
  {
    queued_tasks[HTTP_REQUEST] = this;

    //Initializing the challenge definitions
    for(unsigned int i = 0; i < ChallengeDefinition::CHALLENGE_COUNT; i++) {
      challenge_specs.emplace_back(new ChallengeSpec(ChallengeDefinition::CHALLENGE_LIST[i], ChallengeDefinition::CHALLENGE_FILE_LIST[i],(ChallengeDefinition::ChallengeType)i));
      challenge_type[ChallengeDefinition::CHALLENGE_LIST[i]] = (ChallengeDefinition::ChallengeType)i;
    }

    ip_database = global_ip_database;
    load_config(banjax_dir);
  }

  /**
     Overloaded to tell banjax that we need url, host
     to rebuild the redirect address and the host also for selective
     challenging, we need the ip to prevent clients using each other's
     challenges.

     TransactionMuncher::UA is for reporting only
   */
  uint64_t requested_info() { return
      TransactionMuncher::IP            |
      TransactionMuncher::URL           |
      TransactionMuncher::HOST          |
      TransactionMuncher::UA            |
      TransactionMuncher::URL_WITH_HOST |
      TransactionMuncher::COOKIE;
  }

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the response and handed to the filter.
  */
  //virtual uint64_t response_info() {return TransactionMuncher::RESP_URL;};

  /**
     overloaded execute to execute the filter, It calls cookie checker
     and if it fails ask for responding by the filter.
  */
  FilterResponse on_http_request(const TransactionParts& transaction_parts) override;
  void on_http_close(const TransactionParts& transaction_parts) override {}

private:
  std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info) override;

  void load_config(const std::string& banjax_dir);
};

#endif /* challenge_manager.h */
