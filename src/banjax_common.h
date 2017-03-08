/*
 * Definitions that don't find any place cause they are being used
 * by many classes they can land here
 *
 * Vmon: Oct 2013, Initial Version
 */
#ifndef BANJAX_COMMON_H
#define BANJAX_COMMON_H

#include <string>
#include <yaml-cpp/yaml.h>
const char BANJAX_PLUGIN_NAME[]="banjax";

/**
 *  stop ats due to banjax config problem
 */
void abort_traffic_server();

#endif
