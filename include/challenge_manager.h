/*
 *  Part of regex_ban plugin: all functions for dealing with the js challenge
 *
 *  Ben, July 2013: Initial version.
 */
#ifndef CHALLENGE_MANAGER_H
#define CHALENGE_MANAGER_H

#include <string>
#include <vector>

class ChallengeManager {
protected:	
	// AES key. 
	static unsigned char key[];
	// Number of zeros needed at the end of the SHA hash 
	static unsigned number_of_trailing_zeros;
	// string to replace the number of trailing zeros in the javascript
	static std::string zeros_in_javascript;
	
static std::vector<std::string> split(const std::string &, char);
	static std::string encrypt_token(unsigned char []);
	static unsigned char* decrypt_token(std::string);
	static bool check_sha(const char* cookiestr, const char* cookie_val_end);
	static bool replace(std::string &original, std::string &from, std::string &to);

    static std::string solver_page;
	// substrings of the page that needs to be replaced
	static std::string sub_token;	// token
	static std::string sub_time;	// time until which the cookie is valid
	static std::string sub_url;		// url that should be queried
	static std::string sub_zeros;	// number of trailing zeros

	// base64 encoding functions
	static const char b64_table[65];
	static const char reverse_table[128];
	static std::string base64_encode(const std::string &data);
	static std::string base64_decode(const char* data, const char* data_end);

public:
  //Error list
  enum MYSQL_ERROR {
    CONNECT_ERROR,
    TABLE_OPEN_ERROR
  };

	/**
	 * Sets the class parameters
	 * @param mKey              AES key
	 * @param nb_trailing_zeros number of zeros in the SHA256
	 */
	static void set_parameters(std::string key, unsigned nb_trailing_zeros);
    /**
     * Checks if the cookie is valid: sha256, ip, and time
     * @param  cookie the value of the cookie
     * @param  ip     the client ip
     * @return        true if the cookie is valid
     */
	static bool check_cookie(std::string cookie_value, std::string client_ip);
	/**
	 * Generates the token from the client ip and the cookie's validity
	 * @param  ip client ip
	 * @param  t  time until which the cookie will be valid
	 * @return    the encrypted token
	 */
	static std::string generate_token(std::string client_ip, long time);
	
	static std::string generate_html(std::string ip, long time, std::string url);

};

#endif /* challenge_manager.h */
