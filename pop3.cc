/* $Id: pop3.cc 1.19 1997/04/20 19:20:34 hardy Exp $
 *
 * This module has been modified for souper.
 */

/* Copyright 1993,1994 by Carl Harris, Jr.
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Carl Harris <ceharris@vt.edu>
 */

//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@swol.de.
//
//  This file is part of VSoup for OS/2.  VSoup including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  NNTP client routines
//

/***********************************************************************
  module:       pop3.c
  program:      popclient
  SCCS ID:      @(#)pop3.c      2.4  3/31/94
  programmer:   Carl Harris, ceharris@vt.edu
  date:         29 December 1993
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  POP2 client code.
 ***********************************************************************/


#include <assert.h> 
#include <ctype.h>
#include <string.h>

#include "areas.hh"
#include "global.hh"
#include "mts.hh"
#include "pop3.hh"
#include "socket.hh"
#include "util.hh"


/* exit code values */

enum PopRetCode {ps_success,      // successful receipt of messages
                 ps_socket,       // socket I/O woes
                 ps_protocol,     // protocol violation
                 ps_error         // some kind of POP3 error condition
};


/*********************************************************************
  function:      POP3_ok
  description:   get the server's response to a command, and return
                 the extra arguments sent with the response.
  arguments:     
    argbuf       buffer to receive the argument string (==NULL -> no return)
    socket       socket to which the server is connected.

  return value:  zero if okay, else return code.
  calls:         SockGets
 *********************************************************************/

static PopRetCode POP3_ok(char *argbuf, TSocket &socket)
{
    PopRetCode ok;
    char buf[BUFSIZ];
    char *bufp;

#ifdef TRACE
    printfT( "POP3_ok()\n" );
#endif
    if (socket.gets(buf, sizeof(buf))) {
#ifdef DEBUG
	printfT( "POP3_ok() -> %s\n",buf );
#endif
	bufp = buf;
	if (*bufp == '+' || *bufp == '-')
	    bufp++;
	else
	    return(ps_protocol);

	while (isalpha(*bufp))
	    bufp++;
	*(bufp++) = '\0';
	
	if (strcmp(buf,"+OK") == 0)
	    ok = ps_success;
	else if (strcmp(buf,"-ERR") == 0)
	    ok = ps_error;
	else
	    ok = ps_protocol;
	
	if (argbuf != NULL)
	    strcpy(argbuf,bufp);
    }
    else {
	ok = ps_socket;
	if (argbuf != NULL)
	    *argbuf = '\0';
    }
    
    return(ok);
}   // POP3_ok



/*********************************************************************
  function:      POP3_Auth
  description:   send the USER and PASS commands to the server, and
                 get the server's response.
  arguments:     
    userid       user's mailserver id.
    password     user's mailserver password.
    socket       socket to which the server is connected.

  return value:  non-zero if success, else zero.
  calls:         SockPrintf, POP3_ok.
 *********************************************************************/

static PopRetCode POP3_Auth(const char *userid, const char *password, TSocket &socket) 
{
    PopRetCode ok;
    char buf[BUFSIZ];

#ifdef TRACE
    printfT( "POP3_Auth(%s,.,.)\n", userid);
#endif
    socket.printf("USER %s\n",userid);
    if ((ok = POP3_ok(buf,socket)) == ps_success) {
	socket.printf("PASS %s\n",password);
	if ((ok = POP3_ok(buf,socket)) == ps_success) 
	    ;  //  okay, we're approved.. 
	else
	    areas.mailPrintf1( 1,"%s\n",buf);
    }
    else
	areas.mailPrintf1(1,"%s\n",buf);
    
    return(ok);
}   // POP3_Auth




/*********************************************************************
  function:      POP3_sendQuit
  description:   send the QUIT command to the server and close 
                 the socket.

  arguments:     
    socket       socket to which the server is connected.

  return value:  none.
  calls:         SockPuts, POP3_ok.
 *********************************************************************/

static PopRetCode POP3_sendQuit(TSocket &socket)
{
    char buf[BUFSIZ];
    PopRetCode ok;

#ifdef TRACE
    printfT( "POP3_sendQuit(): QUIT\n" );
#endif
    socket.printf("QUIT\n");
    ok = POP3_ok(buf,socket);
    if (ok != ps_success)
	areas.mailPrintf1( 1,"%s\n",buf);

    return(ok);
}   // POP3_sendQuit



/*********************************************************************
  function:      POP3_sendStat
  description:   send the STAT command to the POP3 server to find
                 out how many messages are waiting.
  arguments:     
    count        pointer to an integer to receive the message count.
    socket       socket to which the POP3 server is connected.

  return value:  return code from POP3_ok.
  calls:         POP3_ok, SockPrintf
 *********************************************************************/

static PopRetCode POP3_sendStat(int *msgcount, TSocket &socket)
{
    PopRetCode ok;
    char buf[BUFSIZ];
    int totalsize;
    
    socket.printf("STAT\n");
    ok = POP3_ok(buf,socket);
    if (ok == ps_success)
	sscanfT(buf,"%d %d",msgcount,&totalsize);
    else
	areas.mailPrintf1( 1,"%s\n",buf);

#ifdef DEBUG
    printfT( "POP3_sendStat: %d, %d\n", *msgcount,totalsize );
#endif

    return(ok);
}   // POP3_sendStat




/*********************************************************************
  function:      POP3_sendRetr
  description:   send the RETR command to the POP3 server.
  arguments:     
    msgnum       message ID number
    socket       socket to which the POP3 server is connected.

  return value:  return code from POP3_ok.
  calls:         POP3_ok, SockPrintf
 *********************************************************************/

static PopRetCode POP3_sendRetr(int msgnum, TSocket &socket)
{
    PopRetCode ok;
    char buf[BUFSIZ];

#ifdef TRACE
    printfT( "POP3_sendRetr(%d,.)\n",msgnum );
#endif
    socket.printf("RETR %d\n",msgnum);
    ok = POP3_ok(buf,socket);
    if (ok != ps_success)
	areas.mailPrintf1( 1,"%s\n",buf);

    return(ok);
}   // POP3_sendRetr



/*********************************************************************
  function:      POP3_sendDele
  description:   send the DELE command to the POP3 server.
  arguments:     
    msgnum       message ID number
    socket       socket to which the POP3 server is connected.

  return value:  return code from POP3_ok.
  calls:         POP3_ok, SockPrintF.
 *********************************************************************/

static PopRetCode POP3_sendDele(int msgnum, TSocket &socket)
{
    PopRetCode ok;
    char buf[BUFSIZ];

#ifdef TRACE
    printfT( "POP3_sendDele(%d,.)\n", msgnum );
#endif
    socket.printf("DELE %d\n",msgnum);
    ok = POP3_ok(buf,socket);
    if (ok != ps_success)
	areas.mailPrintf1(1,"%s\n",buf);

    return(ok);
}   // POP3_sendDele



/*********************************************************************
  function:      POP3_readmsg
  description:   Read the message content as described in RFC 1225.
                 RETR with reply evaluation has been done before
  arguments:     
    socket       ... to which the server is connected.
    mboxfd       open file descriptor to which the retrieved message will
                 be written.  
    topipe       true if we're writing to the system mailbox pipe.

  return value:  zero if success else PS_* return code.
  calls:         SockGets.
 *********************************************************************/

static PopRetCode POP3_readmsg(TSocket &socket, TFile &outf)
{
    char buf[BUFSIZ];
    char *bufp;
    int KbRead = 0;
    int OldKbRead = -1;
    int inHeader = 1;
    int firstFromTo = 1;

    //
    //  read the message content from the server
    //
    OldKbRead = -1;
    outf.seek(0L,SEEK_SET);
    for (;;) {
	if (socket.gets(buf,sizeof(buf)) == NULL) {
	    return ps_socket;
	}
	bufp = buf;
	if (*bufp == '\0')
	    inHeader = 0;
	else if (*bufp == '.') {
	    bufp++;
	    if (*bufp == 0)
		break;     // end of message
	}
	outf.printf( "%s\n",bufp );

	if (inHeader  &&  (isHeader(bufp,"From")  ||  isHeader(bufp,"To"))) {
	    areas.mailPrintf1( 0,"%s: %s%s\n", progname,firstFromTo ? "" : "  ", bufp );
	    firstFromTo = 0;
	    OldKbRead = -1;
	}

	KbRead = (TSocket::getBytesRcvd() + TSocket::getBytesXmtd()) / 1000;
	if (KbRead != OldKbRead) {
	    printfT( "(%05dk)\b\b\b\b\b\b\b\b", KbRead );
	    OldKbRead = KbRead;
	}
    }

    return ps_success;
}   // POP3_readmsg



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



static void pop3Cleanup( TSocket &socket, PopRetCode ok )
{
#ifdef TRACE
    printfT( "pop3CleanUp(.,%d)\n",ok );
#endif
    if (ok != ps_success  &&  ok != ps_socket)
	POP3_sendQuit(socket);
    
    if (ok == ps_socket) 
	perror("doPOP3: cleanUp");

    socket.close();
}   // pop3Cleanup



static int pop3Connect( TSocket &socket, const char *host, const char *userid,
			const char *password, int port )
//
//  opens the socket and returns number of messages in mailbox,
//  on error -1 is returned
//
{
    static int firstCall = 1;
    PopRetCode ok;
    int count;

#ifdef TRACE
    printfT( "pop3Connect(.,%s,%s,.,%d )\n", host,userid,port );
#endif
    if (socket.open( host,"pop3","tcp",port ) < 0) {
	if (firstCall)
	    areas.mailPrintf1( 1,"%s: cannot connect to pop3 server %s\n", progname,host);
	firstCall = 0;
	return -1;
    }

    if (firstCall)
	areas.mailPrintf1( 1,"%s: connected to pop3 server %s\n",progname,host );
    
    ok = POP3_ok(NULL, socket);
    if (ok != ps_success) {
	if (ok != ps_socket)
	    POP3_sendQuit(socket);
	socket.close();
	firstCall = 0;
	return -1;
    }

    /* try to get authorized */
    ok = POP3_Auth(userid, password, socket);
    if (ok != ps_success) {
	pop3Cleanup( socket,ok );
	return -1;
    }

    /* find out how many messages are waiting */
    ok = POP3_sendStat(&count, socket);
    if (ok != ps_success) {
	pop3Cleanup( socket,ok );
	return -1;
    }

    /* show them how many messages we'll be downloading */
    if (firstCall)
	areas.mailPrintf1( 1,"%s: you have %d mail message%s\n", progname, count,
			   (count == 1) ? "" : "s");
    firstCall = 0;
    return count;
}   // pop3Connect



static int pop3WriteMail( TAreasMail &msgF, TFile &inF )
//
//  Copy the mail content from the temporary to the SOUP file
//  returns 1 on success, 0 otherwise
//
{
    long msgSize;
    long toRead, wasRead;
    char buf[4096];   // 4096 = good size for file i/o
    int  res;
    TFileTmp tmpF;

    //
    //  Get message size.
    //
    msgSize = inF.tell();
    if (msgSize <= 0)
	return 0;	// Skip empty messages

    msgF.msgStart( "Email","bn" );
    
    //
    //  Copy article body.
    //
    inF.seek(0L, SEEK_SET);
    res = 1;
    while (msgSize > 0) {
	toRead = ((size_t)msgSize < sizeof(buf)) ? msgSize : sizeof(buf);
	wasRead = inF.read(buf, toRead);
	if (wasRead != toRead) {
	    perror("read mail");
	    res = 0;
	    break;
	}
	assert( wasRead > 0 );
	if (msgF.msgWrite(buf, wasRead) != wasRead) {
	    perror("write mail");
	    res = 0;
	    break;
	}
	msgSize -= wasRead;
    }

    msgF.msgStop();
    return res;
}   // pop3WriteMail


    
/*********************************************************************
  function:      getMail
  description:   retrieve messages from the specified mail server
                 using Post Office Protocol 3.

  arguments:     
    options      fully-specified options (i.e. parsed, defaults invoked,
                 etc).

  return value:  exit code from the set of PS_.* constants defined in 
                 popclient.h
  calls:
 *********************************************************************/

int getMail( const char *host, const char *userid, const char *password, int port )
{
    PopRetCode ok;
    TSocket socket;
    int count;
    int percent;
    int msgTotal;
    int msgNumber;
    TFileTmp tmpF;

#ifdef TRACE
    printfT( "getMail(%s,%s,.)\n", host,userid );
#endif

    if (host == NULL) {
	areas.mailPrintf1( 1,"%s: no pop3 server defined\n", progname );
	return 0;
    }

    if ( !tmpF.open("soup")) {
	areas.mailPrintf1( 1,"%s: cannot open temporary mail file\n", progname );
	return 0;
    }

    msgTotal = pop3Connect( socket,host,userid,password,port );
    if (msgTotal < 0)
	return 0;

    for (count = 1, msgNumber = 1;  count <= msgTotal;  ++count, ++msgNumber) {
	percent = (count * 100) / msgTotal;
	printfT("\r%d%%  ", percent);

	ok = POP3_sendRetr(msgNumber,socket);
	if (ok != ps_success) {
	    pop3Cleanup( socket,ok );
	    return 0;
	}
	    
	ok = POP3_readmsg(socket, tmpF);
	if (ok != ps_success) {
	    pop3Cleanup( socket,ok );
	    return 0;
	}
	if ( !pop3WriteMail( areas,tmpF )) {
	    pop3Cleanup( socket, ps_success );
	    areas.mailPrintf1( 1,"%s: cannot copy mail into SOUP file\n", progname );
	    return 0;
	}
	    
	if ( !readOnly) {
	    ok = POP3_sendDele(msgNumber,socket);
	    if (ok != ps_success) {
		pop3Cleanup( socket,ok );
		return 0;
	    }
	}
	if (forceMailDelete  &&  count < msgTotal) {
	    int res;
	    
	    POP3_sendQuit( socket );
	    socket.close();
	    res = pop3Connect(socket,host,userid,password,port);
	    if (res < 0) {
		areas.mailPrintf1( 1,"%s: cannot reconnect to pop3 server %s\n",progname,host );
		return 0;
	    }
	    else if (res == 0)
		msgTotal = 0;    // msg vanished??
	    msgNumber = 0;
	}
    }
    printfT( "\r                    \r" );
    ok = POP3_sendQuit(socket);
    socket.close();
    return 1;
}   // getMail
