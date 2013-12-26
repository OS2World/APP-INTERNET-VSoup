//  $Id: main.cc 1.33 1997/04/20 19:11:34 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@swol.de.
//
//  This file is part of VSoup for OS/2.  VSoup including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  Fetch mail and news using POP3 and NNTP into a SOUP packet.
//

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>

//
//  fÅr TCPOS2.INI-Leserei
//
#ifdef OS2
#define INCL_WINSHELLDATA
#include <os2.h>
#endif

#define DEFGLOBALS
#include "areas.hh"
#include "global.hh"
#include "mts.hh"
#include "news.hh"
#include "pop3.hh"
#include "reply.hh"
#include "socket.hh"
#include "util.hh"


#define VERSION "VSoup v1.2.8 (rg190497)"


static enum { RECEIVE, SEND, CATCHUP } mode = RECEIVE;
static char doMail = 1;
static char doNews = 1;
static char doSummary = 0;
static int  catchupCount;
static clock_t startTime;
static long minThroughput = 0;
static int  threadCntGiven = 0;
static int  newsStrategy = 1;
#ifdef OS2
static char doIni = 1;		// if TRUE, read TCPOS2.INI file
#endif



static void prepareAbort( void )
{
    doAbortProgram = 1;
    TSocket::abortAll();
}   // prepareAbort



static void setFiles( void )
{
#ifdef OS2
    sprintfT(newsrcFile, "%s/newsrc", homeDir);
    sprintfT(killFile, "%s/score", homeDir);
#else
    sprintfT(newsrcFile, "%s/.newsrc", homeDir);
    sprintfT(killFile, "%s/.score", homeDir);
#endif
}   // setFiles



static void getUrlInfo( const char *url, serverInfo &info )
//
//  Get an URL in the form [user[:passwd]@]host[:port]
//  This routine changes only existing fields in 'info'.
//  restrictions:
//  - not optimized for speed
//  - single field length limited to 512 chars (not checked)
//  - portno 0 not allowed
//  - can be fooled easily (with illegal URL specs)
//
{
    char user[512];
    char host[512];
    char username[512];
    char passwd[512];
    char hostname[512];
    char port[512];

    if (sscanfT(url,"%[^@]@%s",user,host) == 2) {
	if (sscanfT(user,"%[^:]:%s",username,passwd) == 2) {
	    xstrdup( &info.user,username );
	    xstrdup( &info.passwd,passwd );
	}
	else
	    xstrdup( &info.user,user );
    }
    else
	strcpy( host,url );

    if (sscanfT(host,"%[^:]:%s",hostname,port) == 2) {
	xstrdup( &info.host,hostname );
	if (atoi(port) != 0)
	    info.port = atoi(port);
    }
    else
	xstrdup( &info.host,host );

#ifdef TRACE
    printfT( "getUrlInfo(%s): %s:%s@%s:%d\n",url,info.user,info.passwd,info.host,info.port );
#endif
}   // getUrlInfo



static void checkTransfer( void * )
//
//  check transfer...
//
{
    long timeCnt;
    long throughputCnt;
    int  throughputErr;
    long oldXfer, curXfer;

#ifdef WININITIALIZE
    WinInitialize(0);
#endif

    timeCnt = 0;
    oldXfer  = 0;
    throughputCnt = 0;
    throughputErr = 6;
    for (;;) {
	sleep( 1 );
	if (doAbortProgram)
	    break;
	
	++timeCnt;
	curXfer = TSocket::getBytesRcvd() + TSocket::getBytesXmtd();

	//
	//  check connect timeout
	//
	if (curXfer < 100  &&  timeCnt > 100) {
	    areas.mailPrintf1( 1,"\n\ncannot connect -> signal %d generated\n",
			       SIGINT );
#ifdef TRACE_ALL
	    printfT( "checkTransfer():  TIMEOUT\n" );
#endif
#ifdef NDEBUG
	    kill( getpid(),SIGINT );
#else
	    areas.mailPrintf1( 1,"NDEBUG not defined -> no signal generated\n" );
#endif
	    break;
	}

	//
	//  check throughput
	//  6 times lower than minThroughput -> generate signal
	//  - minThroughput is mean transferrate over 10s
	//  - minThroughput == -1  ->  disable throughput checker
	//
	++throughputCnt;
	if (curXfer == 0)
	    throughputCnt = 0;
	if (throughputCnt >= 10) {
	    long xfer = curXfer-oldXfer;

	    if (xfer/throughputCnt > labs(minThroughput)  ||  minThroughput == -1)
		throughputErr = 6;
	    else {
		--throughputErr;
		if (throughputErr < 0) {
		    areas.mailPrintf1( 1,"\n\nthroughput lower/equal than %ld bytes/s -> signal %d generated\n",
				       labs(minThroughput),SIGINT );
#ifdef TRACE_ALL
		    printfT( "checkTransfer():  THROUGHPUT\n" );
#endif
#ifdef NDEBUG
		    kill( getpid(),SIGINT );
#else
		    throughputCnt = 30;
#endif
		    break;
		}
	    }
	    if (minThroughput < -1)
		areas.mailPrintf1( 1,"throughput: %lu bytes/s, %d fail sample%s left\n",
				   xfer/throughputCnt,throughputErr,
				   (throughputErr == 1) ? "" : "s");
	    oldXfer = curXfer;
	    throughputCnt = 0;
	}
    }
}   // checkTransfer



static void openAreasAndStartTimer( int argc, char **argv )
{
    int i;
    char serverNames[BUFSIZ];

    //
    //  put for operation important server names into status mail header
    //
    strcpy( serverNames,"" );
    if (nntpInfo.host != NULL  &&
	(mode == CATCHUP  ||  mode == SEND  ||  (mode == RECEIVE  &&  doNews))) {
	strcat( serverNames,nntpInfo.host );
	strcat( serverNames," " );
    }
    if (pop3Info.host != NULL  &&  mode == RECEIVE  &&  doMail) {
	strcat( serverNames,pop3Info.host );
	strcat( serverNames," " );
    }
    if (smtpInfo.host != NULL  &&  mode == SEND)
	strcat( serverNames,smtpInfo.host );
    areas.mailOpen( serverNames );

    //
    //  write all given paramters into StsMail (no passwords...)
    //
    areas.mailStart();
    areas.mailPrintf( "%s started\n", VERSION );
    for (i = 0;  i < argc;  ++i)
	areas.mailPrintf( "%s%s", (i == 0) ? "" : " ", argv[i] );
    areas.mailPrintf( "\n" );
    areas.mailPrintf( "home directory: %s\n", homeDir );
    areas.mailPrintf( "newsrc file:    %s\n", newsrcFile );
    areas.mailPrintf( "kill file:      %s\n", killFile );
    if (mode == CATCHUP  ||  mode == SEND  ||  (mode == RECEIVE  &&  doNews)) {
	areas.mailPrintf( "nntp server:    nntp://%s%s%s",
			  (nntpInfo.user != NULL) ? nntpInfo.user : "",
			  (nntpInfo.user != NULL) ? ":*@" : "", nntpInfo.host );
	if (nntpInfo.port != -1)
	    areas.mailPrintf( ":%d", nntpInfo.port );
	areas.mailPrintf( "\n" );
    }
    if (mode == RECEIVE  &&  doMail) {
	areas.mailPrintf( "pop3 server:    pop3://%s%s%s",
			  (pop3Info.user != NULL) ? pop3Info.user : "",
			  (pop3Info.user != NULL) ? ":*@" : "", pop3Info.host );
	if (pop3Info.port != -1)
	    areas.mailPrintf( ":%d", pop3Info.port );
	areas.mailPrintf( "\n" );
    }
    if (mode == SEND) {
	areas.mailPrintf( "smtp gateway:   smtp://%s", smtpInfo.host );
	if (smtpInfo.port != -1)
	    areas.mailPrintf( ":%d", smtpInfo.port );
	areas.mailPrintf( "\n" );
    }
    areas.mailStop();

    startTime = clock();
}   // openAreasAndStartTimer



static void stopTimerEtc( int retcode )
{
    clock_t deltaTime;
    long deltaTimeS10;

    if (TSocket::getBytesRcvd() + TSocket::getBytesXmtd() != 0) {
	deltaTime = clock() - startTime;
	deltaTimeS10 = (10*deltaTime) / CLOCKS_PER_SEC;
	if (deltaTimeS10 == 0)
	    deltaTimeS10 = 1;
	areas.mailPrintf1( 1,"%s: totally %lu bytes received, %lu bytes transmitted\n",
			   progname, TSocket::getBytesRcvd(), TSocket::getBytesXmtd() );
	areas.mailPrintf1( 1,"%s: %ld.%ds elapsed, throughput %ld bytes/s\n",progname,
			   deltaTimeS10 / 10, deltaTimeS10 % 10,
			   (10*(TSocket::getBytesRcvd()+TSocket::getBytesXmtd())) / deltaTimeS10 );
    }

    areas.mailStart();
    areas.mailPrintf( "%s finished, retcode=%d\n", progname,retcode );
    areas.mailStop();
}   // stopTimerEtc



static void usage( const char *fmt, ... )
{
    va_list ap;

    if (fmt != NULL) {
	char buf[BUFSIZ];
	
	va_start( ap,fmt );
	vsprintfT( buf,fmt,ap );
	va_end( ap );
	areas.mailPrintf1( 1,"%s\n",buf );
    }

    hprintfT( STDERR_FILENO, "%s - transfer POP3 mail and NNTP news to SOUP\n",VERSION );
    hprintfT( STDERR_FILENO, "usage: %s [options] [URLs]\n",progname );
    hputsT("  URL:  (nntp|pop3|smtp)://[userid[:password]@]host[:port]\n",STDERR_FILENO );
    hputsT("global options:\n",STDERR_FILENO );
    hputsT("  -h dir   Set home directory\n", STDERR_FILENO);
#ifdef OS2
    hputsT("  -i       Do not read 'Internet Connection for OS/2' settings\n", STDERR_FILENO);
#endif
    hputsT("  -m       Do not get mail\n", STDERR_FILENO);
    hputsT("  -M       generate status mail\n", STDERR_FILENO);
    hputsT("  -n       Do not get news\n", STDERR_FILENO);
    hputsT("  -r       Read only mode.  Do not delete mail or update newsrc\n", STDERR_FILENO);
    hputsT("  -s       Send replies\n", STDERR_FILENO);
    hputsT("  -T n     limit for throughput surveillance [default: 0]\n", STDERR_FILENO);

    hputsT("news reading options:\n", STDERR_FILENO);
    hputsT("  -a       Add new newsgroups to newsrc file\n", STDERR_FILENO);
    hputsT("  -c[n]    Mark every article as read except for the last n [default: 10]\n", STDERR_FILENO);
    hputsT("  -k n     Set maximum news packet size in kBytes\n", STDERR_FILENO);
    hputsT("  -K file  Set score file\n", STDERR_FILENO);
    hputsT("  -N file  Set newsrc file\n", STDERR_FILENO);
#if defined(OS2)  &&  defined(__MT__)
    hputsT("  -S n     News reading strategy [0..2, default 1]\n", STDERR_FILENO );
    hprintfT( STDERR_FILENO,"  -t n     Number of threads [1..%d, standard: %d]\n",
	      MAXNNTPTHREADS,DEFNNTPTHREADS );
#endif
    hputsT("  -u       Create news summary\n", STDERR_FILENO);
    hputsT("  -x       Do not process news Xref headers\n", STDERR_FILENO);
    hputsT( "mail reading option:\n", STDERR_FILENO );
    hputsT("  -D       force deletion of mail on POP3 server on each message\n", STDERR_FILENO );

    assert( _heapchk() == _HEAPOK );

    areas.forceMail();
    areas.closeAll();

    assert( _heapchk() == _HEAPOK );

    exit( EXIT_FAILURE );
}   // usage



static void parseCmdLine( int argc, char **argv, int doAction )
{
    int c;
    int i;

    optind = 0;
    while ((c = getopt(argc, argv, "?ac::Dh:iK:k:MmN:nrsS:t:T:ux")) != EOF) {
	if (!doAction) {
#ifdef OS2
	    if (c == 'i')
		doIni = 0;
#endif
	}
	else {
	    switch (c) {
		case '?':
		    usage( NULL );
		    break;
		case 'a':
		    doNewGroups = 1;
		    break;
		case 'c':
		    mode = CATCHUP;
		    if (optarg != NULL) {
			catchupCount = atoi(optarg);
			if (catchupCount < 0)
			    catchupCount = 0;
		    }
		    else
			catchupCount = 10;
		    break;
		case 'D':
		    forceMailDelete = 1;
		    break;
		case 'h':
		    homeDir = optarg;
		    setFiles();
		    break;
#ifdef OS2
		case 'i':
		    break;
#endif
		case 'K':
		    strcpy(killFile, optarg);
		    killFileOption = 1;
		    break;
		case 'k':
		    maxBytes = atol(optarg) * 1000L;
		    break;
		case 'M':
		    areas.forceMail();
		    break;
		case 'm':
		    doMail = 0;
		    break;
		case 'N':
		    strcpy(newsrcFile, optarg);
		    break;
		case 'n':
		    doNews = 0;
		    break;
		case 'r':
		    readOnly = 1;
		    break;
		case 's':
		    mode = SEND;
		    break;
#if defined(OS2)  &&  defined(__MT__)
		case 'S':
		    newsStrategy = atoi(optarg);
		    break;
		case 't':
		    maxNntpThreads = atoi(optarg);
		    if (maxNntpThreads < 1  ||  maxNntpThreads > MAXNNTPTHREADS)
			usage( "ill num of threads %s",optarg );
		    threadCntGiven = 1;
		    break;
#endif
		case 'T':
		    minThroughput = atol(optarg);
		    break;
		case 'u':
		    doNews = 1;
		    doSummary = 1;
		    break;
		case 'x':
		    doXref = 0;
		    break;
		default:
		    usage( NULL );
	    }
	}
    }

    //
    //  get the URLs
    //
    if (doAction) {
	for (i = optind;  i < argc;  i++) {
	    if (strnicmp("smtp://",argv[i],7) == 0)
		getUrlInfo( argv[i]+7, smtpInfo );
	    else if (strnicmp("pop3://",argv[i],7) == 0)
		getUrlInfo( argv[i]+7, pop3Info );
	    else if (strnicmp("nntp://",argv[i],7) == 0)
		getUrlInfo( argv[i]+7, nntpInfo );
	    else
		usage( "ill URL %s",argv[i] );
	}
    }
}   // parseCmdLine



#ifdef OS2
static void readTcpIni (void)
{
    HAB hab;
    HINI hini;
    char *etc;
    char buf[BUFSIZ];
    char curConnect[200];

    etc = getenv("ETC");
    if (etc == NULL) {
	hputsT( "Must set ETC\n", STDERR_FILENO );
	exit( EXIT_FAILURE );
    }
    sprintfT(buf, "%s\\TCPOS2.INI", etc);

    hab = WinInitialize(0);
    hini = PrfOpenProfile(hab, buf);
    if (hini == NULLHANDLE) {
	hprintfT( STDERR_FILENO, "cannot open profile %s\n", buf );
	exit(EXIT_FAILURE);
    }

    PrfQueryProfileString(hini, "CONNECTION", "CURRENT_CONNECTION", "",
                          curConnect, sizeof(curConnect));

    PrfQueryProfileString(hini, curConnect, "POPSRVR", "", buf, sizeof(buf));
    xstrdup( &pop3Info.host,buf );
    PrfQueryProfileString(hini, curConnect, "POP_ID", "", buf, sizeof(buf));
    xstrdup( &pop3Info.user,buf );
    xstrdup( &smtpInfo.user,buf );
    xstrdup( &nntpInfo.user,buf );
    PrfQueryProfileString(hini, curConnect, "POP_PWD", "", buf, sizeof(buf));
    xstrdup( &pop3Info.passwd,buf );
    xstrdup( &smtpInfo.passwd,buf );
    xstrdup( &nntpInfo.passwd,buf );    

    PrfQueryProfileString(hini, curConnect, "DEFAULT_NEWS", "", buf, sizeof(buf));
    xstrdup( &nntpInfo.host,buf );

    PrfQueryProfileString(hini, curConnect, "MAIL_GW", "", buf, sizeof(buf));
    xstrdup( &smtpInfo.host,buf );
    
    PrfCloseProfile(hini);
    WinTerminate(hab);

#ifdef DEBUG
    printfT( "TCPOS2.INI information:\n" );
    printfT( "-----------------------\n" );
    printfT( "nntpServer:   %s\n", nntpInfo.host );
    printfT( "popServer:    %s\n", pop3Info.host );
    printfT( "mailGateway:  %s\n", smtpInfo.host );
    printfT( "-----------------------\n" );
#endif
}   // readTcpIni
#endif



static void signalHandler( int signo )
//
//  Signal handling:
//  -  abort the sockets (sockets are unblocked...)
//  -  close the files
//  -  output an error message
//
//  es ist sehr die Frage, ob man hier sprintfT etc verwenden darf, da nicht 100%
//  klar ist, ob ein abgeschossener Thread die zugehîrigen Semaphore freigibt!? (scheint aber so...)
//
{
    //
    //  if SIGINT will be received, consecutive reception of the signal
    //  will be disabled.  Otherwise set handling to default
    //  (e.g. SIGTERM twice will terminate also signalHandler())
    //
    signal( signo, (signo == SIGUSR1  ||  signo == SIGINT) ? SIG_IGN : SIG_DFL );
    signal( signo, SIG_ACK );

#ifndef NDEBUG
    printfT( "\nmain thread received signal %d\n",signo );
#endif

    prepareAbort();

    //  durch's Abbrechen gibt es manchmal einen SIGSEGV der sub-threads, da sie
    //  z.B. auf eine Datei zugreifen, die schon geschlossen wurde.
    //  die Kinder mÅssen zuerst gekillt werden (aber wie??)
    //  Holzhammer:  die Threads brechen bei SIGSEGV ab.  Die Sockets werden auf
    //  NONBLOCK gestellt und lîsen ein SIGUSR1 aus, wenn abgebrochen werden soll
    //

    areas.mailException();          // otherwise semaphors could block forever
    
    areas.mailPrintf1( 1,"\n*** signal %d received ***\n\n",signo );

    stopTimerEtc( EXIT_FAILURE );

    areas.forceMail();          // generate status mail in case of signal reception
    areas.closeAll();

    if ( !readOnly)
	newsrc.writeFile();

    exit( EXIT_FAILURE );       // wird ein raise() gemacht, so werden die exit-Routinen nicht aufgerufen (z.B. files lîschen)
}   // signalHandler



int main( int argc, char **argv )
{
    int retcode = EXIT_SUCCESS;
    char *s;
    
#ifdef DEBUG
    setvbuf( stdout,NULL,_IONBF,0 );
#endif

#ifdef OS2
    if (_osmode != OS2_MODE) {
	hprintfT( STDERR_FILENO,"sorry, DOS not sufficient...\n" );
	exit( EXIT_FAILURE );
    }
#endif
    
    signal(SIGINT,   signalHandler );     // ^C
    signal(SIGBREAK, signalHandler );     // ^Break
    signal(SIGHUP,   signalHandler );     // hang up
    signal(SIGPIPE,  signalHandler );     // broken pipe
    signal(SIGTERM,  signalHandler );     // kill (lÑ·t der sich doch catchen?)
    signal(SIGUSR1,  SIG_IGN );           // ignore this signal in the main thread (which should not be aborted...)
	
    progname = strrchr(argv[0], '\\');
    if (progname == NULL)
	progname = argv[0];
    else
	++progname;

    //
    //  get some environment vars (HOME, NNTPSERVER)
    //
    parseCmdLine(argc, argv, 0);    // only get doIni (-i)
#ifdef OS2
    if (doIni)
	readTcpIni();
#endif
    if ((homeDir = getenv("HOME")) == NULL)
	homeDir = ".";
    if ((s = getenv("NNTPSERVER")) != NULL)
	getUrlInfo( getenv("NNTPSERVER"), nntpInfo );
    setFiles();
    parseCmdLine(argc, argv, 1);
    
    assert( _heapchk() == _HEAPOK );

    //
    //  check the number of free file handles
    //
#ifndef HANDLEERR
    if (mode == RECEIVE  &&  doNews) {
	int h = nhandles(150);    // maximum 150 handles

#ifdef DEBUG_ALL
	printfT( "nhandles() returned %d\n", h );
#endif
	if (2*maxNntpThreads+8 > h) {
	    if (threadCntGiven)
		areas.mailPrintf1( 1,"%s: not enough file handles for %d connected threads\n",
				   progname, maxNntpThreads );
	    maxNntpThreads = (h-8) / 2;
	    if (threadCntGiven)
		areas.mailPrintf1( 1,"%s: number of threads cut to %d (increase the -h setting in the EMXOPT\n\tenvironment variable in CONFIG.SYS, e.g. 'SET EMXOPT=-h40')\n",
				   progname,maxNntpThreads );
	    if (maxNntpThreads < 1)
		exit( EXIT_FAILURE );
	}
    }
#endif
    
    openAreasAndStartTimer(argc,argv);

#if defined(OS2)  &&  defined(__MT__)
    BEGINTHREAD( checkTransfer, NULL );
#endif

    switch (mode) {
    case RECEIVE:
	if (doMail) {
	    if ( !getMail( pop3Info.host,pop3Info.user,pop3Info.passwd,pop3Info.port ))
		retcode = EXIT_FAILURE;
	}
	if (doNews) {
	    if (doSummary) {
		if ( !sumNews())
		    retcode = EXIT_FAILURE;
	    }
	    else {
		if ( !getNews(newsStrategy))
		    retcode = EXIT_FAILURE;
	    }
	}
	break;

    case SEND:
	if ( !sendReply())
	    retcode = EXIT_FAILURE;
	break;

    case CATCHUP:
	if ( !catchupNews(catchupCount))
	    retcode = EXIT_FAILURE;
	break;
    }

    stopTimerEtc( retcode );

    prepareAbort();

    if (retcode != EXIT_SUCCESS)
	areas.forceMail();
    areas.closeAll();

    assert( _heapchk() == _HEAPOK );
    
    exit( retcode );
}   // main
