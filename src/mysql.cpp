/*
################################################################################
#                                                                              #
#       * * * * * *     Fachhochschule Köln                                    #
#         * * * * *     Cologne University of Applied Sciences                 #
#         * * * * *                                                            #
#         * * * * *     Institute of Communication Systems                     #
#         * * *                                                                #
#                                                                              #
#       Research Project QoE PDN                                               #
#               Website: http://www.qoepdn.org                                 #
#                                                                              #
#       Computer Networks Research Group                                       #
#               Website: http://www.dn.fh-koeln.de                             #
#                                                                              #
################################################################################
*/

/*!     \file mysql.cpp
 *      \brief C++ MySQL Wrapper 
 *      \author Mike Kosek <mike.kosek@fh-koeln.de>
 *      \author Moritz Gemmeke <moritz.gemmeke@fh-koeln.de>
 *      \date Last update: 2015.04.01
 *      \note Copyright (&copy;) 2014 - 2015 Cologne University of Applied Sciences
 */
  

#include "mysql.h"
#include <iostream>
#include <sstream>
#include <unistd.h>

CMysql* CMysql::global_pHandlerMysql = NULL;
pthread_mutex_t CMysql::global_mutexCreateMysql = PTHREAD_MUTEX_INITIALIZER;


//! \brief
//!	Standard Constructor
CMysql::CMysql() {
    
    //set default values
    this->dbhost = "";
    this->dbname = "";
    this->dbuser = "";
    this->dbpassword = "";
    
    this->ids.clear();
}


//! \brief
//!	Virtual Destructor
CMysql::~CMysql() {
    
}


//! \brief
//!	Get Singleton Instance
//! \return Database Handler
CMysql* CMysql::getInstance() {
    
    //pthread_mutexattr_t attr;
    //pthread_mutexattr_init(&attr);
    //pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    
    //pthread_mutex_init(&global_mutexCreateMysql, &attr);
    
    pthread_mutex_lock(&global_mutexCreateMysql);
    {
        if (global_pHandlerMysql == NULL) {
                global_pHandlerMysql = new CMysql();
        }
    }
    pthread_mutex_unlock(&global_mutexCreateMysql);

    return global_pHandlerMysql;
}


//! \brief
//!	Set Database Parameters
//! \return 0
int CMysql::setDBParameters(string dbhost, string dbname, string dbuser, string dbpassword) {
    
    //set Database Parameters
    this->dbhost = dbhost;
    this->dbname = dbname;
    this->dbuser = dbuser;
    this->dbpassword = dbpassword;
    
    return 0;
}


//! \brief
//!	Set Identifiers
//! \return 0 if OK, -1 if Error
int CMysql::setIDs(map<string, string> ids) {
    
    //set IDs for use in INSERT and UPDATE
    if (ids.size() < 5) {
        string errorString = "MYSQL: setIDs: Too few arguments";
        cout << errorString;
        return -1;
    }
    else {
        this->ids = ids;
        return 0;   
    }
}


//! \brief
//!	Get Identifiers
//! \return IDs
map<string,string> CMysql::getIDs() {
    
    //return ids
    return this->ids;
}


//! \brief
//!	Close Database Connection
void CMysql::closeConnection(MYSQL* connection) {
    
    //Close MySQL-Connection
    mysql_close(connection);

    //Unload MySQL-Library
    mysql_library_end();
    
    return;
}


//! \brief
//!	Single Insert
//! \return 0 if OK, -1 if Error
int CMysql::insert(string table, map<string, string> keysValues) {
	
    //mySQL INSERT with TABLE, KEYS, VALUES as arguments
    MYSQL *connection;

    connection = mysql_init(NULL);
    
    if (mysql_real_connect(connection, dbhost.c_str(), dbuser.c_str(), dbpassword.c_str(), dbname.c_str(), 0, 0, 0) == NULL) {
        string errorString = "MYSQL: insert: No Connection to the Database: " + string(mysql_error(connection));
        cout << errorString << endl;
        return -1;
    }
 
    if (insertExecution(table, keysValues, connection) != 0) {
        closeConnection(connection);
        return -1;
    }
    
    closeConnection(connection);
    
    return 0;
}


//! \brief
//!	Insert Vector
//! \return 0 if OK, -1 if Error
int CMysql::insertVector(string table, vector<map<string,string> > keysValuesVector){
    
    //return 0 if all inserts are OK
    //returns position in vector of failed insert if one insert fails
    
    MYSQL *connection;

    connection = mysql_init(NULL);
    
    if (mysql_real_connect(connection, dbhost.c_str(), dbuser.c_str(), dbpassword.c_str(), dbname.c_str(), 0, 0, 0) == NULL) {
        string errorString = "MYSQL: insert: No Connection to the Database: " + string(mysql_error(connection));
        cout << errorString << endl;
        return -1;
    }
    
    for(auto it = keysValuesVector.begin(); it != keysValuesVector.end(); ++it){
        if(insertExecution(table, *it, connection) != 0){
            closeConnection(connection);
            return std::distance(keysValuesVector.begin(), it);      
        }
    }
    
    closeConnection(connection);
    
    return 0;
} 


//! \brief
//!	Select stating Query as String
//! \return Result Vector
std::vector<map<string, string>> CMysql::select(string queryString) {
    
    //mySQL SELECT query with queryString
    MYSQL *connection;
    MYSQL_ROW row;
    MYSQL_RES *result;
    MYSQL_FIELD *field;

    ostringstream query;
    
    query.str("");
    query << queryString;
    
    connection = mysql_init(NULL);

    std::vector<map<string, string>> resultvector;
    map<string, string> resultmap;

    if (mysql_real_connect(connection, dbhost.c_str(), dbuser.c_str(), dbpassword.c_str(), dbname.c_str(), 0, 0, 0) == NULL) {
        string errorString = "MYSQL: select: No Connection to the Database: " + string(mysql_error(connection));
        cout << errorString << endl;
        return resultvector;
    }
    
    if(mysql_query(connection, query.str().c_str())) {
        string errorString = "MYSQL: select: " + string(mysql_error(connection));
        cout << errorString << endl;
        closeConnection(connection);
        return resultvector;
    }

    result = mysql_store_result(connection);
    
    //get Field names
    map<int, string> fieldNames;
    int i = 0;
    while(field = mysql_fetch_field(result)) {
        fieldNames[i] = field->name;
        i++;
    }
    
    while((row = mysql_fetch_row(result))) {
        
        resultmap.clear();
        for(i=0; i<mysql_num_fields(result); i++) {
            if (row[i] != NULL) resultmap[fieldNames[i]] = row[i]; 
            else resultmap[fieldNames[i]] = ""; 
        }

        resultvector.push_back(resultmap);
    }

    //Free results
    mysql_free_result(result);

    closeConnection(connection);

    return resultvector;
}


//! \brief
//!	Select stating TABLE and WHERE clause
//! \return Result Vector
std::vector<map<string, string>> CMysql::select(string table, map<string, string> where, string orderBy="", int limit=0) {
        
    //mySQL SELECT with TABLE, KEYS, VALUES, LIMIT (optional) as Argument
    bool first = true;

    ostringstream query;

    //create SELECT statement
    query.str("");
    query << "SELECT * FROM `" << table << "` WHERE ";

    for (auto it = where.begin(); it != where.end(); ++it) {

        if (!first) query << " AND ";
        first = false;

        query << "`" << it->first << "`='" << it->second << "'";
    }

    if (!orderBy.compare("")) query << " ORDER BY " << orderBy;
    
    if (limit != 0) query << " LIMIT " << limit;

    vector<map<string, string>> resultvector = select(query.str());
    
    return resultvector;
}


//! \brief
//!	Update
//! \return 0 if OK, -1 if Error
int CMysql::update(string table, map<string, string> keysValues) {

    //mySQL UPDATE with TABLE, KEYS, VALUES
    bool first = true;
    
    MYSQL *connection;

    ostringstream query;

    connection = mysql_init(NULL);
    
    if (mysql_real_connect(connection, dbhost.c_str(), dbuser.c_str(), dbpassword.c_str(), dbname.c_str(), 0, 0, 0) == NULL) {
        string errorString = "MYSQL: update: No Connection to the Database: " + string(mysql_error(connection));
        cout << errorString << endl;
        return -1;
    }

    //create UPDATE statement
    query.str("");
    query << "UPDATE `" << table << "` SET ";

    for (auto it = keysValues.begin(); it != keysValues.end(); ++it) {

        if (!first) query << ", ";
        first = false;

        if (it->second.compare("CURRENT_TIMESTAMP") == 0) {
            query << "`" << it->first << "`= CURRENT_TIMESTAMP";
        } else {
            query << "`" << it->first << "`='" << it->second << "'";
        }
    }

    first = true;
    query << " WHERE ";

    for (auto itids = this->ids.begin(); itids != this->ids.end(); ++itids) {

        if (!first) query << " AND ";
        first = false;
        
        query << "`" << itids->first << "`='" << itids->second << "'";
    }
    
    //cout << query.str() << endl;

    if(mysql_query(connection, query.str().c_str())) {
        string errorString = "MYSQL: update: UPDATE: " + string(mysql_error(connection));
        cout << errorString << endl;
        closeConnection(connection);
        return -1;
    }
    
    closeConnection(connection);

    return 0;
}


//! \brief
//!	Generic Query
//! \return 0 if OK, -1 if Error
int CMysql::query(string queryString) {
    
    //generic mySQL query
    //don't use for SELECT
    MYSQL *connection;

    ostringstream queryStream;
    
    queryStream.str("");
    queryStream << queryString;
    
    connection = mysql_init(NULL);

    std::vector<map<string, string>> resultvector;
    map<string, string> resultmap;

    if (mysql_real_connect(connection, dbhost.c_str(), dbuser.c_str(), dbpassword.c_str(), dbname.c_str(), 0, 0, 0) == NULL) {
        string errorString = "MYSQL: query: No Connection to the Database: " + string(mysql_error(connection));
        cout << errorString << endl;
        return -1;
    }
    
    if(mysql_query(connection, queryStream.str().c_str())) {
        string errorString = "MYSQL: query: " + string(mysql_error(connection));
        cout << errorString << endl;
        closeConnection(connection);
        return -1;
    }
    
    closeConnection(connection);
    
    return 0;
}


//! \brief
//!	Insert Execute
//! \return 0 if OK, -1 if Error
int CMysql::insertExecution(string table, map<string, string> keysValues, MYSQL *connection) {
    
    ostringstream keys, values, query;

    map<string,string>::iterator itkeysValues;
    map<string,string>::iterator itids;
    
    //create and execute INSERT statement
    keys.str("");
    keys << "INSERT INTO `" << table << "` (";

    values.str("");
    values << ") VALUES (";
    
    //INSERT timestamp_offset
    keys << "`timestamp_offset`";
    values << "'" << getTimestampOffset() << "'";
    
    //iterate over ids
    for (itids = this->ids.begin(); itids != this->ids.end(); ++itids) {

        keys << ", ";
        values << ", ";

        keys << "`" << itids->first << "`";
        values << "'" << itids->second << "'";
    }
    
    //iterate over keys and values
    for (itkeysValues = keysValues.begin(); itkeysValues != keysValues.end(); ++itkeysValues) {

        keys << ", ";
        values << ", ";

        keys << "`" << itkeysValues->first << "`";
        if (itkeysValues->second.empty()) values << "NULL";
        else values << "'" << itkeysValues->second << "'";
    }

    values << ")";

    query.str("");
    query << keys.str() << values.str();
    
    if(mysql_query(connection, query.str().c_str())) {
        string errorString = "MYSQL: insert: " + string(mysql_error(connection));
        cout << errorString << endl;
        return -1;
    }
    
    return 0;
}


//! \brief
//!	Get Timestamp Offset in Seconds
//! \return Timestamp Offset 
int CMysql::getTimestampOffset() {
    struct tm * lt;
    time_t t = time(NULL);
    lt = localtime(&t);

    long int offset = lt->tm_gmtoff;

    return offset;
}


//! \brief
//!	Create Database
//! \return 0 if OK, -1 if Error
int CMysql::createDatabase(string name) {
    
    //creates a mysql database
    MYSQL *connection;

    connection = mysql_init(NULL);
    
    if (mysql_real_connect(connection, dbhost.c_str(), dbuser.c_str(), dbpassword.c_str(), 0, 0, 0, 0) == NULL) {
        string errorString = "MYSQL: createDatabase: No Connection to the Host: " + string(mysql_error(connection));
        cout << errorString << endl;
        return -1;
    }
    
    string queryString = "CREATE DATABASE IF NOT EXISTS `" + name + "` /*!40100 DEFAULT CHARACTER SET utf8 */";
    
    if(mysql_query(connection, queryString.c_str())) {
        string errorString = "MYSQL: createDatabase: " + string(mysql_error(connection));
        cout << errorString << endl;
        closeConnection(connection);
        return -1;
    }
    
    closeConnection(connection);

    return 0;
}