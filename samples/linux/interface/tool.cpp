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

/*!     \file tool.cpp
 *      \brief C++ Tool Class 
 *      \author Mike Kosek <mike.kosek@fh-koeln.de>
 *      \date Last update: 2015.04.01
 *      \note Copyright (&copy;) 2014 - 2015 Cologne University of Applied Sciences
 */

#include "tool.h"


//! \brief
//!	Standard Constructor
CTool::CTool() {
    
}


//! \brief
//!	Virtual Destructor
CTool::~CTool() {
    
}


//! \brief
//!	Get Timestamp in uSeconds
//! \return uSeconds since 1970
unsigned long long CTool::getTimestampUSec() {
    
    struct timeval tp;
    gettimeofday(&tp, NULL);

    return (((unsigned long long) (tp.tv_sec) * 1000000) + (unsigned long long) (tp.tv_usec));
}


//! \brief
//!	Get Timestamps in seconds
//! \return Timestamp in Seconds
long CTool::getTimestampSec() {
    
	struct timeval tp;
	gettimeofday(&tp,NULL);
	
	return (long) tp.tv_sec;
}


//! \brief
//!	Get Timestamp as Formatted String:
//!     YYYY-MM-DD hh:mm:ss:msec
//! \return Formatted Timestamp
std::string CTool::getDateTime() {
    
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[40];
    struct timeval tv;

    gettimeofday(&tv, NULL);
    rawtime = tv.tv_sec;
    int ms = tv.tv_usec / 1000;
    strftime(buffer, 40, "%F %T.", localtime(&rawtime));

    std::string dt = buffer;
    dt.append(to_string(ms));

    return dt;
} 


//! \brief
//!	Get Timestamp as Formatted String for Filesystem safe usage:
//!     YYYY-MM-DD_hh-mm-ss-msec
//! \return Formatted Timestamp
std::string CTool::getDateTimeFilesystem() {
    
    std::string dt = CTool::getDateTime();
    stringReplace(dt, " ", "_");
    stringReplace(dt, ":", "-");

    return dt;
}


//! \brief
//!	Replace Occurrences of oldStr with newStr
void CTool::stringReplace(std::string& str, const std::string& oldStr, const std::string& newStr) {
    
    size_t pos = 0;
    while((pos = str.find(oldStr, pos)) != std::string::npos){
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}


//! \brief
//!	Get PreProcessor Build Time (compile Time)
//! \return Build Time
std::string CTool::getBuildTime() {

    stringstream preTime;
    preTime << __DATE__[7] << __DATE__[8] << __DATE__[9] << __DATE__[10] << "." << __DATE__[0] << __DATE__[1] << __DATE__[2] << __DATE__[3]  << "." << __DATE__[4] << __DATE__[5] << __DATE__[6]  << "-" << __TIME__ << "\n";
      
    string preTimeString = preTime.str();
    
    preTimeString.erase(std::remove_if(preTimeString.begin(), preTimeString.end(), ::isspace), preTimeString.end());
    
    return preTimeString;
}


//! \brief
//!	Get Mac Address of given Interface
//! \return Mac Address
std::string CTool::getMacAddress(std::string ifname) {
    
    std::string systemcall = "ifconfig " + ifname + " | grep -s -o -P 'HWaddr.{0,18}' | cut -d ' ' -f 2";
    FILE* call = popen(systemcall.c_str(), "r");

    char buffer[128];
    std::string mac;
    while (!feof(call)) {
        if (fgets(buffer, 128, call) == NULL) break;
        mac = buffer;
    }
    pclose(call);
    
    if (mac.empty()) mac = "00:00:00:00:00:00";
    mac.erase(std::remove(mac.begin(), mac.end(), '\n'), mac.end());
    
    return mac;
}


//! \brief
//!	Get IPv4 Address of given Interface
//! \return IPv4 Address
std::string CTool::getIPv4Address(std::string ifname) {
    
    std::string systemcall = "ifconfig " + ifname + " | grep -s -o -P 'inet addr:.{0,15}' | cut -d ' ' -f 2 | cut -d ':' -f 2";
    FILE* call = popen(systemcall.c_str(), "r");

    char buffer[128];
    std::string ipv4address;
    while (!feof(call)) {
        if (fgets(buffer, 128, call) == NULL) break;
        ipv4address = buffer;
    }
    pclose(call);
    
    if (ipv4address.empty()) ipv4address = "0.0.0.0";
    ipv4address.erase(std::remove(ipv4address.begin(), ipv4address.end(), '\n'), ipv4address.end());
  
    return ipv4address;
}


//! \brief
//!	Get IPv4 Netmask of given Interface
//! \return IPv4 Netmask
std::string CTool::getIPv4Netmask(std::string ifname) {
    
    std::string systemcall = "ifconfig " + ifname + " | grep -s -o -P 'Mask:.{0,15}' | cut -d ':' -f 2";
    FILE* call = popen(systemcall.c_str(), "r");

    char buffer[128];
    std::string ipv4netmask;
    while (!feof(call)) {
        if (fgets(buffer, 128, call) == NULL) break;
        ipv4netmask = buffer;
    }
    pclose(call);
    
    if (ipv4netmask.empty()) ipv4netmask = "0.0.0.0";
    ipv4netmask.erase(std::remove(ipv4netmask.begin(), ipv4netmask.end(), '\n'), ipv4netmask.end());
    
    return ipv4netmask;
}


//! \brief
//!	Get IPv4 Default Gateway of given Interface
//! \return IPv4 Default Gateway
std::string CTool::getIPv4Gateway(std::string ifname) {
    
    std::string systemcall = "ip route show | grep 'default' | grep '" + ifname + "' | grep -s -o -P 'via.{0,15}' | cut -d ' ' -f 2";
    FILE* call = popen(systemcall.c_str(), "r");

    char buffer[128];
    std::string ipv4gateway;
    while (!feof(call)) {
        if (fgets(buffer, 128, call) == NULL) break;
        ipv4gateway = buffer;
    }
    pclose(call);
    
    if (ipv4gateway.empty()) ipv4gateway = "0.0.0.0";
    ipv4gateway.erase(std::remove(ipv4gateway.begin(), ipv4gateway.end(), '\n'), ipv4gateway.end());
    
    return ipv4gateway;
}


//! \brief
//!	Get DNS Nameserver
//! \return DNS Nameserver
std::string CTool::getDNSNameserver() {
    
    std::string systemcall = "cat /etc/resolv.conf | grep -s -o -P 'nameserver.{0,15}' | cut -d ' ' -f 2";
    FILE* call = popen(systemcall.c_str(), "r");

    char buffer[128];
    std::string nameserver;
    while (!feof(call)) {
        if (fgets(buffer, 128, call) == NULL) break;
        nameserver = buffer;
    }
    pclose(call);
    
    nameserver.erase(std::remove(nameserver.begin(), nameserver.end(), '\n'), nameserver.end());
    
    return nameserver;
}


//! \brief
//!	Get DNS Search
//! \return DNS Search
std::string CTool::getDNSSearch() {
    
    std::string systemcall = "cat /etc/resolv.conf | grep -s -o -P 'search.{0,15}' | cut -d ' ' -f 2";
    FILE* call = popen(systemcall.c_str(), "r");

    char buffer[128];
    std::string search;
    while (!feof(call)) {
        if (fgets(buffer, 128, call) == NULL) break;
        search = buffer;
    }
    pclose(call);
    
    search.erase(std::remove(search.begin(), search.end(), '\n'), search.end());
    
    return search;
}