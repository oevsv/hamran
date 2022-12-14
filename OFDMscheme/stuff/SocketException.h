/******************************************************************************
 * C++ source of RPX-100S
 *
 * File:   SocketException.h
 * Author: Bernhard Isemann
 *
 * Created on 06 Jan 2022, 12:37
 * Updated on 07 Jan 2022, 17:00
 * Version 1.00
 *****************************************************************************/


#ifndef SocketException_class
#define SocketException_class

#include <string>

class SocketException
{
 public:
  SocketException ( std::string s ) : m_s ( s ) {};
  ~SocketException (){};

  std::string description() { return m_s; }

 private:

  std::string m_s;

};

#endif
