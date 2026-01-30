#!/usr/bin/tclsh

proc sequence { req_action } {
    global action
    switch $req_action {
	start {
	    return [SequenceStart]
	}
	stop {
	    return [SequenceStop]
	}
	status {
	    return $action(doingseq)
	}
	default {
	    return -code error "ERR:\nValid methods: start stop"
	}
    }
}

# SequencePop - this procedure reads the command file and "pops" the top
#               command off the stack, rewriting the file with the remaining
#               commands, terminating the program if there are none.
#
proc SequencePop { infile } {
    global log
    if {![file exists $infile]} {
	return -code error "ERR:\nSequence file $infile does not exist."
    }
    set f [open $infile r]
    if {[gets $f cmdline] < 0} {
	close $f
	exec rm $infile
	return -code error "ERR:\nCould not get command"
    }

    if [catch {read $f} remainder] {
	close $f
	exec rm $infile
	return -code error "ERR:\nCan't read remainder of file"
    }

    close $f
    set f [open $infile w]
    foreach line [split $remainder \n] {
	if {[string length $line] > 0} {
	    puts $f $line
	}
    }
    close $f
    return $cmdline
}

proc SequenceLogWrite { msg } {
    global env logs
    set logfile "$env(HOME)/sequence.log"
    set log [open $logfile a]
    puts $log "[FormatTime [clock seconds]] - $msg"
    close $log
    # Write message to core log as well
    putmsg $logs(corelog) "(SEQUENCE) $msg"
}

proc SequenceLogStart { } {
    global env
    set logfile "$env(HOME)/sequence.log"
    set log [open $logfile w]
    set msg "Sequence Started"
    puts $log "[FormatTime [clock seconds]] - $msg"
    close $log
}

proc SequenceRun { args } {
    global action time
    if {[llength $args] < 2} {
	set output "ERR: Syntax => run buffer <bufno>"
	puts stderr $output
	return [SequenceEnd $output]
    }
    set subcmd [lindex $args 0]
    switch $subcmd {
	buffer {
	    set bufno [lindex $args 1]
	    if {($bufno < 0) || ($bufno > 11)} {
		set output "ERR: Buffer number out of range!"
		puts stderr $output
		return [SequenceEnd $output]
	    } else {
		catch {run buffer $bufno} result
	    }
	}
	default {
	    set output "ERR: Invalid option"
	    puts stderr $output
	    return [SequenceEnd $output]
	}
    }    
    set time(seq,id) [after 1000 SequenceRunProgress]
}

proc SequenceRunProgress { } {
    global action time
    if {$action(doingrun)} {
	set time(seq,id) [after 60000 SequenceRunProgress]
    } else {
	set time(seq,id) [after 1000 SequenceProgress]
    }
}

proc SequenceScan { args } {
    global action time
    if {[llength $args] < 2} {
	set output "ERR: Syntax => scan buffer <bufno>"
	puts stderr $output
	return [SequenceEnd $output]
    }
    set subcmd [lindex $args 0]
    switch $subcmd {
	buffer {
	    set bufno [lindex $args 1]
	    if {($bufno < 0) || ($bufno > 11)} {
		set output "ERR: Buffer number out of range!"
		puts stderr $output
		return [SequenceEnd $output]
	    } else {
		catch {scan buffer $bufno} result
	    }
	}
	default {
	    set output "ERR: Invalid option"
	    puts stderr $output
	    return [SequenceEnd $output]
	}
    }    
    set time(seq,id) [after 1000 SequenceRunProgress]
}

proc SequenceDoppler { args } {
    global action time
    if {[llength $args] < 2} {
	set output "ERR: Syntax => doppler speed <value>"
	puts stderr $output
	return [SequenceEnd $output]
    }
    set subcmd [lindex $args 0]
    switch $subcmd {
	speed {
	    if [catch {expr [lindex $args 1] * 1.0}] {
		set output "ERR: Specify real number for Doppler speed!"
		puts stderr $output
		return [SequenceEnd $output]
	    } else {
		if [catch {doppler speed [lindex $args 1]} result] {
		    set output "ERR: $result"
		    puts stderr $output
		    return [SequenceEnd $output]
		}
	    }
	}
	default {
	    set output "ERR: Invalid option"
	    puts stderr $output
	    return [SequenceEnd $output]
	}
    }    
    set time(seq,id) [after 1000 SequenceProgress]
}

proc SequenceTemp { args } {
    global time action
    if {[llength $args] < 2} {
	set output "ERR: Syntax => temperature set <value>"
	puts stderr $output
	return [SequenceEnd $output]
    }
    set subcmd [lindex $args 0]
    switch $subcmd {
	set {
	    if [catch {expr [lindex $args 1] * 1.0}] {
		set output "ERR: Specify real number for temperature"
		puts stderr $output
		return [SequenceEnd $output]
	    } else {
		if [catch {temperature set [lindex $args 1]} result] {
		    set output "ERR: $result"
		    puts stderr $output
		    return [SequenceEnd $output]
		}
	    }
	}
	ramp {
	    if [catch {expr [lindex $args 1] * 1.0}] {
		set output "ERR: Specify real number for ramp"
		puts stderr $output
		return [SequenceEnd $output]
	    } else {
		if [catch {temperature ramp [lindex $args 1]} result] {
		    set output "ERR: $result"
		    puts stderr $output
		    return [SequenceEnd $output]
		}
	    }
	}
	default {
	    set output "ERR: invalid option"
	    puts stderr $output
	    return [SequenceEnd $output]
	}
    }
    set time(seq,id) [after 1000 SequenceProgress]
}



proc SequenceWait { args } {
    global time action
    if {[llength $args] < 1} {
	set output "Syntax: wait <run/time> ?<params>?: $cmdline"
	puts stderr $output
	return [SequenceEnd $output]
    }
    set subcmd [lindex $args 0]
    switch $subcmd {
	seconds {
	    if [catch {expr [lindex $args 1] * 1} waittime] {
		set waittime 1
	    }
	}
	minutes {
	    if [catch {expr [lindex $args 1] * 60} waittime] {
		set waittime 1
	    } 
	}
	default {
	    set output "ERR: invalid option"
	    puts stderr $output
	    return [SequenceEnd $output]
	}
    }
    set time(seq,id) [after [expr int($waittime * 1000)] SequenceProgress]

}

#
# clean up old sequence log file
#
proc SequenceStart { } {
    global action time
    if {$action(doingseq)} {
	return -code error "ERR:\nSequence already in progress."
    }
    catch {SequenceLogStart}
    set action(doingseq) 1
    set time(seq,id) [after 1000 SequenceProgress]
    return "OK:\nSequence started."
}

proc SequenceStop { } {
    global action time

    if {!$action(doingseq)} {
	return -code error "ERR:\nNo sequence in progress"
    }

    after cancel $time(seq,id)
    set action(doingseq) 0

    # If a run is in progress, stop it
    if {$action(doingrun)} {
	catch {run abort} result
    }

    return "OK:\nSequence stopped."
}

# End sequence prematurely
proc SequenceEnd { args } {
    global action time

    after cancel $time(seq,id)
    set time(seq,id) -1
    set action(doingseq) 0
    if {[llength $args] > 0} {
	SequenceLogWrite $args
    }
}

proc SequenceProgress { } {
    global env action time
    set seq_file "$env(HOME)/hfbs.seq"
    if [catch {SequencePop $seq_file} cmdline] {
	set action(doingseq) 0
	set time(seq,id)    -1
	return
    }
    SequenceLogWrite $cmdline
    set action(seqcmd) $cmdline

    set cmdparts [split $cmdline]
    set cmd      [lindex $cmdparts 0]
    set cmdargs  [lrange $cmdparts 1 end]    
    switch $cmd {
	run {
	    return [eval SequenceRun $cmdargs]
	}
	scan {
	    return [eval SequenceScan $cmdargs]
	}
	doppler {
	    return [eval SequenceDoppler $cmdargs]
	}
	temperature {
	    return [eval SequenceTemp $cmdargs]
	}
	wait {
	    return [eval SequenceWait $cmdargs]
	}
	default {
	    SequenceLogWrite "Unsupported command: $cmdline"
	    set time(seq,id) [after 1000 SequenceProgress]
	}
    }    
}



