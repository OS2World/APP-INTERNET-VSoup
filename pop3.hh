//  $Id: pop3.hh 1.3 1997/01/19 10:10:50 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  NNTP client routines
//


#ifndef __POP3_HH__
#define __POP3_HH__


int getMail( const char *host, const char *userid, const char *password, int port );


#endif   // __POP3_HH__
