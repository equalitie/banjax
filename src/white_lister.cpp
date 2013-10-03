/*
 * Functions deal with the ip white listing.
 * 
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: Oct 2013, Initial version
 */
#include <string>
#include <list>

#include <ts/ts.h>

using namespace std;

#include "white_lister.h"

/**
  reads all the regular expressions from the database.
  and compile them
 */
void
WhiteLister::load_config(libconfig::Setting& cfg)
{
   try
   {
     const libconfig::Setting &ip_white_list = cfg["white_listed_ips"];
     unsigned int count = ip_white_list.getLength();

     //now we compile all of them and store them for later use
     for(unsigned int i = 0; i < count; i++)
       white_list.push_back((const char*)(ip_white_list[i]));

   }
   catch(const libconfig::SettingNotFoundException &nfex)
     {
       // Ignore.
     }

}

FilterResponse WhiteLister::execute(const TransactionParts& transaction_parts)
{

  for(list<string>::iterator it= white_list.begin(); it != white_list.end(); it++)
    {
      if (*it == transaction_parts.at(TransactionMuncher::IP))
        {
          TSDebug("banjax", "white listed ip: %s", (*it).c_str());
          return FilterResponse(FilterResponse::NO_WORRIES_SERVE_IMMIDIATELY);
        }
    }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
                    
}

