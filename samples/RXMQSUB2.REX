/* REXX - Publish to a fixed administratively defined topic        */
/* Scenario:                                                       */
/* 1. Subscribe to variable topic string as specified              */
/* 2. Inquire subscription queue name                              */
/* 3. Get publications from subscription Queue                     */
ARG Parm
PARSE VAR Parm QM ' ' TS ' ' trace.

say
say 'Managed MQ subscriber'
say 'It should be run after RXMQPUB2 publication sample'
say

/* If not on TSO, add INIT REXX function */
env = address()
if env <> 'TSO' then
  rcc = RxFuncAdd('RXMQINIT','RXMQN','RXMQINIT')

                                              /* Set defaults */
if QM == '' then QM = 'WMQW'                  /* QMgr name               */
if TS == '' then TS = 'SWG/RXMQ/VAR/PRICE'    /* Topic string            */


/* Reset elapsed time clock */

t=time('R')

say
say 'Initialize the interface'
say

RXMQTRACE = ''
rcc= RXMQINIT()
 call sayrcc

say
say 'Connect to Queue Manager -' QM
say

RXMQTRACE = ''
rcc = RXMQCONN(QM)
 call sayrcc


say
say 'Subscribe to topic string' TS
say

isd.opt  = MQSO_CREATE + MQSO_MANAGED + MQSO_NON_DURABLE + MQSO_FAIL_IF_QUIESCING
isd.os.1 = TS                 /* Topic string name          */
isd.os.0 = LENGTH(TS)         /* Topic string name length   */
isd.ros.0 = 200               /* Resolved topic string max length      */

RXMQTRACE = trace
rcc = RXMQSUB('isd.', 'h1', 'osd.')
 call sayrcc

   do w=1 to words(osd.zlist)
     wrd = word(osd.zlist,w)
     say left('SD.'wrd,15) '<'osd.wrd'>'
   end

say
say 'Inquire subscription queue name'
say

RXMQTRACE = ''
rcc   = RXMQINQ(h1, MQCA_Q_NAME, 'SQ' )
 call sayrcc

do i=1
  igmo.opt = MQGMO_WAIT + MQGMO_NO_SYNCPOINT + MQGMO_CONVERT
  igmo.wait = 10000              /* Wait interval          */
  imd.ccsi = MQCCSI_Q_MGR        /* Setup CCSID            */
  d.0 = 200                      /* Reserve buffer for pub */

  say
  say 'Waiting' igmo.wait/1000 'seconds for publications from' SQ
  say

  RXMQTRACE = trace
  rcc      = RXMQGET(h1,'d.','imd.','omd.','igmo.','ogmo.')
   call sayrcc

  if ( word(rcc,1) <> 0 ) then leave

   do w=1 to words(omd.zlist)
     wrd = word(omd.zlist,w)
     say left('MD.'wrd,15) '<'omd.wrd'>'
   end
   do w=1 to words(ogmo.zlist)
     wrd = word(ogmo.zlist,w)
     say left('GMO.'wrd,15) '<'ogmo.wrd'>'
   end

  say
  say 'Recevied publication number' i '->"'d.1'"'
  say

end

say
say 'Stop access to queue' SQ
say

RXMQTRACE = ''
rcc = RXMQCLOS(h1, mqco_none)
 call sayrcc


say
say 'Disconnect from' QM
say

RXMQTRACE = ''
rcc = RXMQDISC()
 call sayrcc

say
say 'Remove the Interface functions from the Rexx Workspace ...'
say

RXMQTRACE = ''
rcc = RXMQTERM()
 call sayrcc

exit

sayrcc:
say 'RCC =' rcc
 if RXMQ.LASTRC > 0 then do
   interpret 'lcc = RXMQ.CCMAP.'RXMQ.LASTCC
   interpret 'lac = RXMQ.RCMAP.'RXMQ.LASTAC
   say 'LASTCC =' lcc '; LASTAC =' lac
 end
say 'Elapsed time, sec =' time('R')
return
/* End of TESTSUB2 exec */
