/*
 * WhiteLister unit test set
 * 
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: Sept 2013, Initial version
 */

// Copyright 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <zmq.hpp>

#include <re2/re2.h> //google re2

#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtest/gtest.h> //google test

#include <ts/ts.h>

#include "util.h"
#include "banjax.h"
#include "unittest_common.h"
#include "white_lister.h"

using namespace std;

/**
   Mainly fill an string stream buffer with a predefined configuration
   and to check if the white lister has picked them correctly and
   match them correctly.
 */
class WhiteListerTest : public testing::Test {
 protected:

  YAML::Node cfg;
  string TEMP_DIR;
  string TEST_CONF_FILE;

  fstream  mock_config;
  BanjaxFilter* test_white_lister;

  virtual void SetUp() {

    test_white_lister = NULL;
    TEMP_DIR = "/tmp";

    //gtest is multip thread so we can't use the same file
    //for them
    char random_suffix[7]; 
    sprintf(random_suffix,"%i", rand()%100000);
    TEST_CONF_FILE = TEMP_DIR + "/test"+random_suffix+".conf";
    try {
      mock_config.open(TEST_CONF_FILE,ios::out);
    } catch (std::ifstream::failure e) {
      ASSERT_TRUE(false);
    }
    
    mock_config << "white_listed_ips: " << endl;
    mock_config << " - 127.0.0.1" << endl;
    mock_config << " - x.y.z.w" << endl;
    mock_config << "" << endl;
    mock_config << "botbanger_port: 1234" << endl;
    mock_config << "" << endl;
    mock_config << "challenger:" << endl;
    mock_config << "  key: testtest" << endl;
    mock_config << "  difficulty: 8" << endl;
    mock_config << "" << endl;
    mock_config << "include:" << endl;
    mock_config << " - banjax.d/general.conf" << endl;
    
    mock_config.close();
  }

  //if there is no way to feed an sstream to
  //to config reader then we might need to close the file
  //tear down. More importantly we need to make the file in case of non
  //existence anyways
  virtual void TearDown() {
   string rm_command("rm ");
   rm_command += TEST_CONF_FILE;
   int r = system(rm_command.c_str());
   (void)r;

   delete test_white_lister;
  }

  void open_config()
  {
    YAML::Node cfg;
    try  {
      cfg= YAML::LoadFile(TEST_CONF_FILE.c_str());
    }
    catch(const libconfig::FileIOException &fioex)  {
      ASSERT_TRUE(false);
    }
    catch(const libconfig::ParseException &pex)   {
      ASSERT_TRUE(false);
    }

    test_white_lister = new WhiteLister(TEMP_DIR, cfg["white_listed_ips"]);

  }

};
  
/**
   read a pre determined config file and check if the values are
   as expected for the regex manager
 */
TEST_F(WhiteListerTest, load_config) {
  open_config();
}

/**
   make up a fake transaction and check that the ip get white listed
 */
TEST_F(WhiteListerTest, white_listed_ip)
{

  open_config();

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "127.0.0.1";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::NO_WORRIES_SERVE_IMMIDIATELY);

}

/**
   make up a fake transaction and check that an ip get doesn't get white listed
 */
TEST_F(WhiteListerTest, ordinary_ip)
{

  open_config();

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::GO_AHEAD_NO_COMMENT);

}

/**
   make up a fake transaction and check that the ip get white listed even if it is an 
   invalid one
 */
TEST_F(WhiteListerTest, invalid_white_ip)
{

  open_config();

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "x.y.z.w";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::NO_WORRIES_SERVE_IMMIDIATELY);

}

