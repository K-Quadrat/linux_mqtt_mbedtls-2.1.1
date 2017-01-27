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
#       C++ Tool Class Version 0.30                                            #
#                                                                              #
#       included Files: tool.h                                                 #
#                       tool.cpp                                               #
#                                                                              #
################################################################################
*/

/*!     \file tool.h
 *      \brief C++ Tool Class 
 *      \author Mike Kosek <mike.kosek@fh-koeln.de>
 *      \date Last update: 2015.04.01
 *      \note Copyright (&copy;) 2014 - 2015 Cologne University of Applied Sciences
 */

#ifndef TOOL_H
#define TOOL_H

#include <string>
#include <sys/time.h>
#include <sstream>
#include <algorithm> 

using namespace std;


//! \class CTool
//! \brief Class with tool functions
class CTool {
    
    public:
        
        // Standard Constructor
        CTool();

        // Standard Destructor
        virtual ~CTool();
        
        // Get Timestamp in uSeconds
        static unsigned long long getTimestampUSec();
        
        // Get Timestamp in Seconds
        static long getTimestampSec();

        // Get Timestamp as Formatted String
        static std::string getDateTime();

        // Get Timestamp as Formatted String for Filesystem safe usage
        static std::string getDateTimeFilesystem();
        
        // Replace Occurrences of oldStr with newStr
        static void stringReplace(std::string& str, const std::string& oldStr, const std::string& newStr);

        // Get PreProcessor Build Time (compile Time)
        static std::string getBuildTime();
        
        // Get Mac Address of given Interface
        static std::string getMacAddress(std::string ifname);
        
        // Get IPv4 Address of given Interface
        static std::string getIPv4Address(std::string ifname);
        
        // Get IPv4 Netmask of given Interface
        static std::string getIPv4Netmask(std::string ifname);
        
        // Get IPv4 Default Gateway of given Interface
        static std::string getIPv4Gateway(std::string ifname);
        
        // Get DNS Nameserver
        static std::string getDNSNameserver();
        
        // Get DNS Search
        static std::string getDNSSearch();

};

#endif


