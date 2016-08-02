/* Trigger test - a Rexx/MQ/MVS Example */
ARG Parm
PARSE VAR Parm QMGR ' ' IQueue ' ' .

/* If working on Windows, add INIT function */
env = address()
if env == 'CMD' then
  RxFuncAdd('RXMQINIT','RXMQN','RXMQINIT')

/* Reset elapsed time clock */

t=time('R')

say
say 'Initialize the interface'
say

RXMQTRACE = ''
rcc= RXMQINIT()
 call sayrcc

say
say 'Connect to Queue Manager -' QMGR
say

RXMQTRACE = ''
rcc = RXMQCONN(QMGR)
 call sayrcc

say
say 'Open Queue' IQueue 'for Browse Access'
say

 oo = mqoo_inquire+mqoo_set+MQOO_INPUT_AS_Q_DEF+MQOO_BROWSE
RXMQTRACE = ''
RCC = RXMQOPEN(IQueue, OO, 'H2', 'OOD.' )
 call sayrcc

say
say 'Get message from Initiation Queue'
say

 g.0      = 1024
 g.1      = ''
 igmo.ver= 3
 igmo.opt = MQGMO_WAIT+MQGMO_BROWSE_NEXT
RXMQTRACE = ''
rcc = RXMQGET(h2,'g.','igmd.','ogmd.','igmo.','ogmo.')
 call sayrcc

say '...............' i 'data <'g.1'> length' g.0
say 'ogmd.pd' ogmd.pd 'ogmo.rqn<'ogmo.rqn'>'

if ( word(rcc,1) <> 0 ) then exit

do w=1 to words(ogmd.zlist)
  wrd = word(ogmd.zlist,w)
  say left('MD.'wrd,15) '<'ogmd.wrd'>'
end

do w=1 to words(ogmo.zlist)
  wrd = word(ogmo.zlist,w)
  say left('GMO.'wrd,15) '<'ogmo.wrd'>'
end

say
say 'Extract fields from message header'
say

RXMQTRACE = ''
rcc = RXMQTM('g.','tm.')
 call sayrcc

do w=1 to words(tm.zlist)
  wrd = word(tm.zlist,w)
  say left('TM.'wrd,30) '<'tm.wrd'>'
end

say
say 'And format returned MQTMC2 structure'
say

say left('PL.StrucId'    ,30) '<'substr(tm.pl,  1,  4)'>'
say left('PL.Version'    ,30) '<'substr(tm.pl,  5,  4)'>'
say left('PL.QName'      ,30) '<'substr(tm.pl,  9, 48)'>'
say left('PL.ProcessName',30) '<'substr(tm.pl, 57, 48)'>'
say left('PL.TriggerData',30) '<'substr(tm.pl,105, 64)'>'
say left('PL.ApplType'   ,30) '<'substr(tm.pl,169,  4)'>'
say left('PL.ApplId'     ,30) '<'substr(tm.pl,173,256)'>'
say left('PL.EnvData'    ,30) '<'substr(tm.pl,429,128)'>'
say left('PL.UserData'   ,30) '<'substr(tm.pl,557,128)'>'
say left('PL.QMgrName'   ,30) '<'substr(tm.pl,685, 48)'>'

say
say 'Close queue, disconnect and remove the interface'
say

RXMQTRACE = ''
rcc = RXMQCLOS(h2, mqco_none)
 call sayrcc

/* Remove the Interface functions from the Rexx Workspace ... */

call  RXMQDISC
call  RXMQTERM
exit

sayrcc:
say 'RCC =' rcc
 if RXMQ.LASTRC > 0 then do
   interpret 'lastcc = RXMQ.CCMAP.'RXMQ.LASTCC
   interpret 'lastac = RXMQ.RCMAP.'RXMQ.LASTAC
   say 'LASTCC =' lastcc '; LASTAC =' lastac
 end
say 'Elapsed time, sec =' time('R')
return

