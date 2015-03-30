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
  //needed by auth challeng
  std::string password_hash;
  std::string magic_word;

};

//Structure to keep state and keep track of challenge failure
struct ChallengerState {
  unsigned int no_of_failures;
};

union ChallengerStateUnion
{
  FilterState state_allocator;
  ChallengerState detail;

  ChallengerStateUnion()
    : state_allocator() { }
  
};


class ChallengerExtendedResponse : public FilterExtendedResponse
{
 public:
  HostChallengeSpec* responding_challenge; 
  std::string alternative_url;

 ChallengerExtendedResponse(ResponseGenerator requested_response_generator = NULL, HostChallengeSpec* cookied_challenge = NULL)
   :  FilterExtendedResponse(requested_response_generator),
    responding_challenge(cookied_challenge) 
    {};
  
};
class ChallengeManager : public BanjaxFilter {
protected:	
  // MAC key. 
  unsigned char hashed_key[SHA256_DIGEST_LENGTH];
  // Number of zeros needed at the end of the SHA hash 
  unsigned int number_of_trailing_zeros;
  // string to replace the number of trailing zeros in the javascript
  unsigned int cookie_life_time;
    
  static std::string zeros_in_javascript;
	
  std::vector<std::string> split(const std::string &, char);

  //Challenge specific validity checks
  /**
   * Checks if the SHA256 of the cookie has the correct number of zeros
   * @param  cookie the value of the cookie
   * @return        true if the SHA256 of the cookie verifies the challenge
   */
  bool check_sha(const char* cookiestr);

  /**
   * Checks if the second part of the cookie indeed SHA256 of the
   *  challenge token +  SHA256(password) 
   * @param  cookie the value of the cookie
   * @return  true if the cookie verifies the challenge
   */
  bool check_auth_validity(const char* cookiestr, const std::string password_hash);

  bool replace(std::string &original, std::string &from, std::string &to);

  //Hosts that challenger needs to check
  std::vector<std::string> challenged_hosts;

  std::string solver_page;
  // substrings of the page that needs to be replaced
  static std::string sub_token;	// token
  static std::string sub_time;	// time until which the cookie is valid
  static std::string sub_url;		// url that should be queried
  static std::string sub_zeros;	// number of trailing zeros
    
  // base64 encoding functions
  static const char b64_table[65];
  static const char reverse_table[128];
  
  bool is_captcha_url(const std::string& url);
  bool is_captcha_answer(const std::string& url);

  //for auth challenge
  bool url_contains_magic_word(const std::string& url, const std::string& magic_word);

  std::map<std::string, ChallengeDefinition::ChallengeType> challenge_type;
  ChallengeSpec* challenge_specs[ChallengeDefinition::CHALLENGE_COUNT];
  
  typedef std::map<std::string, HostChallengeSpec*> ChallengeSettingsMap;
  typedef std::map<std::string, std::list<HostChallengeSpec*>> HostChallengeMap;
  ChallengeSettingsMap challenge_settings;
  HostChallengeMap host_challenges;

  //We store the forbidden message at the begining so we can copy it fast 
  //everytime. It is being stored here for being used again
  //ATS barf if you give it the message in const memory
  const std::string too_many_failures_message;
  const size_t too_many_failures_message_length;

  SwabberInterface* swabber_interface;

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
  bool report_failure(std::string client_ip, HostChallengeSpec* failed_challenge, std::string failed_host);

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
   * @param  ip     the client ip
   * @param  cookied_challenge the challenge structure that the cookie is
   *         supposed to address
   * @return        true if the cookie is valid
   */

  bool check_cookie(std::string answer, std::string cookie_value, std::string client_ip, const HostChallengeSpec& cookied_challenge);
  
  //TODO: This needs to be changed to adopt Otto's approach in placing
  //the variable info in cookie header and make the jscript to read them
  //nonetheless it is more efficient to have the html generated in a
  //referenece sent to the function rather than copying it in the stack
  //upon return
  void generate_html(std::string ip, long time, std::string url, const TransactionParts& transaction_parts, ChallengerExtendedResponse* response_info, string& generated_html);

  /**
     gets a time in long format in future and turn it into browser and human
     understandable point in time
   */
  std::string format_validity_time_for_cookie(long validity_time);
public:
    /**
       construtor which receives the config object, set the filter 
       name and calls the config reader function.

       @param main_root the root of the configuration structure
    */
  ChallengeManager(const string& banjax_dir, YAML::Node main_root, IPDatabase* global_ip_database, SwabberInterface* global_swabber_interface)
    :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, CHALLENGER_FILTER_ID, CHALLENGER_FILTER_NAME), solver_page(banjax_dir + "/solver.html"),
    too_many_failures_message("<html><header></header><body>504 Gateway Timeout</body></html>"),
     too_many_failures_message_length(too_many_failures_message.length()),
    swabber_interface(global_swabber_interface),
    challenger_resopnder(static_cast<ResponseGenerator>(&ChallengeManager::generate_response))

  {
    queued_tasks[HTTP_REQUEST] = static_cast<FilterTaskFunction>(&ChallengeManager::execute);

    //Initializing the challenge definitions
    for(unsigned int i = 0; i < ChallengeDefinition::CHALLENGE_COUNT; i++) {
      challenge_specs[i] = new ChallengeSpec(ChallengeDefinition::CHALLENGE_LIST[i], ChallengeDefinition::CHALLENGE_FILE_LIST[i],(ChallengeDefinition::ChallengeType)i);
      challenge_type[ChallengeDefinition::CHALLENGE_LIST[i]] = (ChallengeDefinition::ChallengeType)i;
    }
    
    ip_database = global_ip_database;
    load_config(main_root, banjax_dir);
    
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them

    @param cfg the config node for "challenger"
  */
  virtual void load_config(YAML::Node cfg, const std::string& banjax_dir);

  /**
     Overloaded to tell banjax that we need url, host 
     to rebuild the redirect address and the host also for selective 
     challenging, we need the ip to prevent clients using each other's
     challenges.
   */
  uint64_t requested_info() { return 
      TransactionMuncher::IP            |
      TransactionMuncher::URL           |
      TransactionMuncher::HOST          |
      TransactionMuncher::URL_WITH_HOST |
      TransactionMuncher::COOKIE;}    

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
  FilterResponse execute(const TransactionParts& transaction_parts);

  /**
     This basically calls the function to generate the html
   */
  virtual std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info);
  //and a pointer to it use later
  ResponseGenerator challenger_resopnder;


};

#endif /* challenge_manager.h */
