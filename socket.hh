//  $Id: socket.hh 1.12 1997/02/12 08:48:50 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//


#ifndef __SOCKET_HH__
#define __SOCKET_HH__


class TSocket {
public:
    enum TState {init,connecting,connected,closed};

private:
    TState State;
    int  sock;
    unsigned char *Buffer;
    int  BuffSize;
    int  BuffEnd;
    int  BuffNdx;
    const char *ipAdr;
    const char *service;
    const char *protocol;

    int send( const void *src, int len );
    int nextchar( void );

public:
    TSocket( void );
    TSocket( const TSocket &right );     // copy constructor not allowed !
    ~TSocket();
    operator = (const TSocket &right);   // assignment operator not allowed !

    void close( void );
    int open( const char *ipAdr, const char *service, const char *protocol,
	      int port=-1, int buffSize=4096 );
    int printf( const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
    char *gets( char *buff, int bufflen );

    TState state( void ) { return State; }
    const char *getIpAdr( void ) { return ipAdr; }
    const char *getLocalhost( void );
    
    static unsigned long getBytesRcvd( void );
    static unsigned long getBytesXmtd( void );
    static void abortAll( void );
};


#endif
