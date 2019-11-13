#pragma once

#include <string>
#include <sstream>
#include "util.h"

static const char* BANJAX_PLUGIN_NAME = "banjax";

namespace print {

struct ts_http_status {
  TSHttpStatus status;
  ts_http_status(TSHttpStatus s) : status(s) {}
};

inline
std::ostream& operator<<(std::ostream& os, ts_http_status s) {
  switch(s.status) {
    case TS_HTTP_STATUS_NONE:                            os << "TS_HTTP_STATUS_NONE"; break;

    case TS_HTTP_STATUS_CONTINUE:                        os << "TS_HTTP_STATUS_CONTINUE"; break;
    case TS_HTTP_STATUS_SWITCHING_PROTOCOL:              os << "TS_HTTP_STATUS_SWITCHING_PROTOCOL"; break;

    case TS_HTTP_STATUS_OK:                              os << "TS_HTTP_STATUS_OK"; break;
    case TS_HTTP_STATUS_CREATED:                         os << "TS_HTTP_STATUS_CREATED"; break;
    case TS_HTTP_STATUS_ACCEPTED:                        os << "TS_HTTP_STATUS_ACCEPTED"; break;
    case TS_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION:   os << "TS_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION"; break;
    case TS_HTTP_STATUS_NO_CONTENT:                      os << "TS_HTTP_STATUS_NO_CONTENT"; break;
    case TS_HTTP_STATUS_RESET_CONTENT:                   os << "TS_HTTP_STATUS_RESET_CONTENT"; break;
    case TS_HTTP_STATUS_PARTIAL_CONTENT:                 os << "TS_HTTP_STATUS_PARTIAL_CONTENT"; break;
    case TS_HTTP_STATUS_MULTI_STATUS:                    os << "TS_HTTP_STATUS_MULTI_STATUS"; break;
    case TS_HTTP_STATUS_ALREADY_REPORTED:                os << "TS_HTTP_STATUS_ALREADY_REPORTED"; break;
    case TS_HTTP_STATUS_IM_USED:                         os << "TS_HTTP_STATUS_IM_USED"; break;

    case TS_HTTP_STATUS_MULTIPLE_CHOICES:                os << "TS_HTTP_STATUS_MULTIPLE_CHOICES"; break;
    case TS_HTTP_STATUS_MOVED_PERMANENTLY:               os << "TS_HTTP_STATUS_MOVED_PERMANENTLY"; break;
    case TS_HTTP_STATUS_MOVED_TEMPORARILY:               os << "TS_HTTP_STATUS_MOVED_TEMPORARILY"; break;
    case TS_HTTP_STATUS_SEE_OTHER:                       os << "TS_HTTP_STATUS_SEE_OTHER"; break;
    case TS_HTTP_STATUS_NOT_MODIFIED:                    os << "TS_HTTP_STATUS_NOT_MODIFIED"; break;
    case TS_HTTP_STATUS_USE_PROXY:                       os << "TS_HTTP_STATUS_USE_PROXY"; break;
    case TS_HTTP_STATUS_TEMPORARY_REDIRECT:              os << "TS_HTTP_STATUS_TEMPORARY_REDIRECT"; break;
    case TS_HTTP_STATUS_PERMANENT_REDIRECT:              os << "TS_HTTP_STATUS_PERMANENT_REDIRECT"; break;

    case TS_HTTP_STATUS_BAD_REQUEST:                     os << "TS_HTTP_STATUS_BAD_REQUEST"; break;
    case TS_HTTP_STATUS_UNAUTHORIZED:                    os << "TS_HTTP_STATUS_UNAUTHORIZED"; break;
    case TS_HTTP_STATUS_PAYMENT_REQUIRED:                os << "TS_HTTP_STATUS_PAYMENT_REQUIRED"; break;
    case TS_HTTP_STATUS_FORBIDDEN:                       os << "TS_HTTP_STATUS_FORBIDDEN"; break;
    case TS_HTTP_STATUS_NOT_FOUND:                       os << "TS_HTTP_STATUS_NOT_FOUND"; break;
    case TS_HTTP_STATUS_METHOD_NOT_ALLOWED:              os << "TS_HTTP_STATUS_METHOD_NOT_ALLOWED"; break;
    case TS_HTTP_STATUS_NOT_ACCEPTABLE:                  os << "TS_HTTP_STATUS_NOT_ACCEPTABLE"; break;
    case TS_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED:   os << "TS_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED"; break;
    case TS_HTTP_STATUS_REQUEST_TIMEOUT:                 os << "TS_HTTP_STATUS_REQUEST_TIMEOUT"; break;
    case TS_HTTP_STATUS_CONFLICT:                        os << "TS_HTTP_STATUS_CONFLICT"; break;
    case TS_HTTP_STATUS_GONE:                            os << "TS_HTTP_STATUS_GONE"; break;
    case TS_HTTP_STATUS_LENGTH_REQUIRED:                 os << "TS_HTTP_STATUS_LENGTH_REQUIRED"; break;
    case TS_HTTP_STATUS_PRECONDITION_FAILED:             os << "TS_HTTP_STATUS_PRECONDITION_FAILED"; break;
    case TS_HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE:        os << "TS_HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE"; break;
    case TS_HTTP_STATUS_REQUEST_URI_TOO_LONG:            os << "TS_HTTP_STATUS_REQUEST_URI_TOO_LONG"; break;
    case TS_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:          os << "TS_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE"; break;
    case TS_HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE: os << "TS_HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE"; break;
    case TS_HTTP_STATUS_EXPECTATION_FAILED:              os << "TS_HTTP_STATUS_EXPECTATION_FAILED"; break;
    case TS_HTTP_STATUS_UNPROCESSABLE_ENTITY:            os << "TS_HTTP_STATUS_UNPROCESSABLE_ENTITY"; break;
    case TS_HTTP_STATUS_LOCKED:                          os << "TS_HTTP_STATUS_LOCKED"; break;
    case TS_HTTP_STATUS_FAILED_DEPENDENCY:               os << "TS_HTTP_STATUS_FAILED_DEPENDENCY"; break;
    case TS_HTTP_STATUS_UPGRADE_REQUIRED:                os << "TS_HTTP_STATUS_UPGRADE_REQUIRED"; break;
    case TS_HTTP_STATUS_PRECONDITION_REQUIRED:           os << "TS_HTTP_STATUS_PRECONDITION_REQUIRED"; break;
    case TS_HTTP_STATUS_TOO_MANY_REQUESTS:               os << "TS_HTTP_STATUS_TOO_MANY_REQUESTS"; break;
    case TS_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE: os << "TS_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE"; break;

    case TS_HTTP_STATUS_INTERNAL_SERVER_ERROR:           os << "TS_HTTP_STATUS_INTERNAL_SERVER_ERROR"; break;
    case TS_HTTP_STATUS_NOT_IMPLEMENTED:                 os << "TS_HTTP_STATUS_NOT_IMPLEMENTED"; break;
    case TS_HTTP_STATUS_BAD_GATEWAY:                     os << "TS_HTTP_STATUS_BAD_GATEWAY"; break;
    case TS_HTTP_STATUS_SERVICE_UNAVAILABLE:             os << "TS_HTTP_STATUS_SERVICE_UNAVAILABLE"; break;
    case TS_HTTP_STATUS_GATEWAY_TIMEOUT:                 os << "TS_HTTP_STATUS_GATEWAY_TIMEOUT"; break;
    case TS_HTTP_STATUS_HTTPVER_NOT_SUPPORTED:           os << "TS_HTTP_STATUS_HTTPVER_NOT_SUPPORTED"; break;
    case TS_HTTP_STATUS_VARIANT_ALSO_NEGOTIATES:         os << "TS_HTTP_STATUS_VARIANT_ALSO_NEGOTIATES"; break;
    case TS_HTTP_STATUS_INSUFFICIENT_STORAGE:            os << "TS_HTTP_STATUS_INSUFFICIENT_STORAGE"; break;
    case TS_HTTP_STATUS_LOOP_DETECTED:                   os << "TS_HTTP_STATUS_LOOP_DETECTED"; break;
    case TS_HTTP_STATUS_NOT_EXTENDED:                    os << "TS_HTTP_STATUS_NOT_EXTENDED"; break;
    case TS_HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED: os << "TS_HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED"; break;
    default:                                             os << "TS_HTTP_STATUS(" << s.status << ")";
  }
  return os;
}

struct ts_event {
  TSEvent event;
  ts_event(TSEvent e) : event(e) {}
};

inline
std::ostream& operator<<(std::ostream& os, ts_event e) {
  switch(e.event) {
    case TS_EVENT_HTTP_TXN_START:         os << "TS_EVENT_HTTP_TXN_START"; break;
    case TS_EVENT_HTTP_READ_REQUEST_HDR:  os << "TS_EVENT_HTTP_READ_REQUEST_HDR"; break;
    case TS_EVENT_HTTP_READ_CACHE_HDR:    os << "TS_EVENT_HTTP_READ_CACHE_HDR"; break;
    case TS_EVENT_HTTP_SEND_REQUEST_HDR:  os << "TS_EVENT_HTTP_SEND_REQUEST_HDR"; break;
    case TS_EVENT_HTTP_SEND_RESPONSE_HDR: os << "TS_EVENT_HTTP_SEND_RESPONSE_HDR"; break;
    case TS_EVENT_HTTP_TXN_CLOSE:         os << "TS_EVENT_HTTP_TXN_CLOSE"; break;
    case TS_EVENT_TIMEOUT:                os << "TS_EVENT_TIMEOUT"; break;
    default:                              os << "UNKNOWN_TS_EVENT";
  }
  return os;
}

inline
std::ostream& operator<<(std::ostream& os, const SubnetRange& r) {
  char str[INET_ADDRSTRLEN];
  auto tmp = htonl(r.first);
  inet_ntop(AF_INET, &tmp, str, INET_ADDRSTRLEN);
  os << str << "/" << std::hex << r.second << std::dec;
  return os;
}

///////////////////////////////////////////////////////////////////////////////
inline
std::string str_impl(std::stringstream& ss) {
    return ss.str();
}

template<class Arg, class... Args>
inline
std::string str_impl(std::stringstream& ss, Arg&& arg, Args&&... args) {
    ss << arg;
    return str_impl(ss, std::forward<Args>(args)...);
}

template<class... Args>
inline
std::string str(Args&&... args) {
    std::stringstream ss;
    return str_impl(ss, std::forward<Args>(args)...);
}

///////////////////////////////////////////////////////////////////////////////
template<class... Args>
inline void debug(const Args&...args)
{
  TSDebug(BANJAX_PLUGIN_NAME, "%s", str(args...).c_str());
}

} // print namespace

#if PRINT_DEBUG
# define DEBUG(...) print::debug(__VA_ARGS__)
#else
# define DEBUG(...) do {} while(false)
#endif

