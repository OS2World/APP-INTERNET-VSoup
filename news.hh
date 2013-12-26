//  $Id: news.hh 1.5 1996/10/04 09:02:22 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//


#ifndef __NEWS_HH__
#define __NEWS_HH__


int sumNews( void );
int getNews( int strategy );
int catchupNews( long numKeep );


//
//  file names
//
#define FN_COMMAND "commands"


#endif   // __NEWS_HH__
