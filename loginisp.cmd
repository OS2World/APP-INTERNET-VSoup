/*
  LoginIsp  -  rg040297

  Login to your ISP.  If Option NOWAIT is given, the command will not wait
  until connection has been established.

  You have to do the following configuration:
  -  the line '...logoutisp...' must show the effective logoff script
  -  the line 'checkConnection...' must show a connection checker
  -  the line 'dialer...' must contain a command to establish the connection
  -  the line 'pause...' contains a pause command.  If 'slipwait 2' is used in
     'checkConnection', 'pause' can be set to the empty string
  -  'startAfterConnect...' could be defined optionally
  Note:  the value of MyPPP has been generated with PPP-Fake!
 */

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

checkConnection   = '@ping www.swol.de 10 1 2>&1 > nul'
pause             = 'rc = SysSleep(2)'
dialer            = '@start /c /min' VALUE('MYPPP',,'os2environment' )
startAfterConnect = '@start /c /min diallog.cmd -interface=ppp0 -leave'

TRACE('')

option = ARG(1)

checkConnection
IF rc = 0 THEN DO
    startAfterConnect
    EXIT
END

DO FOREVER
    '@cmd /c logoutisp.cmd'
    '' dialer
    IF option = 'NOWAIT' THEN
	LEAVE

    do j = 1 TO 35
	interpret pause
	checkConnection
	if rc = 0 THEN DO
	    startAfterConnect
	    EXIT
	END
    end j
END

startAfterConnect

EXIT
