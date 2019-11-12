/*
 * functions to communicate with swabber to ban the ips detected as botnet 
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 * 
 * Vmon: June 2013
 */

#ifndef SWABBER_INTERFACE_H
#define SWABBER_INTERFACE_H

#include <zmq.hpp>
#include <fstream>

#include "ip_database.h"
#include "banjax_filter.h"

class SwabberInterface
{
public:
  struct Socket {
    zmq::context_t ctx;
    zmq::socket_t s;
    std::string bound_endpoint;

    Socket()
      : ctx(1 /* thread count */)
      , s(ctx, ZMQ_PUB)
    {}

    bool bind(std::string endpoint) {
      try {
        s.bind(endpoint.c_str());
        bound_endpoint = std::move(endpoint);
      } catch(const zmq::error_t&) {
        return false;
      }
      return true;
    }

    bool is_bound() const {
      return !bound_endpoint.empty();
    }
  };

protected:
  std::string local_endpoint;
  std::unique_ptr<Socket> socket;

  std::string _binding_string; //store the last binded address to unbind on reload
  //"" indicate that we haven't bind anywhere yet

  std::ofstream ban_ip_list;

  //lock for writing into the socket
  TSMutex swabber_mutex;

  //to forgive ips after being banned
  IPDatabase* ip_database;

  //server and the port that swabber is going to connect to
  //if they are not specified in the config, they be set to
  //the default value
  std::string swabber_server;
  std::string swabber_port;

  //the grace period where swabber will wait after it receives the
  //first ban request from the filter. It only bans if it gets another
  //ban request (from any filter) after grace period ends this is
  // to get a log that is representative of the bot behavoir to
  //train ML for bot detection. Default value is zero means
  //ban immediately after receiving the first request
  long grace_period;
  
public:
  //Error list
  enum SWABBER_ERROR {
    CONNECT_ERROR,
    SEND_ERROR
  };

  /**
     initiating the interface
  */
  SwabberInterface(IPDatabase* global_ip_db, std::unique_ptr<Socket> s = nullptr);

  /**
   * access function for grace period used by denialator
   */
  long get_grace_period()
  {
    return grace_period;
  }
  /**
     reads the grace period and swabber listening port and bind to it
     @param swabber_config list of YAML nodes containing swabber configs
  */ 
  void load_config(FilterConfig& swabber_config);
  
  /**
     Asks Swabber to ban the bot ip

     @param bot_ip the ip address to be banned
     @param banning_reason the reason for the request to be stored in the log
  */
  void ban(std::string bot_ip, std::string banning_reason);
  
  /**
   * Release and return the socket that is used to send ban information to
   * swabber agregator. Doing so will disable this Swabber interface and any
   * bans done from that point will not send anything.
   * Releasing the socket is necessary for when we want/need to create another
   * Swabber interface that binds to the same local TCP endpoint. It also
   * enables us to reuse the socket in the new Swabber interface.
   */
  std::unique_ptr<Socket> release_socket();
};

#endif /*db_tools.h*/




