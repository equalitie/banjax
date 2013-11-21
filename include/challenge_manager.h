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

#include <openssl/aes.h>

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
  static const unsigned int CHALLENGE_COUNT = 2;

  enum ChallengeType {
    CHALLENGE_SHA_INVERSE,
    CHALLENGE_CAPTCHA
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

  std::string host_name; //this is actually redundant as we are indexing by hostname
  ChallengeDefinition::ChallengeType challenge_type;
  
  std::string challenge_stream;
  unsigned int fail_tolerance_threshold; //if an ip fails to show a solution after no of attemps
  //greater than this threshold, it will be reported to swabber for bannig, threshold
  //of 0 means: infinite failure allowed, don't keep state
  HostChallengeSpec()
    : fail_tolerance_threshold() {}

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

class ChallengeManager : public BanjaxFilter {
protected:	
	// AES key. 
    AES_KEY enc_key, dec_key;
	// Number of zeros needed at the end of the SHA hash 
	unsigned int number_of_trailing_zeros;
	// string to replace the number of trailing zeros in the javascript
    unsigned int cookie_life_time;
    
	static std::string zeros_in_javascript;
	
    std::vector<std::string> split(const std::string &, char);
	bool check_sha(const char* cookiestr, const char* cookie_val_end);
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

  std::string base64_encode(const std::string &data);
  std::string base64_decode(const char* data, const char* data_end);
  bool is_captcha_url(const std::string& url);
  bool is_captcha_answer(const std::string& url);

  std::map<std::string, ChallengeDefinition::ChallengeType> challenge_type;
  ChallengeSpec* challenge_specs[ChallengeDefinition::CHALLENGE_COUNT];
  
  typedef std::map<std::string, HostChallengeSpec*> HostSettingsMap;
  HostSettingsMap host_settings_;

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
   *
   * @return true if no_of_failures exceeded the threshold
   */
  bool report_failure(std::string client_ip, unsigned int host_failure_threshold);

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
   * @return        true if the cookie is valid
   */

  bool check_cookie(std::string cookie_value, std::string client_ip);
  /**
   * Generates the token from the client ip and the cookie's validity
   * @param  ip client ip
   * @param  t  time until which the cookie will be valid
   * @return    the encrypted token
   */
  std::string generate_token(std::string client_ip, long time);
  
  //TODO: This needs to be changed to adopt Otto's approach in placing
  //the variable info in cookie header and make the jscript to read them
  //nonetheless it is more efficient to have the html generated in a
  //referenece sent to the function rather than copying it in the stack
  //upon return
  void generate_html(std::string ip, long time, std::string url, string host_header, const TransactionParts& transaction_parts, FilterExtendedResponse* response_info, string& generated_html);
  
public:
    /**
       construtor which receives the config object, set the filter 
       name and calls the config reader function.

       @param main_root the root of the configuration structure
    */
  ChallengeManager(const string& banjax_dir, const libconfig::Setting& main_root, IPDatabase* global_ip_database, SwabberInterface* global_swabber_interface)
    :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, CHALLENGER_FILTER_ID, CHALLENGER_FILTER_NAME), solver_page(banjax_dir + "/solver.html"),
    too_many_failures_message("<html><header></header><body>You are a failure!</body></html>"),
     too_many_failures_message_length(too_many_failures_message.length()),
     swabber_interface(global_swabber_interface)

  {
    queued_tasks[HTTP_REQUEST] = static_cast<FilterTaskFunction>(&ChallengeManager::execute);

    //Initializing the challenge definitions
    for(unsigned int i = 0; i < ChallengeDefinition::CHALLENGE_COUNT; i++) {
      challenge_specs[i] = new ChallengeSpec(ChallengeDefinition::CHALLENGE_LIST[i], ChallengeDefinition::CHALLENGE_FILE_LIST[i],(ChallengeDefinition::ChallengeType)i);
      challenge_type[ChallengeDefinition::CHALLENGE_LIST[i]] = (ChallengeDefinition::ChallengeType)i;
    }
    
    ip_database = global_ip_database;
    load_config(main_root[BANJAX_FILTER_NAME], banjax_dir);
    
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them

    @param cfg the config node for "challenger"
  */
  virtual void load_config(libconfig::Setting& cfg, const std::string& banjax_dir);

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
  virtual char* generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info);

};

#endif /* challenge_manager.h */
