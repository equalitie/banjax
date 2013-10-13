/**
   Copyright (C) 2013 eQualit.ie under AGPL V3.0 or later
*/

#include <iostream>
#include <fstream>
#include <time.h>

#include <ts/ts.h>

#include "util.h"
#include "banjax.h"
#include "unittest_common.h"
#include "challenge_manager.h"

using namespace std;



int main()
{
  const unsigned long no_tokens_to_encode = 1000000L;
  ChallengeManager test_challenge_manager("/usr/local/libexec/trafficserver/banjax");
  cout << "Testing the speed of the b64 encorders and decoders..." << endl;
  cout << "Ben's version..." << endl;
  cout << "Encoding " << no_tokens_to_encode << " random tokens.." << endl;

  ifstream fd("/dev/urandom",ios::binary);
  uint8_t* cur_token = new uint8_t[no_tokens_to_encode*AES_BLOCK_SIZE];
  fd.read((char*)cur_token, no_tokens_to_encode*AES_BLOCK_SIZE);

  clock_t t1,t2;
  t1=clock();
  for(unsigned long i = 0;  i < no_tokens_to_encode; i++) {

    string original_str((const char*)cur_token+i*AES_BLOCK_SIZE, AES_BLOCK_SIZE);
    string encoded_token = test_challenge_manager.base64_encode(original_str);
	string recovered_str = test_challenge_manager.base64_decode(encoded_token.c_str(), encoded_token.c_str()+encoded_token.length());
    //assert(recovered_str == original_str);
  }    
  
  //code goes here
  t2=clock();
  double diff( (t2-t1) / (double) CLOCKS_PER_SEC);
  cout<<diff<<endl;
  return 0;
    
}
 
