/*
 * Collection of isolated functions used in different part of banjax
 *
 * Copyright (c) 2013 eQualit.ie under GNU AGPL v3.0 or later
 * 
 * Vmon: June 2013, Initial version
 *       Oct 2013, send_zmq_mess
 */

/* Check if the ATS version is the right version for this plugin
   that is version 2.0 or higher for now
   */
#ifndef UTIL_H
#define UTIL_H

#include <zmq.hpp>
#include <string>
#include <utility>

#include <arpa/inet.h>

enum ZMQ_ERROR {
    CONNECT_ERROR,
    SEND_ERROR
};

int check_ts_version();

void send_zmq_mess(zmq::socket_t& zmqsock, const std::string mess, bool more = false);



/* dealing with ip ranges, 
   all filters can benefit from them */

typedef std::pair<in_addr_t, uint32_t> SubnetRange;

/**
   Get an ip range and return a CIDR bitmask
   
   @param hey_ip ip/range
   
   @return pair of (subnet ip, CIDR bitmask)
*/
SubnetRange make_mask_for_range(const std::string& hey_ip);

/**
   Check an ip against a subnet

   @param needle_ip the ip to be checked against the list
   @param pair of <subnet ip, CIDR mask>

   @return true if in the list
   
 */
bool is_match(const std::string &needle_ip, const SubnetRange& ip_range_pair);

#endif
