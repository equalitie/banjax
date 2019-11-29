/*
 * Banjax Cookie unit test set
 *
 * Copyright (c) eQualit.ie 2019 under GNU AGPL v3.0 or later
 *
 */
#include <boost/test/unit_test.hpp>
#include <iostream>

#include "cookie.h"

using namespace std;
using boost::string_view;
using boost::optional;

BOOST_AUTO_TEST_SUITE(BanjaxCookieTest)

BOOST_AUTO_TEST_CASE(parse) {
  {
    auto oc = Cookie::parse("");
    BOOST_REQUIRE(!oc);
  }

  {
    optional<Cookie> oc = Cookie::parse("a");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "a");
    BOOST_REQUIRE_EQUAL(oc->value, "");
  }

  {
    optional<Cookie> oc = Cookie::parse("a=");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "a");
    BOOST_REQUIRE_EQUAL(oc->value, "");
  }

  {
    optional<Cookie> oc = Cookie::parse("a=b");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "a");
    BOOST_REQUIRE_EQUAL(oc->value, "b");
  }

  {
    optional<Cookie> oc = Cookie::parse(" =b");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "");
    BOOST_REQUIRE_EQUAL(oc->value, "b");
  }

  {
    optional<Cookie> oc = Cookie::parse(" = ");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "");
    BOOST_REQUIRE_EQUAL(oc->value, "");
  }

  {
    optional<Cookie> oc = Cookie::parse("a=b ");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "a");
    BOOST_REQUIRE_EQUAL(oc->value, "b");
  }

  {
    optional<Cookie> oc = Cookie::parse(";");

    BOOST_REQUIRE(oc);
    BOOST_REQUIRE_EQUAL(oc->name, "");
    BOOST_REQUIRE_EQUAL(oc->value, "");
  }

  {
    string_view jar = "k1=v1;k2=v2";

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k1");
      BOOST_REQUIRE_EQUAL(oc->value, "v1");
    }

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k2");
      BOOST_REQUIRE_EQUAL(oc->value, "v2");
    }

    BOOST_REQUIRE(!Cookie::consume(jar));
  }

  {
    string_view jar = " k1=v1; k2=v2 ";

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k1");
      BOOST_REQUIRE_EQUAL(oc->value, "v1");
    }

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k2");
      BOOST_REQUIRE_EQUAL(oc->value, "v2");
    }

    BOOST_REQUIRE(!Cookie::consume(jar));
  }

  {
    string_view jar = " k1=v1; k2=v2 ; ";

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k1");
      BOOST_REQUIRE_EQUAL(oc->value, "v1");
    }

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k2");
      BOOST_REQUIRE_EQUAL(oc->value, "v2");
    }

    BOOST_REQUIRE(!Cookie::consume(jar));
  }

  {
    string_view jar = " k1=v1; k2=v2 ; ;";

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k1");
      BOOST_REQUIRE_EQUAL(oc->value, "v1");
    }

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "k2");
      BOOST_REQUIRE_EQUAL(oc->value, "v2");
    }

    BOOST_REQUIRE(Cookie::consume(jar));
    BOOST_REQUIRE(!Cookie::consume(jar));
  }

  {
    string_view jar = "no-semicolon";

    {
      optional<Cookie> oc = Cookie::consume(jar);
      BOOST_REQUIRE(oc);
      BOOST_REQUIRE_EQUAL(oc->name, "no-semicolon");
    }
    BOOST_REQUIRE(!Cookie::consume(jar));
  }
}

BOOST_AUTO_TEST_SUITE_END()
