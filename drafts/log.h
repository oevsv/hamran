/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   log.h
 * Author: Bernhard Isemann
 *
 * Created on 26 Mar 2020, 08:15
 * Updated on 29 Mar 2020, 08:06
 * Version 1.00
 *****************************************************************************/

#include <string>
#include <fstream>
using namespace std;

inline string getCurrentDateTime(string s)
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    if (s == "now")
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    else if (s == "date")
        strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
    return string(buf);
};

inline void Logger(string logMsg)
{
    string filePath = "/var/log/RPX-100_" + getCurrentDateTime("date") + ".log";
    string now = getCurrentDateTime("now");
    ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs << now << '\t' << logMsg << '\n';
    ofs.close();
}

inline void LogInit(void)
{
    string logHeader1 = "********************************************************************************";
    string logHeader2 = "* RPX-100 for Radio Amateur VHF and UHF bands                              *";
    string logHeader3 = "* by OE3BIA, PA3BI                                                             *";
    string filePath = "/var/log/RPX-100_" + getCurrentDateTime("date") + ".log";
    string now = getCurrentDateTime("now");
    ifstream ifile;
    ifile.open(filePath);
    if (ifile)
    {
        ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
        ofs << logHeader1 << '\n';
        ofs << logHeader2 << '\n';
        ofs << logHeader3 << '\n';
        ofs << "* " << now << "                                                             *" << '\n'; 
        ofs << logHeader1 << '\n';
        ofs.close();
    }
    else
    {
        ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::trunc);
        ofs << logHeader1 << '\n';
        ofs << logHeader2 << '\n';
        ofs << logHeader3 << '\n';
        ofs << "* " << now << "                                                             *" << '\n'; 
        ofs << logHeader1 << '\n';
        ofs.close();
    }
}