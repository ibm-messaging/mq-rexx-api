/* REXX - Publish to a fixed administratively defined topic        */
/* Scenario:                                                       */
/* 1. Define topic name and topic string as specified              */
/* 2. Define local queue for subscriptions                         */
/* 3. Define subscription and relations with queue and topic       */
/* 4. Open previously defined topic                                */
/* 5. Put specified publication to topic                           */
/* 6. Now manually run RXMQSUB1 to consume publication             */
ARG Parm
PARSE VAR Parm QM ' ' TN ' ' TS ' ' PB ' ' trace .

say
say 'Publisher to a fixed administratively defined topic'
say

/* If not on TSO, add INIT REXX function */
env = address()
if env <> 'TSO' then
  rcc = RxFuncAdd('RXMQINIT','RXMQN','RXMQINIT')

                                              /* Set defaults */
if QM == '' then QM = 'WMQW'                  /* QMgr name               */
if TN == '' then TN = 'RXMQFIXEDPRICE'        /* Topic name              */
if TS == '' then TS = 'SWG/RXMQ/FIXED/PRICE'  /* Topic string            */
if PB == '' then PB = '$70000'                /* Publication             */
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
say 'Now administrator defines the following objects:'
say

command = 'DEFINE TOPIC('TN') TOPICSTR('TS') REPLACE'

say
say '1. Define topic name and topic string:'
say '   Issue command <'command'>'
say

data.0 = 0
RXMQTRACE = ''
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display the data */
say
say 'Messages received' data.0
if ( data.0 <> 0 ) then do i=1 to data.0
   say i '<'data.i'>'
end

command = 'DEFINE QLOCAL('SQ') REPLACE'

say
say '2. Define local queue for subscriptions:'
say '   Issue command <'command'>'
say

data.0 = 0
RXMQTRACE = ''
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display the data */
say
say 'Messages received' data.0
if ( data.0 <> 0 ) then do i=1 to data.0
   say i '<'data.i'>'
end

command = 'DEFINE SUB('SN') DEST('SQ') TOPICOBJ('TN') REPLACE'

say
say '3. Define subscription and relations with queue and topic:'
say '   Issue command <'command'>'
say

data.0 = 0
RXMQTRACE = ''
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display the data */
say
say 'Messages received' data.0
if ( data.0 <> 0 ) then do i=1 to data.0
   say i '<'data.i'>'
end

say
say 'Connect to Queue Manager -' QM
say

RXMQTRACE = ''
rcc = RXMQCONN(QM)
 call sayrcc

say
say 'Open predefined topic' TN
say

iod.ver= MQOD_VERSION_4       /* Descriptor needs to be V4             */
iod.ot = MQOT_TOPIC           /* Object is a topic                     */
iod.on = TN                   /* Topic name                            */
iod.ros.0 = 200               /* Resolved topic string max length      */

oo = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING

RXMQTRACE = trace
rcc = RXMQOPEN('iod.', oo, 'h1', 'ood.')
 call sayrcc

   do w=1 to words(ood.zlist)
     wrd = word(ood.zlist,w)
     say left('OD.'wrd,15) '<'ood.wrd'>'
   end


say
say 'Put a retained publication to topic' TN
say

d.1 = PB
d.0 = LENGTH(d.1)

ipmo.opt = MQPMO_FAIL_IF_QUIESCING + MQPMO_RETAIN

RXMQTRACE = trace
rcc      = RXMQPUT(h1,'d.','imd.','omd.','ipmo.','opmo.')
 call sayrcc

   do w=1 to words(omd.zlist)
     wrd = word(omd.zlist,w)
     say left('MD.'wrd,15) '<'omd.wrd'>'
   end
   do w=1 to words(opmo.zlist)
     wrd = word(opmo.zlist,w)
     say left('PMO.'wrd,15) '<'opmo.wrd'>'
   end

say
say 'Published "'PB'" using topic "'TN'" to topic string "'ood.ros.1'"'
say 'Now run RXMQSUB1 sample to consume publication'
say

say
say 'Stop access to a topic' TN
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
/* End of RXMQPUB1 exec */
