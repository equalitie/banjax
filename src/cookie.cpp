/* Cookies name-value pairs parser  */
//#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>

#include "cookie.h"

using boost::string_view;

static string_view trim_space_prefix(string_view s)
{
  while (!s.empty() && isspace(s[0])) s.remove_prefix(1);
  return s;
}

static string_view trim_space_suffix(string_view s)
{
  while (!s.empty() && isspace(s.back())) s.remove_suffix(1);
  return s;
}

static bool is_eol_at(string_view s, size_t pos) {
  if (s.size() < pos + 1) return false;
  s.remove_prefix(pos);
  return s.starts_with("\r\n");
}

/* static */
boost::optional<Cookie>
Cookie::consume(boost::string_view& cookie_s)
{
  Cookie c;

  size_t name_end = 0;

  cookie_s = trim_space_prefix(cookie_s);

  if (cookie_s.empty()) return boost::none;

  /* Find end of the name token */
  while (name_end < cookie_s.size()
      && cookie_s[name_end] != ';'
      && cookie_s[name_end] != '='
      && !isspace(cookie_s[name_end])
      && cookie_s[name_end])
    name_end++;

  /* Bail out if name token is empty */
  if (name_end == 0 && cookie_s.front() != '=') {
    if (cookie_s.front() == ';') {
      cookie_s.remove_prefix(1);
      return c;
    }
    return boost::none;
  }

  c.name = cookie_s.substr(0, name_end);

  if (name_end == cookie_s.size()) {
      cookie_s.remove_prefix(name_end);
      return c;
  }

  cookie_s = trim_space_prefix(cookie_s.substr(name_end));

  if (cookie_s.empty()) return c;

  switch (cookie_s[0]) {
  case '\0':
  case ';':
    cookie_s.remove_prefix(1);
    /* No value token, so just set to empty value */
    return c;

  case '=':
    /* Map 'a===b' to 'a=b' */
    do cookie_s.remove_prefix(1); while (cookie_s[0] == '=');
    break;

  default:
    /* No spaces in the name token is allowed */
    return boost::none;
  }

  cookie_s = trim_space_prefix(cookie_s);

  /* Parse value token */

  /* Start with empty value, so even 'a=' will work */
  size_t val_end = 0;

  while (val_end < cookie_s.size()
      && cookie_s[val_end] != ';'
      && cookie_s[val_end]
      && !is_eol_at(cookie_s, val_end))
  {
    ++val_end;
  }

  c.value = trim_space_suffix(cookie_s.substr(0, val_end));

  cookie_s.remove_prefix(val_end);

  if (cookie_s.starts_with(';')) cookie_s.remove_prefix(1);

  return c;
}
