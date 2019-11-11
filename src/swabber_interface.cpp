/*
 * functions to communicate with swabber to ban the ips detected as botnet
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 *
 * Vmon: June 2013
 */

#include <zmq.hpp>
#include <string>
#include <ctime>
#include <sys/time.h>
#include <utility>

#include <stdio.h>
#include <iostream>
#include <ts/ts.h>

using namespace std;

#include "swabber_interface.h"
#include "banjax.h"

//Swabber connection detail
const string SwabberInterface::SWABBER_SERVER = "*";
const string SwabberInterface::SWABBER_PORT =  "22620";
const long SwabberInterface::SWABBER_GRACE_PERIOD = 0;
const string SwabberInterface::SWABBER_BAN = "swabber_bans";

const unsigned int SwabberInterface::SWABBER_MAX_MSG_SIZE = 1024;

const string SwabberInterface::BAN_IP_LOG("/usr/local/trafficserver/logs/ban_ip_list.log");

/* initiating the interface */
SwabberInterface::SwabberInterface(IPDatabase* global_ip_db)
  :context (1),
   ban_ip_list(BAN_IP_LOG.c_str(), ios::out | ios::app), //openning banned ip log file
   swabber_mutex(TSMutexCreate()),
   ip_database(global_ip_db),
   swabber_server(SWABBER_SERVER),
   swabber_port(SWABBER_PORT),
   grace_period(SWABBER_GRACE_PERIOD)
{
  //TODO: I need to handle error here!!!
  //socket.bind(("tcp://"+SWABBER_SERVER+":"+SWABBER_PORT).c_str());
}

/**
  reads the grace period and swabber listening port and bind to it
 */
void
SwabberInterface::load_config(FilterConfig& swabber_config)
{
  //reset to default
  swabber_server = SWABBER_SERVER;
  swabber_port = SWABBER_PORT;
  grace_period = SWABBER_GRACE_PERIOD;

  TSDebug(BANJAX_PLUGIN_NAME, "Loading swabber interface conf");

  for(auto& cur_node_p : swabber_config.config_node_list) {
    try {
      // Look for white_listed_ips, they all should have been merged by the
      // yaml merger
      auto cur_node = cur_node_p->second;
      if (cur_node["grace_period"])
        grace_period = cur_node["grace_period"].as<long>();

      if (cur_node["port"])
        swabber_port = cur_node["port"].as<string>();

      if (cur_node["server"])
        swabber_server = cur_node["server"].as<string>();

    } catch(YAML::RepresentationException& e) {
      TSDebug(BANJAX_PLUGIN_NAME, "Error loading swabber config: %s", e.what());
      throw;
    }
  }

  string new_binding_string  = "tcp://"+swabber_server+":"+swabber_port;
  if (!p_socket) { //we haven't got connected to anywhere before
    TSDebug(BANJAX_PLUGIN_NAME,"connecting to %s",  new_binding_string.c_str());
    p_socket.reset(new zmq::socket_t(context, ZMQ_PUB));
    p_socket->bind(new_binding_string.c_str());
    //just get connected
  } else if (new_binding_string != _binding_string) { //we are getting connected to a new end point just drop the last point and connect to new point
    TSDebug(BANJAX_PLUGIN_NAME, "unbinding from %s",  _binding_string.c_str());
    try {
      //socket.unbind(_binding_string); //no unbind in old zmq :(
      p_socket.reset(new zmq::socket_t(context, ZMQ_PUB));
      TSDebug(BANJAX_PLUGIN_NAME,"connecting to %s",  new_binding_string.c_str());
      p_socket->bind(new_binding_string.c_str());
    } catch (zmq::error_t e) {
      //this shouldn't happen this is a bug but * doesn't get free even
      //if you delete it
      TSDebug(BANJAX_PLUGIN_NAME, "failed to bind: %s this is probably zmq bug not being able to unbind properly",  e.what());
      throw;
    }
  }; //else  {re-connecting to the same point do nothing} //unbind bind doesn't work

  _binding_string = new_binding_string;
  TSDebug(BANJAX_PLUGIN_NAME, "Done loading swabber conf");
}

/**
   Asks Swabber to ban the bot ip

   @param bot_ip the ip address to be banned
   @param banning_reason the reason for the request to be stored in the log
*/
void
SwabberInterface::ban(string bot_ip, std::string banning_reason)
{
  /*zmq_msg_t msg_to_send, ip_to_send;

  zmq_msg_init_size(&msg_to_send, SWABBER_BAN.size());
  memcpy((zmq_msg_data)(&msg_to_send), (void*)SWABBER_BAN.c_str(), SWABBER_BAN.size());
  if (zmq_send(socket, &msg_to_send, ZMQ_SNDMORE) == -1)
    throw SEND_ERROR;
  else {
    zmq_msg_close(&msg_to_send);

    TSDebug(BANJAX_PLUGIN_NAME, "Publishing %s to be banned", bot_ip.c_str());
    zmq_msg_init_size(&msg_to_send, bot_ip.size());
    memcpy((zmq_msg_data)(&msg_to_send), (void*)bot_ip.c_str(), bot_ip.size());
    if (zmq_send(socket, &msg_to_send, 0) == -1)
      throw SEND_ERROR;
  }

  zmq_msg_close(&msg_to_send);*/

  timeval cur_time; gettimeofday(&cur_time, NULL);
  char time_buffer[80];
  time_t rawtime;

  /* we are waiting for grace period before banning for inteligent gathering purpose */
  if (grace_period > 0) { //if there is no grace then ignore these steps

    std::pair<bool,FilterState> cur_ip_state(ip_database->get_ip_state(bot_ip, SWABBER_INTERFACE_ID));

    /* If we failed to query the database then just don't report to swabber */
    if (cur_ip_state.first == false) {
      /* If it is zero size we set it to the current time */
      TSDebug(BANJAX_PLUGIN_NAME, "not reporting to swabber due to failure of aquiring ip db lock ");
      return;
    }

    if (cur_ip_state.second.size() == 0) {
      //recording the first request for banning
      cur_ip_state.second.resize(1);
      cur_ip_state.second[0] = cur_time.tv_sec;

      ip_database->set_ip_state(bot_ip, SWABBER_INTERFACE_ID, cur_ip_state.second);

      /* Format the time for log */
      time(&rawtime);
      tm* timeinfo = std::gmtime(&rawtime);
      strftime(time_buffer,80,"%Y-%m-%dT%H:%M:%S",timeinfo);

      ban_ip_list << bot_ip << ", " << "[" << time_buffer << "], " << banning_reason << ", flagged" <<endl;
    }

    /* only ban if the grace period is passed */
    if ((cur_time.tv_sec - cur_ip_state.second[0]) < grace_period) {
      TSDebug(BANJAX_PLUGIN_NAME, "not reporting to swabber cause grace period has not passed yet");
      return;
    }
  }

  //grace period pass or no grace period
  /* Format the time for log */
  time(&rawtime);
  tm* timeinfo = std::gmtime(&rawtime);
  strftime(time_buffer,80,"%Y-%m-%dT%H:%M:%S",timeinfo);

  zmq::message_t ban_request(SWABBER_BAN.size());
  memcpy((void*)ban_request.data(), SWABBER_BAN.c_str(), SWABBER_BAN.size());

  zmq::message_t ip_to_ban(bot_ip.size());
  memcpy((void*)ip_to_ban.data(), bot_ip.c_str(), bot_ip.size());

  TSDebug(BANJAX_PLUGIN_NAME, "locking the swabber socket...");
  if (TSMutexLockTry(swabber_mutex) == TS_SUCCESS) {
    if (p_socket) {
      p_socket->send(ban_request, ZMQ_SNDMORE);
      p_socket->send(ip_to_ban);
    }
    else {
      TSDebug(BANJAX_PLUGIN_NAME, "Can't send ban request, socket not instantiated");
    }

    //also asking fail2ban to ban
    //char fail2ban_cmd[1024] = "fail2ban-client set ats-filter banip ";
    //char iptable_ban_cmd[1024] = "iptables -A INPUT -j DROP -s ";
    //strcat(iptable_ban_cmd, bot_ip.c_str());

    //TSDebug(BANJAX_PLUGIN_NAME, "banning client ip: %s", iptable_ban_cmd);
    //system(iptable_ban_cmd);
    ban_ip_list << bot_ip << ", " << "[" << time_buffer << "], " << banning_reason << ", banned" << endl;
    TSMutexUnlock(swabber_mutex);
    //now we can drop the ip from the database
    ip_database->drop_ip(bot_ip);
  }
  else {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the swabber socket");
  }
}
