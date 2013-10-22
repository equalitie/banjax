/*
 * functions to communicate with swabber to ban the ips detected as botnet 
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 * 
 * Vmon: June 2013
 */

#ifndef SWABBER_INTERFACE_H
#define SWABBER_INTERFACE_H

#include <fstream>

class SwabberInterface
{
 protected:
  static const string SWABBER_SERVER;
  static const string SWABBER_PORT;
  static const string SWABBER_BAN;

  static const string BAN_IP_LOG;

  static const unsigned int SWABBER_MAX_MSG_SIZE;

  //socket stuff
  zmq::context_t context;
  zmq::socket_t socket;

  std::ofstream ban_ip_list;

 public:
  //Error list
  enum SWABBER_ERROR {
    CONNECT_ERROR,
    SEND_ERROR
  };

  /**
     initiating the interface
  */
  SwabberInterface();

  /**
     Asks Swabber to ban the bot ip

     @param bot_ip the ip address to be banned
  */
  void ban(string bot_ip);
  
};

#endif /*db_tools.h*/




