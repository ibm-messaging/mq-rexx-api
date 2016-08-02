/* Transmission queue header - a Rexx/MQ/MVS Example */
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

 oo = mqoo_inquire+mqoo_output+mqoo_browse+mqoo_set
RXMQTRACE = '*'
RCC = RXMQOPEN(IQueue, OO, 'H2', 'OOD.' )
 call sayrcc

say
say 'Browse Transmission Queue' IQueue
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

RXMQTRACE = 'HXT'
rcc = RXMQHXT('g.','hxt.')
 call sayrcc

do w=1 to words(hxt.zlist)
  wrd = word(hxt.zlist,w)
  say left('HXT.'wrd,30) '<'hxt.wrd'>'
end

say
say 'Close queue and disconnect'
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

