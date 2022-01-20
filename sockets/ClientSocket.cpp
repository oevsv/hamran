/******************************************************************************
 * C++ source of aprsNet
 *
 * File:   ClientSocket.cpp
 * Author: DI Bernhard Isemann
 *         Koglerstrasse 20
 *         3443 Sieghartskirchen
 *         Oesterreich
 *
 * Created on 02. Jänner 2022, 00:10
 * Updated on 07. Jänner 2022, 09:10
 * Version 1.01
 *****************************************************************************/

#include "ClientSocket.h"
#include "SocketException.h"


ClientSocket::ClientSocket ( std::string host, int port )
{
  if ( ! Socket::create() )
    {
      throw SocketException ( "Could not create client socket." );
    }

  if ( ! Socket::connect ( host, port ) )
    {
      throw SocketException ( "Could not bind to port." );
    }

}


const ClientSocket& ClientSocket::operator << ( const std::string& s ) const
{
  if ( ! Socket::send ( s ) )
    {
      throw SocketException ( "Could not write to socket." );
    }

  return *this;

}


const ClientSocket& ClientSocket::operator >> ( std::string& s ) const
{
  if ( ! Socket::recv ( s ) )
    {
      throw SocketException ( "Could not read from socket." );
    }

  return *this;
}



