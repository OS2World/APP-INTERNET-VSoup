//  $Id: smtp.hh 1.3 1996/10/01 13:59:11 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//


#ifndef __SMTP_HH__
#define __SMTP_HH__


#include "socket.hh"


int  smtpConnect( TSocket &socket );
void smtpClose( TSocket &socket );
int  smtpMail( TSocket &socket, TFile &file, size_t bytes );


#endif   // __SMTP_HH__
