/*
 * The functions to access the ats database, all access to the database
 * should be interfaced through this class
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 * 
 * Vmon: June 2013
 */

#ifndef DB_TOOLS_H
#define DB_TOOLS_H

/*
  DBTools return this type when it reads a table
  it both contains the table data and number of records
 */
class TableData
{
 public:
  unsigned int no_of_rows;
  MYSQL_RES* cursor;
};
  
class DBTools
{
 protected:
  MYSQL* mysql_connect; // Connect to the database

  static const string SERVER;
  static const string USER;
  static const string PASSWORD;
  static const string DATABASE;

  static const string REGEX_TABLE_NAME;
  static const unsigned int REGEX_FIELD_INDEX;
  
  /*to open a table and select all fields from
    this moves the main cursor to that table*/
  //unsigned int open_table(string table_name); Seems to be unnecessary

 public:
  //Error list
  enum MYSQL_ERROR {
    CONNECT_ERROR,
    TABLE_OPEN_ERROR
  };

  //constructor establishes the connection to the database.
  DBTools();

  /**
     Destructor simply closes the DB connection
  */
  ~DBTools()
    {
      mysql_close(mysql_connect);
    }

  /**
     opens a table and associate a row objects to its rows

     @param table_name: the name of the table

     @return If succeeds returns a pointer TableData object containing the
             length and pointer to rows. If fails return NULL and throw and exception
   */
  TableData* read_table(string table_name);
  //Read current record from the table
  MYSQL_ROW read_from_table(MYSQL_RES* table_handl);

  /**
     Read the list of Regex and return them as an array of string

     @return array of string whose elements are banning regex
   */
  vector<string>* read_ban_regex();
  
  
};

#endif /*db_tools.h*/
