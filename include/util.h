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

enum ZMQ_ERROR {
    CONNECT_ERROR,
    SEND_ERROR
};

int check_ts_version();

void send_zmq_mess(zmq::socket_t& zmqsock, const std::string mess, bool more = false);

#endif
