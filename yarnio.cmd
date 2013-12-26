/*
 *  make IO for Yarn -  rg060297
 *
 *  Please read the following sections carefully.  For further information
 *  refer to VSoup.Inf/VSoup.Txt.
 *
 *  rg130696:
 *  - now using VSoup for news reception
 *  - big update:
 *    simultaneous mail/news reception/transmission
 *  rg180796:
 *  - 'import' retries in an endlos loop until soup.zip has been accepted
 *  - if SEND fails, the messages are zipped again, which means they can be edited
 *    again in Yarn
 *  rg080996:
 *  - auto connect/disconnect, if required
 *  - user configuration section better indicated
 *  - YarnBinDir no longer required (see config section)
 *  - the secondary newsserver can be defined via YARNIONEWS2 environment variable
 *  - if YARNIONEWS2 is empty, the secondary news server is disabled by default
 *  - yarnio-operation a little bit more secure (chance of packet loss minimized...)
 *  - new command line options for disabling specific services
 *  rg130996:
 *  - semaphor for each service
 *  - legal to start multiple instances of YarnIo
 *  - simple signal handling for logoutISP (signal handling is real b*llsh*t in REXX,
 *    because one has to use the environment vars)
 *  - YarnIo is checking %YARN%\history.pag before starting import...
 *  rg170996:
 *  - configuration file yarnio.set (resides in the same directory as yarnio.cmd)
 *    now user configuration and YarnIo are independent
 *  rg200996:
 *  - zipProg/unzipProg now in YarnIo.Set
 *  rg220996:
 *  - signal handling removed!  To check and abort the connection an extra CMD
 *    will be opened
 *  rg161096:
 *  - zipping now only for SEND operation (to allow reedit)
 *  - areas / *.msg is not imported with 'import -u' (YarnIo.Set must be changed!!)
 *  rg041196:
 *  - now capable of handling up to 9 news/mail server for reception
 *    RCVMAIL2..RCVMAIL9, RCVNEWS2..RCVNEWS9 are all optional and enabled in YARNIO.SET
 *    through definition of the variables 'soupRcvMail2'..'soupRcvMail9' and
 *    'soupRcvNews2'..'soupRcvNews9'
 *  rg171196:
 *  - connect loop changed.  Now 'ping' cmds with delay can also be used (e.g. slipwait)
 *  rg060297:
 *  - bug in 'ping' loop (Connect)
 *
 *  environment vars:
 *  - home
 *  - tmp
 *  - yarn
 *
 *  directory structure (must be set up manually!):
 *  - %home%\yarn\in       - files for reception
 *  - %home%\yarn\in\mail  - files for received mail from primary POP3 server
 *  - %home%\yarn\in\mail2 - files for received mail from POP3 server 2 (optionally)
 *             :
 *  - %home%\yarn\in\mail9 - files for received mail from POP3 server 9 (optionally)
 *  - %home%\yarn\in\news  - files for received news from primaray NNTP server
 *  - %home%\yarn\in\news2 - files for received news from NNTP server 2 (optionally)
 *             :
 *  - %home%\yarn\in\news9 - files for received news from NNTP server 9 (optionally)
 *  - %home%\yarn\out      - souper packets for transmission
 *  - %yarn%               - yarn subdir (history.pag required)
 *
 *  external utilities (configured in yarnio.set):
 *  - vsoup.exe        - soup packet receiver transmitter
 *  - import.exe       - import messages to Yarn database
 *  - zip              - zip files
 *  - unzip            - unzip files
 *  - ping             - ping a host (exit status 0 indicates there is a connection)
 *  - loginisp.cmd     - connect to your ISP
 *  - logoutisp.cmd    - disconnect from your ISP
 *                       if you have an auto-dialer, loginisp/logoutisp can be empty cmd files
 *
 *  options:
 *  -SEND      - no mail/news transmission
 *  -RCVMAIL   - no mail reception primary POP3 server
 *  -RCVMAIL2  - no mail reception 2nd POP3 server
 *       :
 *  -RCVMAIL9  - no mail reception 9th POP3 server
 *  -RCVNEWS   - no news reception from primary newsserver
 *  -RCVNEWS2  - no news reception from newsserver #2
 *       :
 *  -RCVNEWS9  - no news reception from newsserver #9
 *
 *  internal options:
 *   SEND      - start the send process
 *   RCVMAIL   - start the receive mail process
 *   RCVMAIL2 .. RCVMAIL9
 *   RCVNEWS   - start the receive news process
 *   RCVNEWS2 -- RCVNEWS9
 *   CHECKISP  - if YarnIo failes, connection will be killed
 *  with no option given, YarnIo starts all the above processes
 *
 *  todo:
 *  - restructure script, so that there are several procedures (& stems)
 *  - output to several servers (smtp & nntp) - requires an external utility
 */

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs


rc = SysCls()
SAY 'YarnIo -- Copyright (c) 1996 by Hardy Griech'
SAY ''
SAY ''


DEBUG = 0

/*
 *  get some global vars
 */
env = 'os2environment'
homeDir = value('home',,env)
tmpDir  = value('tmp',,env)
yarnDir = value('yarn',,env)
parse source . . compCmdName
cmdName = filespec('name',compCmdName)
CompSetName = delstr(CompCmdName,lastpos('.',CompCmdName)) || '.set'

IF (homeDir = '')  |  (tmpDir = '')  |  (yarnDir = '') THEN DO
    call Help 'Either HOME, TMP or YARN not defined in environment'
END

/*********************************************************************************
 *********************************************************************************
 *
 *  begin of user configuration section
 *
 *  define some global vars (commands / directories)
 *  change, if you like to
 *
 *  NEVERTHELESS IT IS NOT RECOMMENDED TO CHANGE ANYTHING IN THIS SECTION
 *       TRY TO KEEP YOUR CONFIGURATION IN THE YARNIO.SET FILE
 */

rc            = setlocal()
xmtDir        = homeDir || '\yarn\out'
soupXmtZip    = 'reply.zip'                 /* same as in yarn-config! */
rcvDir        = homeDir || '\yarn\in'
rcvNewsDir    = rcvDir  || '\news'
rcvMailDir    = rcvDir  || '\mail'
DO ii = 2 TO 9
    dummy = VALUE( 'rcvNews' || ii || 'Dir', rcvNewsDir || ii )
    dummy = VALUE( 'rcvMail' || ii || 'Dir', rcvMailDir || ii )
END

/*
 *  set up some configuration defaults
 */
soupSend      = ''
soupRcvNews   = ''
soupRcvMail   = ''
echoProlog    = 'echo' CompSetName || ': no'
preImportProg = ''
importProg    = echoProlog 'importProg defined:'
pingHost      = echoProlog 'pingHost defined'
connectISP    = echoProlog 'connectISP defined'
hangupISP     = echoProlog 'hangupISP defined'
unzipProg     = echoProlog 'unzipProg defined'

/*
 *  the following could be changed, but this is not required (at least if you have zip/unzip)
 */
yarnHistory   = yarnDir || '\history.pag'
soupRcvAreas  = 'areas'
SoupRcvMsgs   = '*.msg'
SoupXmtReply  = 'replies'
SoupXmtMsgs   = '*.msg'

/*
 *  interprete the user configuration
 */
if \FileExists(CompSetName) then do
   say CompSetName 'required for configuration'
   exit 3
END

CALL GetIniFile

unzipProg     = unzipProg soupXmtZip
zipProgSend   = zipProg soupXmtZip soupXmtReply soupXmtMsgs

/*
 *  end of configuration section
 *
 *********************************************************************************
 *********************************************************************************/

'@set EMXOPT=-h40 -c -n'   /*  set the maximum number of handles to 40  */

/*
 *  files for internal use
 */
semActive   = tmpdir  || '\yarnio.act'
semLocal    = '.\yarnio.act'
semImport   = tmpdir  || '\yarnio.imp'
flgRcvNews  = tmpdir  || '\yarnio.fn'
flgRcvMail  = tmpDir  || '\yarnio.fm'
flgXmt      = tmpDir  || '\yarnio.fx'
DO ii = 2 TO 9
    dummy = VALUE( 'flgRcvNews' || ii, flgRcvNews || ii )
    dummy = VALUE( 'flgRcvMail' || ii, flgRcvMail || ii )
END


if \DEBUG then do
    startProc = 'start /c /b /min'
end
else do
    startProc = 'start /c /b'
end

if DEBUG then 
    trace('?i')

/*
 *  get command line options
 */
Option = ''
AddOption = ''
parse upper arg ropt
do while ropt \= ''
    parse var ropt opt ropt
    add = 1
    select
	when LEFT(opt,8) = '-RCVMAIL'  &  SYMBOL('soup' || SUBSTR(opt,2)) = 'VAR' THEN
	    dummy = VALUE( 'soup' || SUBSTR(opt,2), '' );
	when LEFT(opt,8) = '-RCVNEWS'  &  SYMBOL('soup' || SUBSTR(opt,2)) = 'VAR' THEN
	    dummy = VALUE( 'soup' || SUBSTR(opt,2), '' );
	when opt = '-SEND' then
	    soupSend = ''
	otherwise do
	    Option = opt
	    add = 0
	end
    end
    if add then
	AddOption = AddOption opt
end

/*
 *  do the thing
 */
if Option = '' then do
    wasConnected = 1;
    haveSema = 0;
    if stream(semActive,'c','open write') = 'READY:' then do
	haveSema = 1;
	wasConnected = Connect()
    end
    /*
     *  initiate all other processes and wait for completion
     */
    rc = time('R')
    say 'starting the transmitting/receiving processes...'
    rc = DeleteFile( flgRcvNews )    /* fails, if already started... */
    '@' startProc '"RCVNEWS:' soupRcvNews || '"'  CompCmdName 'RCVNEWS'  AddOption
    rc = DeleteFile( flgRcvMail )
    '@' startProc '"RCVMAIL:' soupRcvMail || '"'  CompCmdName 'RCVMAIL'  AddOption
    rc = DeleteFile( flgXmt )
    '@' startProc '"SEND:' soupSend || '"'     CompCmdName 'SEND'     AddOption
    DO ii = 2 TO 9
	rc = DeleteFile( VALUE('flgRcvNews' || ii) )
	IF SYMBOL('soupRcvNews' || ii) = 'VAR' THEN
	    '@' startProc '"RCVNEWS' || ii || ':' VALUE('soupRcvNews'||ii) || '"' CompCmdName 'RCVNEWS' || ii AddOption
	rc = DeleteFile( VALUE('flgRcvMail' || ii) )
	IF SYMBOL('soupRcvMail' || ii) = 'VAR' THEN
	    '@' startProc '"RCVMAIL' || ii || ':' VALUE('soupRcvMail'||ii) || '"' CompCmdName 'RCVMAIL' || ii AddOption
    END
    
    State = 'aborted'
    if haveSema then do
	say 'waiting...'
	say ''
	rc = SysSleep( 10 )     /*  time to start subprocesses  */
	loopCnt = 0
	/*
	 *  This loop waits until no flgXmt/flgRcv remains
	 *  After that operations is aborted
	 */
	do forever
	    rc = SysSleep( 1 )
	    loopCnt = loopCnt + 2
	    if loopCnt > 3600 then do
		say 'timeout:  cleaning up and exit...'
		leave
	    END
	    te = TIME('E')  /* **** */
	    SAY te || 's'
	    rc = DispStatus( 'online:' format(te,,1) || 's' )
	    rc = CheckFlag( flgXmt,     'SEND:    ' );
	    rc = CheckFlag( flgRcvMail, 'RCVMAIL: ' );
	    rc = CheckFlag( flgRcvNews, 'RCVNEWS: ' );
	    DO ii = 2 TO 9
		rc = CheckFlag( VALUE('flgRcvNews' || ii), 'RCVNEWS' || ii || ':' );
		rc = CheckFlag( VALUE('flgRcvMail' || ii), 'RCVMAIL' || ii || ':' );
	    END
	    rc = SysSleep( 1 )
	    ex = \FileExists(flgRcvNews) & \FileExists(flgRcvMail) & \FileExists(flgXmt);
	    DO ii = 2 TO 9
		ex = ex & \FileExists(VALUE('flgRcvNews' || ii)) & \FileExists(VALUE('flgRcvMail' || ii));
	    END
	    IF ex THEN DO
		SAY ''
		say 'finished...'
		leave
	    end
	end
	State = 'ok'
    end
    else
	State = 'already active'
    rc = CleanUp( State )
end
else if Option = 'CHECKISP' then do
    say 'checking the connection...'
    do forever
	rc = SysSleep( 5 )
	if stream(semActive,'c','open write') = 'READY:' then do
	    rc = DeleteFile( semActive )
	    rc = SysSleep( 5 )
	    rc = hangup( 1 )
	    rc = SysSleep( 5 )
	    exit 0
	end
    end
end
else if Option = 'SEND' then do
    /*
     *  transmit messages (news & mail)
     */
    text = CheckCmd(soupSend)
    if text = '' then do
	text = SetWorkingDir(xmtDir)
	if text = '' then DO
	    text = SetFlag(flgXmt,semLocal)
	    if text = '' then do
		rc = DoImport()
		text = '-- nothing done'
		if FileExists(soupXmtZip) then do
		    '@' unzipProg
		END
		IF FileExists(soupXmtReply) THEN DO
		    SAY 'sending messages...'
		    SAY soupSend
		    '@' soupSend
		    text = 'msg(s) transmitted'
		    if rc \= 0 then
			text = soupSend 'returned with an error'
		    rc = DoImport()                  /* deletes all *.msg belonging to area */
		    rc = SysFileDelete( soupXmtZip )
		    if FileExists(soupXmtReply) then do
			say 'moving' soupXmtMsgs 'again into' soupXmtZip
			'@' zipProgSend
		    end
		END
		rc = SendFlag( flgXmt,text )
		rc = DeleteFile( semLocal )
	    end
	end
    end
end
else if LEFT(Option,7) = 'RCVMAIL'  &  SYMBOL('soup' || Option) = 'VAR'  &  SYMBOL(Option || 'Dir') = 'VAR' then
    text = DoReception( VALUE('soup' || Option), VALUE(Option || 'Dir'), VALUE('flg' || Option) )
else if LEFT(Option,7) = 'RCVNEWS'  &  SYMBOL('soup' || Option) = 'VAR'  &  SYMBOL(Option || 'Dir') = 'VAR' then
    text = DoReception( VALUE('soup' || Option), VALUE(Option || 'Dir'), VALUE('flg' || Option) )
ELSE DO
    call Help 'ill option:' Option
END

say Option || ':' text
if substr(text,1,1) \= "-" then do
    rc = SysSleep( 30 )   /* do not close window immediately */
    say 'goodbye'         /* last chance for ^S...           */
end
exit 0



CleanUp:  procedure expose hangupISP semActive wasConnected cmdName
State = arg(1)

if State = 'ok'  |  State = 'aborted' then do
    say CmdName 'took' format(TIME('E'),,1) || 's'
    
    if \wasConnected then
	rc = Hangup(1)
    say State
    rc = DeleteFile(semActive)
    rc = SysSleep( 30 )
    SAY 'goodbye'
    if State = 'aborted' then
	exit 3
    exit 0
end
else do
    say CmdName 'already active'
    say '(if actually not, delete' semActive 'manually)'
    exit 3 
end

AbsExit:
exit 99



/********************************************************************************
 *
 *  Do a single reception procedure
 */
DoReception: procedure expose semImport semLocal preImportProg importProg,
                              soupRcvAreas soupRcvMsgs yarnHistory
cmd     = arg(1)
dir     = arg(2)
flgfile = arg(3)

text = CheckCmd(cmd)
if text = '' then do
    text = SetWorkingDir(dir)
    if text = '' then do
	text = SetFlag(flgfile,semLocal)
	if text = '' then do
	    rc = DoImport()
	    say cmd
	    '@' cmd
	    text = 'ok'
	    if rc \= 0 then
		text = cmd 'returned with an error'
	    rc = SendFlag( flgfile,text )     /* hier kann es ein Problem geben, wenn eine zweite Instanz gestartet wurde */
	    rc = DoImport()
	end
	rc = DeleteFile( semLocal )
    end
end
return text



/********************************************************************************
 *
 *  Import the received data to the Yarn database
 *  - on entry the current subdir contains the areas, *.msg or zip-files
 *  - sets semaphor to avoid simultaneous imports
 *
 */
DoImport: procedure expose semImport preImportProg importProg ,
    soupRcvAreas soupRcvMsgs yarnHistory

/*
 *  wait until semaphor is free
 */
First = 1
do forever
    if \FileExists(soupRcvAreas)  &  \FileExists(soupRcvMsgs) then DO    /*******????****/
	if \First then
	    say soupRcvAreas 'vanished (was another instance active?)'
	return 0
    end
    if stream(semImport,'c','open write') = 'READY:' then
	leave
    if First then do
	say 'import locked, waiting for access'
	First = 0
    end
    rc = SysSleep( 5 )
end

IF preImportProg \= '' THEN
    '@' preImportProg

/*
 *  repeat import until soupRcvAreas has been successfully read...
 */
do forever
    if stream(yarnHistory,'c','open write') = 'READY:' then do
	rc = stream(yarnHistory,'c','close')   /**** hier kann was schiefgehen... ****/
	'@' importProg
	if \FileExists(soupRcvAreas) then
	    leave
    end
    if First then do
	say 'import locked, waiting for access'
	First = 0
    end
    rc = SysSleep( 5 )
end
rc = DeleteFile( semImport )
return 1



/********************************************************************************
 *
 *  set the working directory
 *  if operation fails, return value is an error message, 
 *  otherwise empty string is returned
 */
SetWorkingDir: procedure
dir = arg(1)

if translate(directory(dir)) = translate(dir) then do
    say 'working directory is' dir
    return ''
end
return 'cannot change to' dir



/********************************************************************************
 *
 *  Check flag file
 */
CheckFlag: procedure
file = arg(1)
msg  = arg(2)

if FileExists(file) then DO
    IF stream(file,'c','open read') = 'READY:' THEN DO
	txt = linein( file )
	rc = DeleteFile( file )
	say msg "'" || txt || "'"
	if rc \= 0 then
	    say 'cannot delete' file || ', rc=' || rc
	return 1
    end
end
return 0



SetFlag: PROCEDURE
flgfile = ARG(1)
semfile = ARG(2)

/*
 *  there is a very small case, when this could fail:
 *  another instance starts & finishs during the '|'
 */
IF stream(semfile,'c','open write') \= 'READY:' THEN
    RETURN '-- blocked'
IF stream(flgfile,'c','open write') \= 'READY:' THEN DO
    rc = DeleteFile(semFile)
    RETURN '-- blocked'
END
RETURN ''



SendFlag: procedure
file = arg(1)
msg  = arg(2)

rc = lineout( file,msg )
rc = lineout( file )
return 1



CheckCmd: procedure
cmd = arg(1)

if cmd = '' then
    return '-- disabled'
return ''



DeleteFile: procedure
f = arg(1)

rc = stream( f,'c','close' )
rc = SysFileDelete( f )
return rc



/********************************************************************************
 *
 *  Connect to ISP, usually calls 'loginisp.cmd'
 *  If you have an auto dialer, then the pinging should be enough to start
 *  connecting.  'loginisp.cmd' can thus be an empty script...
 *
 *  Requires the time('R')/('S') commands to stop the time.  This is due
 *  to delay of the ping command (e.g. 'slipwait 2' takes two seconds, 'ping host 1'
 *  almost no time).
 *
 *  Thanx to scgf@netcomuk.co.uk (Phillip Deackes) for the idea
 */
Connect: procedure expose hangupISP connectISP pingHost CompCmdName startProc DEBUG

IF DEBUG THEN
    RETURN 1

rc = charout(, 'checking connection --> ' )

rc = TIME('R')
do FOREVER
    IF TIME('E') > 2 THEN
	LEAVE
    '@' pingHost
    IF rc = 0 THEN DO
	SAY 'ok'
	RETURN 1
    END
END

say 'connecting to ISP'
'@' startProc '"YarnIo CHECKISP"' CompCmdName 'CHECKISP'

do i = 1 to 10
    IF i \= 1 THEN
	SAY 'retrying...'
    '@' connectISP
    rt = 80
    DO WHILE rt >= 0
        rc = DispStatus( format(rt,,1) || 's left' )
	rc = TIME('R')
	'@' pingHost
	dt = TIME('E')
	rt = rt - dt
	IF dt < 0.5 THEN DO 
	    call SysSleep 1
	    rt = rt - 1
	END
	IF rc = 0 THEN DO
	    rc = DispStatus( '' )
	    RETURN 0
	END
    END
    rc = DispStatus( '' )
    rc = Hangup( 0 )
    rc = SysSleep( 5 )
END i
SAY ''
say '*****************************'
say '*** impossible to connect ***'
say '*** check your equipment  ***'
say '*****************************'
say 'aborted...'
DO FOREVER
    rc = SysSleep( 6000 )
END
return 0



/********************************************************************************
 *
 *  Hangup connection to your ISP, usually calls 'logoutisp.cmd'
 *  The called script/progam could do whatever you want, e.g. if you have
 *  established other connections the 'logoutisp.cmd' could reject the hangup etc...
 */
Hangup: procedure expose hangupISP
sayMsg = arg(1)

if sayMsg then
    say 'hanging up connection'
'@' hangupISP
return 1



DispStatus: PROCEDURE
msg = LEFT( ARG(1),25,' ' )

IF msg \= '' THEN
    msg = 'Sts:' msg

PARSE VALUE SysCurPos( 0,48 ) WITH l c
SAY msg
rc = SysCurPos( l,c );
RETURN 1



FileExists: procedure
filemask = arg(1)
rc = SysFileTree( filemask,res,F,, )
return res.0 \= 0



Help: PROCEDURE EXPOSE CompSetName 
helpMsg = ARG(1)

CALL GetIniFile

say 'failed:' helpMsg
say ''
say CmdName '0.93 rg060297'
say CmdName 'receives and transmits news/mail to Yarn via VSoup'
say ''
say 'options (for disabling corresponding service):'
msg = ''
IF soupSend \= '' THEN
    msg = '-SEND '
IF soupRcvMail \= '' THEN
    msg = msg || '-RCVMAIL '
IF soupRcvNews \= '' THEN
    msg = msg || '-RCVNEWS '
SAY msg

msg = ''
DO ii = 2 TO 9
    IF SYMBOL('soupRcvMail' || ii) = 'VAR' THEN
	msg = msg || '-RCVMAIL' || ii || ' '
    IF SYMBOL('soupRcvNews' || ii) = 'VAR' THEN
	msg = msg || '-RCVNEWS' || ii || ' '
END
SAY msg
SAY
say 'no options given:  perform all services'
exit( 3 )



GetIniFile:

IF stream(CompSetName,'c','open read') \= 'READY:' THEN DO
    SAY 'cannot open' CompSetName
    rc = SysSleep( 30 )
    SAY 'goodbye'
    EXIT( 3 )
END

do WHILE LINES(CompSetName) \= 0
    line = linein(CompSetName)
    if substr(strip(line),1,1) \= '#' THEN DO
	interpret line
    END
END
rc = stream( CompSetName,'c','close' )
RETURN
