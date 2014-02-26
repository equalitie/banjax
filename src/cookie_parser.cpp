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
	memset(this, 0, sizeof(CookieParser));
	this->str = cookie_str;

	/* Parse name token */
	while (*cookie_str != ';' && *cookie_str != '=' && !isspace(*cookie_str) && *cookie_str && (!(*cookie_str == '\r' && *(cookie_str+1) == '\n')))
		cookie_str++;

	/* Bail out if name token is empty */
	if (cookie_str == str) return NULL;

	nam_end = cookie_str;

	cookie_str = skip_space(cookie_str);

	switch (*cookie_str) {
	case '\0':
	case ';':
		/* No value token, so just set to empty value */
		val_start = cookie_str;
		val_end = cookie_str;
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
	val_start = cookie_str;
	val_end = cookie_str;

	for (; *cookie_str != ';' && *cookie_str && (!(*cookie_str == '\r' && *(cookie_str+1) == '\n')); cookie_str++) {
		/* Allow spaces in the value but leave out ending spaces */
		if (!isspace(*cookie_str))
			val_end = cookie_str + 1;
	}

	return skip_space(++cookie_str);
}
