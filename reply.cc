//  $Id: reply.cc 1.17 1997/01/20 16:29:40 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  Send reply packet.
//


#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "global.hh"
#include "mts.hh"
#include "reply.hh"
#include "util.hh"

#include "nntpcl.hh"
#include "smtp.hh"
#include "socket.hh"

static TSocket smtpSock;
static TNntp nntpReply;
static char *mailer;
static char *poster;



static int sendPipe( TFile &fd, size_t bytes, const char *agent )
//
//  Pipe a message to the specified delivery agent.
//
{
    FILE *pfd;
    char buff[4096];
    int cnt;

    /* Open pipe to agent */
    if ((pfd = popenT(agent, "wb")) == NULL) {
	areas.mailPrintf1( 1,"%s: cannot open reply pipe %s\n", progname,agent );
	while (bytes > 0) {
	    cnt = (bytes >= sizeof(buff)) ? sizeof(buff) : bytes;
	    fd.read( buff,cnt );
	    bytes -= cnt;
	}
	return 0;
    }

    /* Send message to pipe */
    while (bytes > 0) {
	cnt = (bytes >= sizeof(buff)) ? sizeof(buff) : bytes;
	if (fd.read(buff,cnt) != cnt) {
	    areas.mailPrintf1( 1,"%s: ill reply file: %s\n", progname,fd.getName() );
	    return 0;
	}
	fwriteT( buff,1,cnt,pfd );
	bytes -= cnt;
    }

    pcloseT(pfd);
    return 1;
}   // sendPipe



static int sendMail( TFile &inf, size_t bytes )
{
    int res = 1;
    if (mailer) {
	const char *to = getHeader(inf, "To");
	areas.mailPrintf1(1,"%s: mailing to %s\n", progname, to);
////	delete to;

	/* Pipe message to delivery agent */
	res = sendPipe(inf, bytes, mailer);
    }
    else {
	if ( !smtpMail(smtpSock, inf, bytes)) {
	    areas.mailPrintf1( 1,"%s: cannot deliver mail\n",progname );
	    res = 0;
	}
    }
    return res;
}   // sendMail



static int sendNews( TFile &inf, size_t bytes )
{
    int res = 1;
    const char *grp;

    grp = getHeader( inf, "Newsgroups" );
    areas.mailPrintf1(1,"%s: posting article to %s\n", progname, grp);
////	delete grp;

    if (poster) {
	/* Pipe message to delivery agent */
	res = sendPipe(inf, bytes, poster);
    } else {
	if (nntpReply.postArticle( inf,bytes ) != TNntp::ok) {
	    areas.mailPrintf1( 1,"%s: cannot post article: %s\n",
			       progname,nntpReply.getLastErrMsg());
	    res = 0;
	}
    }
    return res;
}   // sendNews



static int sendMailu (const char *fn)
//
//  Process a mail reply file, usenet type (is that one really correct???)
//
{
    char buf[BUFSIZ];
    TFile fd;
    int bytes;
    int res = 1;

    //
    //  Open the reply file
    //  problem here is non-fatal!
    //
    if ( !fd.open(fn,TFile::mread,TFile::obinary)) {
	areas.mailPrintf1( 1,"%s: cannot open file %s\n", progname,fn );
	return 1;
    }

    /* Read through it */
    while (fd.fgets(buf,sizeof(buf),1)) {
	if (strncmp (buf, "#! rnews ", 9)) {
	    areas.mailPrintf1( 1,"%s: malformed reply file\n", progname);
	    res = 0;
	    break;
	}

	/* Get byte count */
	sscanfT(buf+9, "%d", &bytes);

	if ( !sendMail(fd, bytes)) {
	    res = 0;
	    break;
	}
    }
    fd.close();
    return res;
}   // sendMailu



static int sendNewsu (const char *fn)
//
//  Process a news reply file, usenet type
//
{
    char buf[BUFSIZ];
    TFile fd;
    int bytes;
    int res = 1;

    //
    //  Open the reply file
    //  problem here is non-fatal!
    //
    if ( !fd.open(fn,TFile::mread,TFile::obinary)) {
	areas.mailPrintf1( 1,"%s: cannot open file %s\n", progname,fn );
	return 1;
    }

    /* Read through it */
    while (fd.fgets(buf,sizeof(buf),1)) {
	if (strncmp (buf, "#! rnews ", 9)) {
	    areas.mailPrintf1( 1,"%s: malformed reply file\n", progname);
	    res = 0;
	    break;
	}

	sscanfT(buf+9, "%d", &bytes);
	if ( !sendNews(fd, bytes)) {
	    res = 0;
	    break;
	}
    }
    fd.close();
    return res;
}   // sendNewsu



static int sendMailb (const char *fn)
//
//  Process a mail reply file, binary type
//  The binary type is handled transparent, i.e. CRLF are two characters!
//
{
    unsigned char count[4];
    TFile fd;
    int bytes;
    int res = 1;

    //
    //  Open the reply file
    //  problem here is non-fatal!
    //
    if ( !fd.open(fn,TFile::mread,TFile::obinary)) {
	areas.mailPrintf1( 1,"%s: cannot open file %s\n", progname,fn );
	return 1;
    }

    /* Read through it */
    while (fd.read(count,4) == 4) {
	/* Get byte count */
	bytes = ((count[0]*256 + count[1])*256 + count[2])*256 + count[3];
	if ( !sendMail(fd, bytes)) {
	    res = 0;
	    break;
	}
    }

    fd.close();
    return res;
}   // sendMailb



static int sendNewsb (const char *fn)
//
//  Process a news reply file, binary type
//  The binary type is handled transparent, i.e. CRLF are two characters!
//
{
    unsigned char count[4];
    TFile fd;
    int bytes;
    int res = 1;

    //
    //  Open the reply file
    //  problem here is non-fatal!
    //
    if ( !fd.open(fn,TFile::mread,TFile::obinary)) {
	areas.mailPrintf1( 1,"%s: cannot open file %s\n", progname,fn );
	return 1;
    }

    /* Read through it */
    while (fd.read(count, 4) == 4) {
	bytes = ((count[0]*256 + count[1])*256 + count[2])*256 + count[3];
	if ( !sendNews(fd, bytes)) {
	    res = 0;
	    break;
	}
    }
    fd.close();
    return res;
}   // sendNewsb



int sendReply (void)
//
//  Process a reply packet.
//
{
    TFile rep_fd;
    char buf[BUFSIZ];
    char fname[FILENAME_MAX], kind[FILENAME_MAX], type[FILENAME_MAX];
    int mailError = 0;
    int nntpError = 0;

    mailer = getenv("MAILER");
    poster = getenv("POSTER");
    
    //
    //  Open the packet
    //  if none exists -> non-fatal
    //
    if ( !rep_fd.open(FN_REPLIES,TFile::mread,TFile::otext)) {
	areas.mailPrintf1( 1,"%s: can't open file %s\n", progname, FN_REPLIES);
	return 0;
    }

    /* Look through lines in REPLIES file */
    while (rep_fd.fgets(buf, sizeof(buf), 1)) {
	if (sscanfT(buf, "%s %s %s", fname, kind, type) != 3) {
	    areas.mailPrintf1( 1,"%s: malformed REPLIES line: %s\n", progname,buf);
	    return 0;
	}

	/* Check reply type */
	if (type[0] != 'u' && type[0] != 'b' && type[0] != 'B') {
	    areas.mailPrintf1( 1,"%s: reply type %c not supported\n", progname,
			       type[0]);
	    areas.forceMail();            // indicates corrupt replies file
	    continue;
	}

	//
	//  Look for mail or news
	//  and first try to connect
	//
	if (type[0] == 'b'  ||  (type[0] == 'u'  &&  strcmp(kind, "mail") == 0)) {
	    if (mailError)
		continue;
	    if ( !mailer  &&  smtpSock.state() != TSocket::connected) {
		if (smtpInfo.host == NULL) {
		    areas.mailPrintf1( 1,"%s: no smtp gateway defined\n", progname );
		    mailError = 1;
		    continue;
		}
		else if ( !smtpConnect(smtpSock)) {
		    areas.mailPrintf1( 1,"%s: cannot connect to smtp gateway %s\n",
				       progname, smtpInfo.host );
		    mailError = 1;
		    continue;
		}
		else
		    areas.mailPrintf1( 1,"%s: connected to smtp gateway %s\n",
				       progname, smtpInfo.host );
	    }
	} else if (type[0] == 'B'  ||  (type[0] == 'u'  &&  strcmp(kind, "news") == 0)) {
	    if (nntpError)
		continue;
	    if ( !poster) {
		if (nntpInfo.host == NULL) {
		    areas.mailPrintf1( 1,"%s: no news server defined\n", progname );
		    nntpError = 1;
		    continue;
		}
		else if (nntpReply.open(nntpInfo.host,nntpInfo.user,nntpInfo.passwd,nntpInfo.port) != TNntp::ok) {
		    areas.mailPrintf1( 1,"%s: cannot connect to news server %s (post):\n\t%s\n",
				       progname, (nntpInfo.host != NULL) ? nntpInfo.host : "\b",
				       nntpReply.getLastErrMsg() );
		    nntpError = 1;
		    continue;
		}
		else
		    areas.mailPrintf1( 1,"%s: connected to news server %s (post)\n",
				       progname, nntpInfo.host );
	    }
	}
	else {
	    areas.mailPrintf1( 1,"%s: bad reply kind: %s\n", progname, kind);
	    areas.forceMail();     // indicates corrupt replies file
	    continue;
	}

	/* Make file name */
	strcat(fname, ".MSG");

	/*
	**  Wenn Datei nicht existiert heiát das, daá sie schon
	**  versendet wurde (und dies sozusagen ein RETRY ist)
	*/

	/* Process it */
	switch (type[0]) {
	    case 'u':
		if (strcmp(kind, "mail") == 0) {
		    if ( !sendMailu(fname)) {
			mailError = 1;
			continue;
		    }
		}
		else if (strcmp(kind, "news") == 0) {
		    if ( !sendNewsu(fname)) {
			nntpError = 1;
			continue;
		    }
		}
		break;
	    case 'b':
		if ( !sendMailb(fname)) {
		    mailError = 1;
		    continue;
		}
		break;
	    case 'B':
		if ( !sendNewsb(fname)) {
		    nntpError = 1;
		    continue;
		}
		break;
	}

	/* Delete it */
	if ( !readOnly)
	    removeT(fname);
    }

    if (smtpSock.state() == TSocket::connected)
	smtpClose( smtpSock );
    nntpReply.close( 1 );

    //
    //  remove REPLIES only, if successfull
    //
    if ( !readOnly  &&  !mailError  &&  !nntpError)
	rep_fd.remove();
    else
	rep_fd.close();

    return !mailError  &&  !nntpError;
}   // sendReply
