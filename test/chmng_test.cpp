#include <iostream>
#include <string>
#include <fstream>

#include "challenge_manager.h"

using namespace std;

int main(){

	// string test = "this is a testas";
	// string enc = ChallengeManager::base64_encode(test);
	// string dec = ChallengeManager::base64_decode(enc);

	// cout << "base 64 testing\n";
	// cout << "encoded = " << enc <<endl;
	// cout << "encoded length = " << enc.size() <<endl;
	// cout << "decoded = " << dec <<endl;
	// cout << "decoded length = " << dec.size() <<endl;

	long t = std::time(NULL);
	t += 60*60*24; // 1 day validity

	ChallengeManager::set_parameters("abcdefghijklmnop", 4);

	string ip = "192.168.1.1"; // example of ip

	string tok = ChallengeManager::generate_token(ip, t);
	cout << "token = " << tok << endl;
	cout << "tok length = " << tok.size() << endl;

	string page = ChallengeManager::generate_html(ip, t, "http://google.com");
	std::ofstream myfile;
	myfile.open ("test.html");
	myfile << page;
	myfile.close();

	string cookie(tok);
	cookie.append("test");

	bool check = ChallengeManager::check_cookie(cookie, ip);
	cout << "check_cookie = " << check << endl;	
}
