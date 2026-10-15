#!/usr/bin/tclsh
# $Id: run.tcl,v 1.15 2009/06/04 21:15:51 nickm Exp $
#
# Modify this to reflect the "cycle-oriented" mode of collecting data.
#

proc run { req_action args } {
    global action run
    switch $req_action {
	start  {
	    if { $action(doingrun) } {
		return -code error "ERR:\nRun in progress"
	    }
	    if {[llength $args] < 3} {
		return -code error \
			"ERR:\nUsage: run start <ncycles> <cycletime> <mode> <comments>"
	    }
	    return [RunStart [lindex $args 0] [lindex $args 1] \
		    [lindex $args 2] [lrange $args 3 end]]
	}
	buffer {
	    if { $action(doingrun) } {
		return -code error "ERR:\nRun in progress"
	    }
	    if {[llength $args] < 1} {
		return -code error "ERR:\nUsage: run buffer <bufno>"
	    }
	    return [RunBuffer [lindex $args 0]]
	}
	pause {
	    return [RunPause]
	}
	resume {
	    return [RunResume]
	}
	abort {
	    return [RunAbort]
	}
	finish {
	    return [RunFinish]
	}
	truncate {
	    return [RunTruncate]
	}
	update {
	    return [RunUpdate]
	}
	status {
	    return $action(doingrun)
	}
	mode {
	    return $run(mode)
	}
	info {
	    set run(inprogress) $action(doingrun)
	    return [array get run]
	}
	default {
	    set    msg "ERR:\nValid options: start pause resume abort finish truncate status info"
	    return -code error $msg
	}
    }
}

# Check on running status of HFBS data acquisition program
proc HFBSCheck { } {
    set output [exec ssh ncnr\@hfbsdaq.ncnr.nist.gov bin/hfbstool]
    if {[regexp {running} $output]} {
	return 1
    } else {
	return 0
    }
    return 0 ;# Default is _not_ running
}

proc HFBSRestart { } {
    # Note that unless we explicitly redirect stdout, stderr, and stdin this command can hang
    set output [exec ssh ncnr\@hfbsdaq.ncnr.nist.gov "nohup bin/hfbstool -s > hfbsdaq.out 2>&1 < /dev/null"]
    return 1
}

proc RunStart { ncycles cycletime {mode velocity} {comments ""}} {
    global action run time logs

    if { $action(doingrun) } {
	return -code error "ERR:\nRun already in progress"
    }

    if [catch {expr 1 * $ncycles} ncycles] {
	return -code error "ERR:\nSpecify integer number of cycles"
    }

    if [catch {expr 1 * $cycletime} cycletime] {
	return -code error "ERR:\nSpecify cycle time in seconds"
    }

    switch $mode {
	pst      { set run(timebin) 30 }
	time     { set run(timebin) 30 }
	velocity { set run(timebin) 30 }
	default {
	    return -code error \
		    "ERR:\nAvailable run modes: pst time velocity"
	}
    }
    
    # Check for running hfbsdaq server
    if {![HFBSCheck]} {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
	    "hfbsdaq does not appear to be running. Restarting"
	HFBSRestart
	sleep 5
	if {![HFBSCheck]} {
	    putmsg [expr $logs(corelog) + $logs(errlog)] \
		"hfbsdaq could not be restarted."
	    return -code error "ERR:\nCould not start hfbsdaq"
	}
    }


    # Set frequency of heartbeat for time binning mode
    #catch {acquire timebin $run(timebin)} result

    # Set run mode
    set errcount 0
    while [catch {acquire mode $mode} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not set run mode: $result"
	incr errcount
	if {$errcount > 4} {
	    putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		"   Could not set run mode. Final attempt"
	    return -code error "ERR:\nCould not set run mode: $result"
	}
    }

    # Initialize the cumulative histogram to zero
    catch {acquire clear}

    # Resize total histogram if needed
    catch {acquire share total} result

    # Create directory for storing data if it does not exist
    set run(rundir)     /usr/local/hfbs/data/$logs(rxcycle)
    if [catch {MakeDirectory $run(rundir)} result] {
	putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		$result
	return -code error "ERR:\n$result"
    }

    set run(user)       USER
    set run(runfile)    [NewFileName $run(rundir) hfbs]
    set run(ncycles)    $ncycles
    set run(curcycle)   0
    set run(cyctime)    $cycletime
    set run(tottime)    0.0
    set run(comments)   $comments
    set run(mode)       $mode
    set run(dopfreq)    {}
    set run(wbm)        {}
    set run(tbm)        {}
    set run(sampletemp) {}
    set run(controltemp) {}
    set run(readtemp)   0
    set run(checktemp)  0
    set run(checkdop)   0

    set ticks [ expr 1000 * $run(cyctime)]

    if [catch {acquire preset $ticks} result] {
	return -code error "RunStart error: $result"
    }

    
    # Start run now
    set errcount
    while [catch {acquire start} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not start run: $result"
	incr errcount
	if {$errcount > 4} {
	    return -code error "ERR:\nUnable to start run: $result"
	}
    }
    set run(starttime)   [clock seconds]
    set run(cycstart)    $run(starttime)
    set action(doingrun) 1

    set outlun [expr $logs(stdout) + $logs(corelog) + $logs(userlog)]

    set msg "Run started:"
    putmsg $outlun $msg
    set msg "     Filename = $run(rundir)/$run(runfile)"
    putmsg $outlun $msg
    set msg "     Comments = $run(comments)"
    putmsg $outlun $msg
    set msg "     Mode     = $run(mode)"
    putmsg $outlun $msg
    set msg "     Cycles   = $run(ncycles)  Cycletime = $run(cyctime)"
    putmsg $outlun $msg
    putmsg $logs(status) [status full]
    set time(run,id) [after [expr 1000 * $run(cyctime)] RunProgress]
    return "OK:\nRun started. Filename = $run(rundir)/$run(runfile)"
}

proc RunBuffer { bufno } {
    global action run time logs hdw   

    # Read buffer information from hfbsbuf.prm
    if {![file exists /home/ncnr/hfbsbuf.prm]} {
	return -code error "ERR:\nCan't find buffer file."
    }
    if [catch {source /home/ncnr/hfbsbuf.prm}] {
	return -code error "ERR:\nError occurred reading buffer file."
    }
    
    # Fill in elements of run array with those from hfbsbuf
    if [catch {set hfbsbuf($bufno,comments)}] {
	return -code error "ERR:\nBuffer $bufno does not appear to exist."
    }

    set comments $hfbsbuf($bufno,comments)
    set user     $hfbsbuf($bufno,user) 
    set ncycles  $hfbsbuf($bufno,ncycles) 
    set cyctime  $hfbsbuf($bufno,cyctime) 
    set mode     $hfbsbuf($bufno,mode)

    switch $mode {
	pst      { set run(timebin) 30 }
	time     { set run(timebin) 30 }
	velocity { set run(timebin) 30 }
	default {
	    return -code error \
		    "ERR:\nAvailable run modes: pst time velocity"
	}
    }

    # Set frequency of heartbeat for time binning mode
    #catch {acquire timebin $run(timebin)} result
    # Check for running hfbsdaq server
    if {![HFBSCheck]} {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
	    "hfbsdaq does not appear to be running. Restarting"
	HFBSRestart
	sleep 5
	if {![HFBSCheck]} {
	    putmsg [expr $logs(corelog) + $logs(errlog)] \
		"hfbsdaq could not be restarted."
	    return -code error "ERR:\nCould not start hfbsdaq"
	}
    }


    # Set run mode
    set errcount 0
    while [catch {acquire mode $mode} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not set run mode: $result"
	incr errcount
	if {$errcount > 4} {
	    putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		"   Could not set run mode. Final attempt"
	    return -code error "ERR:\nCould not set run mode: $result"
	}
    }


    # Initialize the cumulative histogram to zero
    catch {acquire clear total}

    # Resize total histogram if needed
    catch {acquire share total} result

    # Create directory for storing data if it does not exist
    set run(rundir)     /usr/local/hfbs/data/$logs(rxcycle)
    if [catch {MakeDirectory $run(rundir)} result] {
	putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		$result
	return -code error "ERR:\n$result"
    }

    set run(runfile)    [NewFileName $run(rundir) hfbs]
    set run(ncycles)    $ncycles
    set run(curcycle)   0
    set run(cyctime)    $cyctime
    set run(tottime)    0.0
    set run(comments)   $comments
    set run(mode)       $mode
    set run(user)       $user
    set run(dopfreq)    {}
    set run(wbm)        {}
    set run(tbm)        {}
    set run(sampletemp) {}
    set run(controltemp) {}
    set run(readtemp)   $hfbsbuf($bufno,readtemp)
    set run(checktemp)  $hfbsbuf($bufno,checktemp)
    set run(checkdop)   $hfbsbuf($bufno,checkdop)

    set ticks [ expr 1000 * $run(cyctime)]

    if [catch {acquire preset $ticks} result] {
	return -code error "RunStart error: $result"
    }
    
    # Start run now
    set errcount
    while [catch {acquire start} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not start run: $result"
	incr errcount
	if {$errcount > 4} {
	    return -code error "ERR:\nUnable to start run: $result"
	}
    }
    set run(starttime)   [clock seconds]
    set run(cycstart)    $run(starttime)
    set action(doingrun) 1

    set outlun [expr $logs(stdout) + $logs(corelog) + $logs(userlog)]

    set msg "Run started:"
    putmsg $outlun $msg
    set msg "     Filename = $run(rundir)/$run(runfile)"
    putmsg $outlun $msg
    set msg "     Comments = $run(comments)"
    putmsg $outlun $msg
    set msg "     User     = $run(user)"
    putmsg $outlun $msg
    set msg "     Mode     = $run(mode)"
    putmsg $outlun $msg
    set msg "     Cycles   = $run(ncycles)  Cycletime = $run(cyctime)"
    putmsg $outlun $msg
    putmsg $logs(status) [status full]
    set time(run,id) [after [expr 1000 * $run(cyctime)] RunProgressCycle]
    return "OK:\nRun started. Filename = $run(rundir)/$run(runfile)"
}

proc RunPause { } {
    if [catch {acquire stop} result] {
	return -code error "ERR:\n$result"
    }
    return "OK:\nRun paused"
}

proc RunAbort { } {
    global action run time logs

    if {$action(doingseq)} {
	return -code error "ERR:\nSequence in progress"
    }

    if {!$action(doingrun)} {
	return -code error "ERR:\nNo run in progress"
    }

    # Cover ourselves in case we're doing a scan instead of a run
    if [regexp scan $run(mode)] {
	return [ScanAbort]
    }

    if [catch {acquire stop} result] {
	return -code error "ERR:\n$result"
    }
    after cancel $time(run,id)
    set action(doingrun) 0
    putmsg $logs(corelog) "Run aborted"
    putmsg $logs(status) [status full]    
    return "OK:\nRun stopped"
}

##
################ Work on update code
##
proc RunUpdate { } {
    global action run time logs

    if {!$action(doingrun)} {
	return -code error "ERR:\nNo run in progress"
    }

    if [regexp scan $run(mode)] {
	return -code error "ERR:\nCan't force update on scans"
    }

    putmsg $logs(corelog) "Run update command issued"
    after cancel $time(run,id)
    RunProgress
    
    return "OK:\nRun update performed"
}


proc RunFinish { } {
    global action run time logs

    if {!$action(doingrun)} {
	return -code error "ERR:\nNo run in progress"
    }

    # Cover ourselves in case we're doing a scan instead of a run
    if [regexp scan $run(mode)] {
	return [ScanAbort]
    }

    # Adjust total number of cycles so that the current cycle will be the last
    set run(ncycles) [expr $run(curcycle) + 1]

    putmsg $logs(corelog) "Number of cycles in run altered"
    
    return "OK:\nRun will terminate with current cycle"
}

proc RunTruncate { } {
    global action run time logs livetime increment total tc

    if {!$action(doingrun)} {
	return -code error "ERR:\nNo run in progress"
    }

    # Cover ourselves in case we're doing a scan instead of a run
    if [regexp scan $run(mode)] {
	return [ScanAbort]
    }

    # Cancel any pending run actions
    after cancel $time(run,id) 
    set action(doingrun) 0
    if [catch {acquire stop} result] {
	putmsg $logs(errlog) "Error stopping run: $result"
    }

    if [catch {acquire update increment} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Error encountered in reading histogram: $result"
    }

    set livetime [acquire timer]
    set run(tottime) [expr $run(tottime) + $livetime]


    # It is at this point that we would decide whether to keep the
    # current cycle's data based on temperature and flux threshholds
    if [catch {acquire add total increment} result] {
	putmsg $logs(errlog) "Error adding histograms: $result"
    }
    if {$run(readtemp)} {
	catch {temperature read}
	set run(curtemp) $tc(sample)
    }

    set run(tottime) [expr $run(tottime) + $livetime]

    # Make sure that the total histogram has the right dimensions
    catch {acquire share total} result 

    catch {acquire scalers} scalers
    lappend run(wbm) [lindex $scalers 1]
    lappend run(tbm) [lindex $scalers 2]
    set dcts [lindex $scalers 3]
    set dfrq [expr $dcts / $livetime]
    lappend run(dopfreq) $dfrq
    if {$run(readtemp)} {
	lappend run(sampletemp) $run(curtemp)
    }

    # Here we write the data to disk
    if [catch {RunWrite} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not write data to disk"
    }
    putmsg $logs(status) [status full]
    return "OK:\nRun finished"
    
}

proc RunProgressCycle { } {
    global action run tc time logs livetime comm

    puts "RunProgressCycle"
    if [catch {acquire counting} counting] {
	return code -error "ERR:\nUnable to determine if we're counting/scan"
    }
    if {$counting} {
	# We're not done, poll again
	set time(run,id) [after 5000 RunProgressCycle]
	return $action(doingrun)
    } else {
	return [RunProgress]
    }
}

proc RunProgress { } {
    global hdw time run action logs comm tc livetime increment total

    puts "RunProgress"
    if {!$action(doingrun)} { return }

    catch {acquire counting} counting
    if ($counting) {
	set errcount 0
	while [catch {acquire stop} result] {
	    putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Error stopping run: $result"
	    incr errcount
	    if {$errcount > 2} {
		set keeprun 0
		break
	    }
	}
    }

    set keeprun 1
    set msg "     Run: retrieving data for cycle $run(curcycle)"
    putmsg [expr $logs(stdout) + $logs(corelog)] $msg

    # increment current cycle counter
    incr run(curcycle)

    set errcount 0
    while [catch {acquire update increment} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Error encountered in reading histogram: $result"
	incr errcount
	if {$errcount > 2} {
	    set keeprun 0
	    break
	}
    }

    # It is at this point that we would decide whether to keep the
    # current cycle's data based on temperature and flux threshholds

    if {$run(readtemp)} {
	catch {temperature read}
	set run(curtemp) $tc(sample)
    }

    if {$keeprun} {

	set livetime [acquire timer]
        set run(tottime) [expr $run(tottime) + $livetime]

	# Note that dimension scales are imbedded in total histogram with this step
        if [catch {acquire add total increment} result] {
	    putmsg $logs(errlog) "Error adding histograms: $result"
        }

	catch {acquire scalers} scalers
	lappend run(wbm) [lindex $scalers 1]
	lappend run(tbm) [lindex $scalers 2]
	set dcts [lindex $scalers 3]
	set dfrq [expr $dcts / $livetime]
	lappend run(dopfreq) $dfrq

	if {$run(readtemp)} {
	    switch $tc(sensor) {
		sample  { lappend run(sampletemp) $tc(sample)  }
		control { lappend run(controltemp) $tc(control) }
		both    { 
		    lappend run(sampletemp) $tc(sample)
		    lappend run(controltemp) $tc(sample)
		}
	    }
	}

        # Update copy in shared memory segment
        catch {acquire share total} result

        # Here we write the data to disk
        if [catch {RunWrite} result] {
	    putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not write data to disk: $result"
        }
    }

    if {$run(curcycle) >= $run(ncycles)} {	
	set action(doingrun) 0
	putmsg $logs(status) [status full]
	set time(run,id) -1

	# Notify any subscribed clients that there is new data to plot
	foreach c $comm(clients) {
	    if [catch {puts $c RemoteUpdate}] {
		# Unsubscribe nonresponsive clients
		set comm(clients) [ldelete $c $comm(clients)]
	    }
	}

	return 0
    } else {
	set action(doingrun) 1
	set errcount 0
	while [catch {acquire start} result] {
	    putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		    "Error resuming run: $result"
	    incr errcount
	    if {$errcount > 3} {
	        putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		    "Error resuming run: $result. Quitting now."
		return 0
	    }
	}
	set run(cycstart) [clock seconds]
    }

    # Notify any subscribed clients that there is new data to plot
    foreach c $comm(clients) {
	if [catch {puts $c RemoteUpdate}] {
	    # Unsubscribe nonresponsive clients
	    set comm(clients) [ldelete $c $comm(clients)]
	}
    }

    putmsg $logs(status) [status full]
    if {$action(doingrun)} {
	after cancel $time(run,id)
	set time(run,id) [after [expr 1000 * $run(cyctime)] RunProgress]
    }
    return $action(doingrun) 
}

proc RunWrite { } {
    global action run logs increment total tc

    puts "RUNWRITE"
    LockFileWrite
    set output ""
    append output "\#Comments          : $run(comments)\n"
    append output "\#User              : $run(user)\n"
    append output "\#Run Mode          : $run(mode)\n"
    append output "\#Start Time        : [FormatTime $run(starttime)]\n"
    append output "\#Stop  Time        : [FormatTime [clock seconds]]\n"
    append output "\#Num Cycles        : $run(curcycle)\n"
    append output "\#Cycle Time        : $run(cyctime)\n"
    append output "\#Live Time         : $run(tottime)\n"
    append output "\#Time bin (usec)   : $run(timebin)\n"
    append output "\#Log Doppler Freq  :\n   "
    append output [join $run(dopfreq) "\n   "]
    append output "\n\#Log White beam    :\n   "
    append output [join $run(wbm) "\n   "]
    append output "\n\#Log Trans beam    :\n   "
    append output [join $run(tbm) "\n   "]
    if {$run(readtemp)} {
	switch $tc(sensor) {
	    sample {
		append output "\n\#Log Sample temp   :\n   "
		append output [join $run(sampletemp) "\n   "]
	    }
	    control {
		append output "\n\#Log Control temp  :\n   "
		append output [join $run(controltemp) "\n   "]
	    }
	    both {
		append output "\n\#Log Control temp  :\n   "
		append output [join $run(controltemp) "\n   "]
		append output "\n\#Log Sample temp   :\n   "
		append output [join $run(sampletemp) "\n   "]
	    }
	}
    }
    if [catch {acquire write total $run(rundir)/$run(runfile) $output} result] {
	putmsg [expr $logs(stdout) + $logs(errlog)] "Could not open data file for writing"
	set action(doingrun) 0
	LockFileRemove
	return -code error "Could not open data file for writing"
    }
    # Mirror data file to analysis computer
    catch {RunBroadcast}
    LockFileRemove
    puts "...RunWrite end"
    return
}

proc RunBroadcast { } {
    global action run logs increment total tc pv xvec hvec

    puts "BROADCAST"
    if {$run(curcycle) == 1} {
	set msg "NG2:START\tNPTS=$run(ncycles)\tFILE=$run(rundir)/$run(runfile)\t$run(comments)"
	catch {acquire broadcast $msg}
    }

    set msg "NG2:\tPT=$run(curcycle)\t"
    if {$run(readtemp)} {
	set sampletemp [lindex $run(sampletemp) end]
	append msg "T=$sampletemp\t"
    }

    for {set i 0} {$i <  5} {incr i} {lappend mask 0}
    for {set i 5} {$i < 17} {incr i} {lappend mask 1}
    
    # Synthesize new data or eliminate broadcasting the data
    # vvvvvvvvvvvvvvvvvvvvv
#    mtx split $run(mode) total xpdata pv xvec
#    xpdata collapse {1 0} $mask hvec

    # Only grab a subset of the slice
    # Note that we need to keep the size of the broadcast below
    # 1497 bytes (under the Ethernet MTU) to avoid fragmentation
#    if [info exists run(xprange)] {
#	set range $run(xprange)
#    } else {
#	set range 200
#    }

#    if [info exists run(xpbin)] {
#	set binsize $run(xpbin)
#    } else {
#	set binsize 5
#    }

#    set totrange [expr $range * $binsize]

    # center around elastic peak?
#    set offset [expr 2048 - int($totrange/2)]

#    if [catch {mtx rebin hvec $offset $range $binsize} counts] {
#	puts "Problem printing counts: $counts"
#	return;
#    }

#    ^^^^^^^^^^^^^^^^^^^^^^^^
#   set counts $run(curcycle)
#   append msg "PSD=$counts"
    puts "Broadcasting $msg"
    catch {acquire broadcast $msg}
}

proc RunBroadcastEnd { } {
    set msg "NG2:FINISH";
    catch {acquire broadcast $msg}
}

# Write a lock file to coordinate contention over data file
proc LockFileWrite { } {
    global run
    catch {exec touch $run(lockfile)}
}

proc LockFileRemove { } {
    global run
    catch {exec rm -f $run(lockfile)}
}
