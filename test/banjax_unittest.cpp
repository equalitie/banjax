/*
 * Tests banjax initializations methods
 *
 * Copyright (C) 2013 eQualit.ie
 * Distributed under GNU AGPL
 * 
 * Vmon: June 2013, initial version
 */

#include "banjax.h"
#include "gtest.h"

class BanjaxTest : public testing:Test
{

protected:
  Banjax test_banjax("./config");

};

TEST_F(BanjaxTest, Intitialization) {

};
