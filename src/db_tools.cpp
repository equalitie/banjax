/*
 * The functions to access the ats database, all access to the database
 * should be interfaced through this class
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 * 
 * Vmon: June 2013
 */

#include <string>
#include <vector>
#include <my_global.h>
#include <mysql.h>

#include <ts/ts.h>

using namespace std;

#include "db_tools.h"


const string DBTools::SERVER =  "localhost";
const string DBTools::USER = "root";
const string DBTools::PASSWORD =  "Hak9Aith";
const string DBTools::DATABASE = "ats";

//Table names
const string DBTools::REGEX_TABLE_NAME = "ats_regex";
const unsigned int DBTools::REGEX_FIELD_INDEX = 2;

/* Connects to the database */ 
DBTools::DBTools()
{
  mysql_connect = mysql_init(NULL);

  if(!mysql_connect)    /* If instance didn't initialize say so and exit with fault.*/
    {
      TSDebug("banjax", "Failed to initiate connection to mysql");
      throw DBTools::CONNECT_ERROR;
    }
    /* Now we will actually connect to the specific database.*/
 
  mysql_connect=mysql_real_connect(mysql_connect, DBTools::SERVER.c_str(), DBTools::USER.c_str(),DBTools::PASSWORD.c_str(),DBTools::DATABASE.c_str(),0,NULL,0);
    /* Following if statements are unneeded too, but it's worth it to show on your
    first app, so that if your database is empty or the query didn't return anything it
    will at least let you know that the connection to the mysql server was established. */
 
    if(mysql_connect){
      TSDebug("banjax","Connection Succeeded\n");
    }
    else{
      TSDebug("banjax","Connection to the database failed!\n");
      throw DBTools::CONNECT_ERROR;
    }
}

/**
   opens a table and associate a row objects to its rows
    @param table_name: the name of the table
    @return If succeeds returns a pointer TableData object containing the
           length and pointer to rows. If fails return NULL and throw and exception
 */
TableData* 
DBTools::read_table(string table_name)
{
  TableData* result_data = new TableData;

  mysql_query(mysql_connect,(string("SELECT * FROM ") + table_name).c_str());

  /* Send a query to the database. */
   result_data->cursor = mysql_store_result(mysql_connect); /* Receive the result and store it in res_set */
  if (!result_data->cursor) {
    TSDebug("banjax", (string("Failed to query Table ")+table_name).c_str());
    throw DBTools::TABLE_OPEN_ERROR;
  }
 
   result_data->no_of_rows = mysql_num_rows(result_data->cursor); /* Create the count to print all rows */

   return result_data;
}

/**
   ad the list of Regex and return them as an array of string
    @return array of string whose elements are banning regex
 */
vector<string>* DBTools::read_ban_regex()
{

  TableData* regex_table_handle = read_table(DBTools::REGEX_TABLE_NAME.c_str());
  vector<string>* Regexes = new vector<string>(regex_table_handle->no_of_rows);

  for(unsigned int i = 0; i < regex_table_handle->no_of_rows; i++)  {
    MYSQL_ROW cur_row = mysql_fetch_row(regex_table_handle->cursor);
    (*Regexes)[i] = cur_row[DBTools::REGEX_FIELD_INDEX];
  }
       
  return Regexes;

}
