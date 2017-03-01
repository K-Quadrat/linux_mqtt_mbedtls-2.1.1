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

/*
################################################################################
#                                                                              #
#       C++ MySQL Wrapper Version 0.81                                         #
#                                                                              #
#       included Files: mysql.h                                                #
#                       mysql.cpp                                              #
#                                                                              #
################################################################################
*/

/*!     \file mysql.h
 *      \brief C++ MySQL Wrapper 
 *      \author Mike Kosek <mike.kosek@fh-koeln.de>
 *      \author Moritz Gemmeke <moritz.gemmeke@fh-koeln.de>
 *      \date Last update: 2015.04.01
 *      \note Copyright (&copy;) 2014 - 2015 Cologne University of Applied Sciences
 */


#include <string>
#include <map>
#include <vector>
#include <mysql/mysql.h>

#ifndef MYSQL_H
#define	MYSQL_H

using namespace std;

class CMysql {
public:
        
    // Standard Destructor
    ~CMysql();
    
    // Get Singleton Instance
    static CMysql* getInstance();
    
    // Set Database Parameters
    int setDBParameters(string dbhost, string dbname, string dbuser, string dbpassword);
    
    // Set Identifiers
    int setIDs(map<string, string> ids);
    
    // Get Identifiers
    map<string,string> getIDs();

    // Single Insert
    int insert(string table, map<string, string> keysValues);
    
    // Vector Insert
    int insertVector(string table, vector<map<string, string>> keysValuesVector);

    // Select stating Query as String
    std::vector<map<string, string>> select(string queryString);
    
    // Select stating TABLE and WHERE clause
    std::vector<map<string, string>> select(string table, map<string, string> where, string orderBy, int limit);
    
    // Update
    int update(string table, map<string, string> keysValues);
    
    // Generic Query
    int query(string queryString);
    
    // Create Database
    int createDatabase(string name);

private:
    
    // Standard Constructor
    CMysql();
    
    static CMysql* global_pHandlerMysql;
    static pthread_mutex_t global_mutexCreateMysql;
    
    // Database Connection Information
    string dbhost;
    string dbname;
    string dbuser;
    string dbpassword;

    // Identifier Map
    map<string, string> ids;
    
    // Close Database Connection
    void closeConnection(MYSQL* connection);
    
    // Insert Execute
    int insertExecution(string table, map<string, string> keysValues, MYSQL* connection);
    
    // Get Timestamp Offset in Seconds
    int getTimestampOffset();
};

#endif