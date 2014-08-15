/*
 * Collection of isolated functions used in different part of banjax
 *
 * Copyright (c) 2013 eQualit.ie under GNU AGPL v3.0 or later
 * 
 * Vmon: June 2013 Initial version
 *       Oct  2013 zmq stuff moved here for public use.
 */

#include <stdio.h>

#include <ts/ts.h>
#include <zmq.hpp>
#include <string>

#include <netinet/in.h>

#include "util.h"

using namespace std;

/* Check if the ATS version is the right version for this plugin
   that is version 2.0 or higher for now
   */
int
check_ts_version()
{

  const char *ts_version = TSTrafficServerVersionGet();
  int result = 0;

  if (ts_version) {
    int major_ts_version = 0;
    int minor_ts_version = 0;
    int patch_ts_version = 0;

    if (sscanf(ts_version, "%d.%d.%d", &major_ts_version, &minor_ts_version, &patch_ts_version) != 3) {
      return 0;
    }

    /* Need at least TS 2.0 */
    if (major_ts_version >= 2) {
      result = 1;
    }

  }

  return result;
}

/**
 * Sends a message through zmq
 * @param mess the message to be sent
 * @param more true if we have additional messages to send
 */
void send_zmq_mess(zmq::socket_t& zmqsock, const string mess, bool more){
  zmq::message_t m(mess.size());
  memcpy((void*) m.data(), mess.c_str(), mess.size());
  if(more){
    zmqsock.send(m, ZMQ_SNDMORE);
  } else {
    zmqsock.send(m);  
  }
}


/* dealing with ip ranges, these probably should go somewhere so 
   all filters can benefit from them */

/**
   Check an ip against a CDR mask
   
 */
inline bool is_match(const std::string &needle_ip, const SubnetRange& ip_range_pair) {

  in_addr_t _IP = inet_addr(needle_ip.c_str());
  _IP = ntohl(_IP);
  return ( (ip_range_pair.first & ip_range_pair.second) == (_IP & ip_range_pair.second) );
}

/**
   Get an ip range and return a CIDR bitmask
   
   @param hey_ip ip/range
   
   @return pair of (subnet ip, CIDR bitmask)
*/
SubnetRange  make_mask_for_range(const std::string& hay_ip)
{
  in_addr_t _ip = inet_addr(hay_ip.c_str());
  _ip = ntohl(_ip);
  uint32_t mask=((_ip & 0x0000ffff) == 0) ? 0xffff0000 : 
    ((_ip & 0x000000ff) == 0 ? 0xffffff00 : 0xffffffff);

  return SubnetRange(_ip, mask);

}
