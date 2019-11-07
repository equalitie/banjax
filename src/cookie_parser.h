#ifndef COOKIES_PARSER_H
#define COOKIES_PARSER_H

#include <boost/utility/string_view.hpp>

class CookieParser {
public:
  boost::string_view name;
  boost::string_view value;

  /**
   * This function parses the starting name/value pair from the cookie string.
   * The syntax is simply: <name token> [ '=' <value token> ] with possible
   * spaces between tokens and '='. However spaces in the value token is also
   * allowed. See bug 174 for a description why.
   * Defined in RFC 2965.
   * Return the rest of the cookie string or NULL if we reached the end.
   * Rise exception in case of disaster
   */
  const char *parse_a_cookie(const char* raw_cookie);

  /**
     TODO(vmon): Make a constructor that calls the parse_cookie function
  */
};

#endif
