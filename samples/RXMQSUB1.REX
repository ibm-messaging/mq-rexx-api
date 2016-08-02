/* REXX - Consume publication from a pre-defined topic             */
/* Scenario:                                                       */
/* 1. Open subscription Queue                                      */
/* 2. Get publications from subscription Queue                     */
/* 3. Clean-up Topic, Queue and Subscription                       */
ARG Parm
PARSE VAR Parm QM ' ' TN ' ' TS ' ' trace .

say
say 'MQ fixed publication consumer'
say 'It should be run after RXMQPUB1 publication sample'
say

/* If not on TSO, add INIT REXX function */
env = address()
if env <> 'TSO' then
  rcc = RxFuncAdd('RXMQINIT','RXMQN','RXMQINIT')

                                              /* Set defaults */
if QM == '' then QM = 'WMQW'                  /* QMgr name               */
if TN == '' then TN = 'RXMQFIXEDPRICE'        /* Topic name              */
if TS == '' then TS = 'SWG/RXMQ/FIXED/PRICE'  /* Topic string            */
SQ = TN'QUE'                                  /* Subscription queue name */
SN = TN'SUB'                                  /* Subscription name       */

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
say 'Topic and subscription were predefined earlier when RXMQPUB1 was run'
say
say 'Open subscription queue' SQ
say

iod.on = SQ                   /* Subscrption queue name                 */
oo = MQOO_INPUT_AS_Q_DEF + MQOO_FAIL_IF_QUIESCING

RXMQTRACE = trace
rcc = RXMQOPEN('iod.', oo, 'h1', 'ood.')
 call sayrcc

   do w=1 to words(ood.zlist)
     wrd = word(ood.zlist,w)
     say left('OD.'wrd,15) '<'ood.wrd'>'
   end

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
say 'Stop access to a queue' SQ
say

RXMQTRACE = ''
rcc = RXMQCLOS(h1, mqco_none)
 call sayrcc


say
say "Clean-up Topic, Queue and Subscription"
say

data.0 = 0
command = 'DELETE TOPIC('TN')'
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc
/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end

data.0 = 0
command = 'DELETE QLOCAL('SQ') PURGE'
say
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc
/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end

data.0 = 0
command = 'DELETE SUB('SN')'
say
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc
/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end


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
/* End of TESTSUB1 exec */
