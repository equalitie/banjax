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
  static const std::string SWABBER_SERVER;
  static const std::string SWABBER_PORT;
  static const std::string SWABBER_BAN;

  static const std::string BAN_IP_LOG;

  static const unsigned int SWABBER_MAX_MSG_SIZE;

  //socket stuff
  zmq::context_t context;
  zmq::socket_t socket;

  std::ofstream ban_ip_list;

  //lock for writing into the socket
  TSMutex swabber_mutex;

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
     Destructor: closes and release the publication channell
   */
  ~SwabberInterface();

  /**
     Asks Swabber to ban the bot ip

     @param bot_ip the ip address to be banned
  */
  void ban(std::string bot_ip);
  
};

#endif /*db_tools.h*/




