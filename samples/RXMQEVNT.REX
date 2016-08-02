/* REXX - Read EVENT queue and interpret results                   */
/* Scenario:                                                       */
/* 1. Open specified Event Queue & Read Event Messages             */
/* 2. Call RXMQEVNT function to interpret Event data               */
ARG Parm
PARSE VAR Parm QM QU trace .

/* If not on TSO, add INIT REXX function */
env = address()
if env <> 'TSO' then
  rcc = RxFuncAdd('RXMQINIT','RXMQN','RXMQINIT')

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

RXMQTRACE = trace
rcc = RXMQCONN(QM)
 call sayrcc

say
say 'Open EVENT Queue -' QU
say

oo = mqoo_inquire+mqoo_set+MQOO_INPUT_AS_Q_DEF
RXMQTRACE = trace
RCC = RXMQOPEN(QU, OO, 'H2', 'OOD.')
 call sayrcc

do forever
  say
  say 'Read EVENT Queue -' QU
  say

  g.0      = 1024
  g.1      = ''
  igmo.ver= 3
  igmo.opt = MQGMO_WAIT

  RXMQTRACE = trace
  rcc = RXMQGET(h2,'g.','igmd.','ogmd.','igmo.','ogmo.')
   call sayrcc

  if RXMQ.LASTRC <> 0 then leave

  say 'ogmd.pd' ogmd.pd 'ogmo.rqn <'ogmo.rqn'>'

  RXMQTRACE = trace
  rcc = RXMQEVNT('g.','evnt.')
   call sayrcc

  do w=1 to words(evnt.zlist)
    wrd = word(evnt.zlist,w)
    say left('EVNT.'wrd,30) '<'evnt.wrd'>'
  end
end

RXMQTRACE = ''
rcc = RXMQCLOS(h2, mqco_none)
 call sayrcc


say
say 'Remove the Interface functions from the Rexx Workspace ...'
say

call  RXMQDISC
call  RXMQTERM
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
