#ifndef COOKIES_PARSER_H
#define COOKIES_PARSER_H

#include <list>

//#include <experimental/string_view>
struct string_view {
  string_view() : start(nullptr), count(0) {}
  string_view(const string_view& other) : start(other.start), count(other.count) {}
  string_view(const char* s, size_t count) : start(s), count(count) {}
  string_view(const char* s) : start(s), count(strlen(s)) {}

  const char* begin() const { return start; }
  const char* end() const { return start + count; }

  bool operator==(const char* s) const {
    for (size_t i = 0; i < count; ++i) {
      if (s[i] == '\0') return false;
      if (s[i] != start[i]) return false;
    }
    return true;
  }

  const char* start;
  size_t count;
};

class CookieParser {
protected:
  /**
     move the pointer to the first non-space char in string
     TODO it shouldn't be defined here.
  */
  inline const char* skip_space(const char* cur_str)
  {
    while (isspace(*cur_str)) cur_str++;
    return cur_str;
  }

public:
  string_view name;
  string_view value;

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
