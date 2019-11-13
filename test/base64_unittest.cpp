/*
 * WhiteLister unit test set
 * 
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: Dec 2015, Initial version
 */

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

#include <gtest/gtest.h> //google test

#include "util.h"
#include "base64.h"

const unsigned long  BUFFERSIZE = 16777216;

#include "b64/encode.h"
#include "b64/decode.h"
#include "unittest_common.h"

using namespace std;

/**
   Mainly fill an string stream buffer with a predefined configuration
   and to check if the white lister has picked them correctly and
   match them correctly.
 */
class Base64Test : public testing::Test {
 protected:

  const unsigned long C_NO_OF_CASES = 1000;
  const unsigned long C_MAX_BLOB_LENGTH = 10000L;
    
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

};
  
/**
   Makes and encode random strings and expect them to be decoded to the same
   strings
 */
TEST_F(Base64Test, banjax_encode_decode) {

  for(unsigned long j = 0; j < C_NO_OF_CASES; j++) {
    unsigned long random_length = rand()%C_MAX_BLOB_LENGTH;
    stringstream stream_in;
    
    for (unsigned long i = 0; i < random_length; i++) {
      char cur_char = rand()%256;
      stream_in << cur_char;
    }

    string encoded =  Base64::Encode(stream_in.str());
    EXPECT_EQ(Base64::Decode(encoded.data(), encoded.data() + encoded.length()), stream_in.str());
  }
  
}

/**
   Makes sure strings encoded by banjax is standard compliance and can be decoded by a generic b64 decoder
 */
TEST_F(Base64Test, banjax_encode_generic_decode) {

  for(unsigned long j = 0; j < C_NO_OF_CASES; j++) {
    unsigned long random_length = rand()%C_MAX_BLOB_LENGTH;
    stringstream stream_in;
    
    for (unsigned long i = 0; i < random_length; i++) {
      char cur_char = rand()%256;
      stream_in << cur_char;
    }

    //char* code = new char[2*stream_in.str().length()];

    string encoded =  Base64::Encode(stream_in.str());
    //cout << encoded << endl;
    stringstream encoded_stream;
    stringstream decoded_stream;
    
    encoded_stream  << encoded;

    base64::decoder D; //generic decoder
    D.decode(encoded_stream, decoded_stream);
    
    EXPECT_EQ(decoded_stream.str(), stream_in.str());
  }
  
}

/**
   Makes sure strings encoded by banjax is the same as the one a generic b64 encoder
   I'm not sure that should even happen
 */
// TEST_F(Base64Test, banjax_encode_generic_encode) {

//   for(unsigned long j = 0; j < C_NO_OF_CASES; j++) {
//     unsigned long random_length = rand()%C_MAX_BLOB_LENGTH;
//     stringstream stream_in;
    
//     for (unsigned long i = 0; i < random_length; i++)
//       stream_in << static_cast<char>(rand()%256);

//     //char* code = new char[2*stream_in.str().length()];

//     string encoded =  Base64::Encode(stream_in.str());
//     //cout << encoded << endl;
//     stringstream encoded_stream;


//     base64::encoder E; //generic encoder
//     E.encode(stream_in,encoded_stream);

//     EXPECT_EQ(encoded_stream.str(), encoded);
//   }
  
// }

/**
   Makes sure strings encoded by banjax is standard compliance and can be decoded by a generic b64 decoder
 */
TEST_F(Base64Test, generic_encode_banjax_decode) {

  for(unsigned long j = 0; j < C_NO_OF_CASES; j++) {
    unsigned long random_length = rand()%C_MAX_BLOB_LENGTH;
    stringstream stream_in;
    
    for (unsigned long i = 0; i < random_length; i++) {
      char cur_char = rand()%256;
      stream_in << cur_char;
    }

      stream_in << rand()%256;

    stringstream encoded_stream;
    base64::encoder E; //generic encoder
    E.encode(stream_in,encoded_stream);
    //cout << encoded;
    string encoded = encoded_stream.str();
    string decoded = Base64::Decode(encoded.data(), encoded.data() + encoded.length());
    EXPECT_EQ(decoded, stream_in.str());
  }
  
}

