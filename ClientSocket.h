/******************************************************************************
 * C++ source of aprsNet
 *
 * File:   ClientSocket.h
 * Author: DI Bernhard Isemann
 *         Koglerstrasse 20
 *         3443 Sieghartskirchen
 *         Oesterreich
 *
 * Created on 02. Jänner 2013, 00:10
 * Updated on 10. August 2014, 09:10
 * Version 1.01
 *****************************************************************************/

#ifndef ClientSocket_class
#define ClientSocket_class

#include "Socket.h"


class ClientSocket : private Socket
{
 public:

  ClientSocket ( std::string host, int port );
  virtual ~ClientSocket(){};

  const ClientSocket& operator << ( const std::string& ) const;
  const ClientSocket& operator >> ( std::string& ) const;

};


#endif
