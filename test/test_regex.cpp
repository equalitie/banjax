/*
 * RegexManager unit test set
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

#include <stdio.h>
#include <stdlib.h>

#include <gtest/gtest.h> //google test

#include <ts/ts.h>
#include <unistd.h>

#include "util.h"
#include "banjax.h"
#include "unittest_common.h"
#include "regex_manager.h"

#include "swabber_interface.h"
#include "ip_database.h"

using namespace std;

/**
   Mainly fill an string stream buffer with a predefined configuration
   and to check if the regex manager has picked them correctly and
   match them correctly.
 */
class RegexTester : public testing::Test {
 protected:

  libconfig::Config cfg;
  string TEMP_DIR;
  string TEST_CONF_FILE;

  fstream  mock_config;
  BanjaxFilter* test_regex_manager;
  
  IPDatabase test_ip_database;
  SwabberInterface test_swabber_interface;

  RegexTester()
    :  test_swabber_interface(&test_ip_database)
  {}

  virtual void SetUp() {

    test_regex_manager = NULL;
    TEMP_DIR = "/tmp";

    TEST_CONF_FILE = TEMP_DIR + "/banjax.conf";
  }

  //if there is no way to feed an sstream to
  //to config reader then we might need to close the file
  //tear down. More importantly we need to make the file in case of non
  //existence anyways
  virtual void TearDown() {
   delete test_regex_manager;
  }

  void open_config()
  {
    libconfig::Config cfg;
    try  {
      cfg.readFile(TEST_CONF_FILE.c_str());
    }
    catch(const libconfig::FileIOException &fioex)  {
      ASSERT_TRUE(false);
    }
    catch(const libconfig::ParseException &pex)   {
      ASSERT_TRUE(false);
    }

    const libconfig::Setting& config_root = cfg.getRoot();
    test_regex_manager = new RegexManager(TEMP_DIR, config_root, &test_ip_database, &test_swabber_interface);

  }

};
  

/**
   make up a fake GET request and check that the manager is banning
 */
TEST_F(RegexManagerTest, match)
{

  open_config();

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://simple_to_ban_me/";
  mock_transaction[TransactionMuncher::HOST] = "neverhood.com";
  mock_transaction[TransactionMuncher::UA] = "neverhood browsing and co";

  FilterResponse cur_filter_result = test_regex_manager->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::I_RESPOND);

}

