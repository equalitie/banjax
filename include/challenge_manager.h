/*
 *  Part of regex_ban plugin: all functions for dealing with the js challenge
 *
 *  Ben, July 2013: Initial version.
 *  Vmon, Setp 2013: Migrate to modular filter structure
 */
#ifndef CHALLENGE_MANAGER_H
#define CHALENGE_MANAGER_H

#include <string>
#include <vector>

#include <openssl/aes.h>

#include "banjax_filter.h"

const size_t AES_KEY_LENGTH = 16;
//const size_t AES_BLOCK_SIZE;

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

    const std::string solver_page;
	// substrings of the page that needs to be replaced
	static std::string sub_token;	// token
	static std::string sub_time;	// time until which the cookie is valid
	static std::string sub_url;		// url that should be queried
	static std::string sub_zeros;	// number of trailing zeros

	// base64 encoding functions
	static const char b64_table[65];
	static const char reverse_table[128];

public:
	std::string base64_encode(const std::string &data);
	std::string base64_decode(const char* data, const char* data_end);

   /**
       construtor which receives the config object, set the filter 
       name and calls the config reader function.

       @param main_root the root of the configuration structure
   */
 ChallengeManager(const std::string& banjax_dir, const libconfig::Setting& main_root)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, CHALLENGER_FILTER_ID, CHALLENGER_FILTER_NAME), solver_page(banjax_dir + "/solver.html")
  {
    load_config(main_root[BANJAX_FILTER_NAME]);
  }

   /**
       default construtor is used to test various aspect of the Challenger
       that is not related to config like b64 speed
   */
 ChallengeManager(const std::string& banjax_dir)
   :BanjaxFilter::BanjaxFilter(CHALLENGER_FILTER_ID, CHALLENGER_FILTER_NAME), solver_page(banjax_dir + "/solver.html")
  {
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them

    @param cfg the config node for "challenger"
  */
  virtual void load_config(libconfig::Setting& cfg);

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
	
	std::string generate_html(std::string ip, long time, std::string url);

};

#endif /* challenge_manager.h */
