/* Cookies name-value pairs parser  */
//#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "cookie_parser.h"

/* static inline void */
/* debug_cookie_parser(struct cookie_str *cstr, unsigned char *pos, int ws, int eq) */
/* { */
/* 	int namelen = int_max(cstr->nam_end - cstr->str, 0); */
/* 	int valuelen = int_max(cstr->val_end - cstr->val_start, 0); */

/* 	printf("[%.*s] :: (%.*s) :: %d,%d [%s] %d\n", */
/* 	       namelen, cstr->str, */
/* 	       valuelen, cstr->val_start, */
/* 	       ws, eq, pos, cstr->nam_end - cstr->str); */
/* } */


const char*
CookieParser::parse_a_cookie(const char *cookie_str)
{
  name  = string_view();
  value = string_view();

  const char* name_begin = cookie_str;

  /* Parse name token */
  while (*cookie_str != ';' && *cookie_str != '=' && !isspace(*cookie_str) && *cookie_str && (!(*cookie_str == '\r' && *(cookie_str+1) == '\n')))
    cookie_str++;

  /* Bail out if name token is empty */
  if (cookie_str == name_begin) return NULL;

  name = string_view(name_begin, cookie_str - name_begin);

  cookie_str = skip_space(cookie_str);

  switch (*cookie_str) {
  case '\0':
  case ';':
    /* No value token, so just set to empty value */
    value = string_view(cookie_str, 0);
    return cookie_str;

  case '=':
    /* Map 'a===b' to 'a=b' */
    do cookie_str++; while (*cookie_str == '=');
    break;

  default:
    /* No spaces in the name token is allowed */
    return NULL;
  }

  cookie_str = skip_space(cookie_str);

  /* Parse value token */

  /* Start with empty value, so even 'a=' will work */
  const char* val_start = cookie_str;
  const char* val_end   = cookie_str;

  for (; *cookie_str != ';' && *cookie_str && (!(*cookie_str == '\r' && *(cookie_str+1) == '\n')); cookie_str++) {
    /* Allow spaces in the value but leave out ending spaces */
    if (!isspace(*cookie_str))
      val_end = cookie_str + 1;
  }

  value = string_view(val_start, val_end - val_start);

  return skip_space(++cookie_str);
}
