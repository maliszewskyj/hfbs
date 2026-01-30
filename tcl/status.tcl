#!/usr/bin/tclsh
# $Id: status.tcl,v 1.1.1.1 1999/09/20 14:32:07 nickm Exp $
#
# status - monolithic command for determining core status
#
#
proc status { {verbosity full} } {
    global action chop logs run
    set output ""
    if {$action(plcdown)} {
	append output \
		"\n*****PLC not communicating -- monitoring suspended*****\n\n"
    }
    if {$action(doingseq)} {
	append output "Sequence in progress\n"
    }
    if {$action(doingrun)} {
	if {[string compare scan $run(mode)] == 0} {
	    append output "Scan in progress   : $run(rundir)/$run(runfile)\n"
	    append output "   Scan comments   : $run(comments)\n"
	    append output "   Scan start      : [FormatTime $run(starttime)]\n"
	    append output "   Scan progress   : $run(curcycle)/$run(ncycles)\n"
	} else {
	    append output "Run in progress    : $run(rundir)/$run(runfile)\n"
	    append output "   Run comments    : $run(comments)\n"
	    append output "   Run user        : $run(user)\n"
	    append output "   Run start       : [FormatTime $run(starttime)]\n"
	    append output "   Run cycle start : [FormatTime $run(cycstart)]\n"
	    append output "   Run progress    : $run(curcycle)/$run(ncycles)\n"
	}
    }
    if {$action(monitor,hdw)} {
	append output "Monitoring in progress : $logs(logdir)/$logs(monfile)\n"
    }
    if {[string compare $verbosity full] == 0} {
	append output [FormatLogBlk]
    }
    return $output
}
