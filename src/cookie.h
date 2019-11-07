#ifndef BANJAX_COOKIE_H
#define BANJAX_COOKIE_H

#include <boost/utility/string_view.hpp>
#include <boost/optional.hpp>

class Cookie {
public:
  boost::string_view name;
  boost::string_view value;

  /**
   * This function parses the starting name/value pair from the cookie string.
   * The syntax is simply: <name token> [ '=' <value token> ] with possible
   * spaces between tokens and '='. However spaces in the value token is also
   * allowed.
   * Defined in RFC 2965.
   */
  static
  boost::optional<Cookie> consume(boost::string_view& cookie_s);

  static
  boost::optional<Cookie> parse(boost::string_view cookie_s) {
    return consume(cookie_s);
  }
};

#endif // BANJAX_COOKIE_H
