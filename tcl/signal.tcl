#!/usr/bin/tclsh
# 
# $Id: signal.tcl,v 1.3 1999/10/05 13:17:46 nickm Exp $
trap {
    putmsg [expr $logs(stdout) + $logs(corelog)] \
	    "Received signal: SIG[trap -name]"
    #exec wall "Core program received nonFATAL signal SIG[trap -name]"
} { INT PIPE TSTP } 

trap {
    putmsg [expr $logs(stdout) + $logs(corelog)] \
	    "Received termination signal. Exiting immediately..."
    exec wall "Core program received termination signal SIGTERM"
    exit
} { TERM }