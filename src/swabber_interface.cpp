/*
 * functions to communicate with swabber to ban the ips detected as botnet 
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 * 
 * Vmon: June 2013
 */

#include <zmq.hpp>
#include <string>

#include <stdio.h>

#include <ts/ts.h>

using namespace std;

#include "swabber_interface.h"
#include "banjax.h"

//Swabber connection detail
const string SwabberInterface::SWABBER_SERVER = "*";
const string SwabberInterface::SWABBER_PORT =  "22620";
const string SwabberInterface::SWABBER_BAN = "swabber_bans";

const unsigned int SwabberInterface::SWABBER_MAX_MSG_SIZE = 1024;

const string SwabberInterface::BAN_IP_LOG("/usr/local/trafficserver/logs/ban_ip_list.log");

/* initiating the interface */ 
SwabberInterface::SwabberInterface()
  :context (1), socket (context, ZMQ_PUB), 
   ban_ip_list(BAN_IP_LOG.c_str(), ios::out | ios::app), //openning banned ip log file
   swabber_mutex(TSMutexCreate())
{

  TSDebug("banjax", "Connecting to swabber server...");
  string test_conn = "tcp://"+SWABBER_SERVER+":"+SWABBER_PORT;
  socket.bind(("tcp://"+SWABBER_SERVER+":"+SWABBER_PORT).c_str());

}

/**
   Destructor: closes and release the publication channell
 */
SwabberInterface::~SwabberInterface()
{
  socket.close();
}
/**
   Asks Swabber to ban the bot ip

   @param bot_ip the ip address to be banned
*/
void 
SwabberInterface::ban(string bot_ip)
{
  /*zmq_msg_t msg_to_send, ip_to_send;
  
  zmq_msg_init_size(&msg_to_send, SWABBER_BAN.size());
  memcpy((zmq_msg_data)(&msg_to_send), (void*)SWABBER_BAN.c_str(), SWABBER_BAN.size());
  if (zmq_send(socket, &msg_to_send, ZMQ_SNDMORE) == -1) 
    throw SEND_ERROR;
  else {
    zmq_msg_close(&msg_to_send);

    TSDebug("banjax", "Publishing %s to be banned", bot_ip.c_str());
    zmq_msg_init_size(&msg_to_send, bot_ip.size());
    memcpy((zmq_msg_data)(&msg_to_send), (void*)bot_ip.c_str(), bot_ip.size());
    if (zmq_send(socket, &msg_to_send, 0) == -1)
      throw SEND_ERROR;
  }

  zmq_msg_close(&msg_to_send);*/
  TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "locking the swabber socket...");
  if (TSMutexLockTry(swabber_mutex) == TS_SUCCESS) {
    zmq::message_t ban_request(SWABBER_BAN.size());
    memcpy((void*)ban_request.data(), SWABBER_BAN.c_str(), SWABBER_BAN.size());
    socket.send(ban_request, ZMQ_SNDMORE);

    zmq::message_t ip_to_ban(bot_ip.size());
    memcpy((void*)ip_to_ban.data(), bot_ip.c_str(), bot_ip.size());
    socket.send(ip_to_ban);

  //also asking fail2ban to ban
  //char fail2ban_cmd[1024] = "fail2ban-client set ats-filter banip ";
  //char iptable_ban_cmd[1024] = "iptables -A INPUT -j DROP -s ";
  //strcat(iptable_ban_cmd, bot_ip.c_str());

  //TSDebug("banjax", "banning client ip: %s", iptable_ban_cmd);
  //system(iptable_ban_cmd);
    TSMutexUnlock(swabber_mutex);
  }
  else
    TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "Unable to get lock on the swabber socket");

  ban_ip_list << bot_ip << endl;

}
