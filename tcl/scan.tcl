#!/usr/bin/tclsh
# $Id: scan.tcl,v 1.23 2009/06/04 21:15:28 nickm Exp $
#
# "Scan" mode of collecting data:
#
#        
#

proc scan { req_action args } {
    global action run

    switch $req_action {
	start {
	    return [eval ScanStart $args]
	}
	buffer {
	    if { $action(doingrun) } {
		return -code error "ERR:\nRun in progress"
	    }
	    if {[llength $args] < 1} {
		return -code error "ERR:\nUsage: scan buffer <bufno>"
	    }
	    return [ScanBuffer [lindex $args 0]]	    
	}
	abort {
	    return [ScanAbort]
	}
	default {
	    set    msg "ERR:\nValid options: "
	    append msg "start abort"
	    return -code error $msg
	}
    }
}

proc ScanBuffer { bufno } {
    global action run time logs hdw tc

    if { $action(doingrun) } {
	return -code error "ERR:\nRun already in progress"
    }

    # Read buffer information from hfbsbuf.prm
    if {![file exists /home/ncnr/hfbsscan.prm]} {
	return -code error "ERR:\nCan't find buffer file."
    }
    if [catch {source /home/ncnr/hfbsscan.prm}] {
	return -code error "ERR:\nError occurred reading buffer file."
    }
    
    # Fill in elements of run array with those from hfbsbuf
    if [catch {set scanbuf($bufno,comments)}] {
	return -code error "ERR:\nBuffer $bufno does not appear to exist."
    }

    set run(comments) $scanbuf($bufno,comments)
    set run(user)     $scanbuf($bufno,user)
    set run(ncycles)  $scanbuf($bufno,ncycles)
    set run(cyctime)  $scanbuf($bufno,cyctime)
    set run(varytemp) $scanbuf($bufno,varytemp)
    set run(mode)     scan
    if {$run(varytemp)} {
	set run(readtemp)  1
	set run(holdtime)  $scanbuf($bufno,holdtime)
	set run(tempstart) $scanbuf($bufno,tempstart)
	set run(tempinc)   $scanbuf($bufno,tempinc)
	set run(curtemp)   $run(tempstart)
	set run(curset)    $run(tempstart)
    } else {
	set run(holdtime)      0
	set run(tempstart) 300.0
	set run(tempinc)     0.0
	set run(curtemp)   $run(tempstart)
	set run(curset)    $run(tempstart)
	set run(readtemp) $scanbuf($bufno,readtemp)
    }

    if {$run(readtemp) || $run(varytemp)} {
	# Check to see whether the temperature controller is defined
	if [string match null $tc(controller)] {
	    return -code error "ERR:\nNo controller selected"
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
    
    # Set run mode to "scan" - scan mode is really just standard mode
    set errcount 0
    while [catch {acquire mode scan} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not set run mode: $result"
	incr errcount
	if {$errcount > 4} {
	    putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		"   Could not set run mode. Final attempt"
	    return -code error "ERR:\nCould not set run mode: $result"
	}
    }

    # Create directory for storing data if it does not exist
    set run(rundir)     /usr/local/hfbs/data/$logs(rxcycle)
    if [catch {MakeDirectory $run(rundir)} result] {
	putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		$result
	return -code error "ERR:\n$result"
    }

    set run(runfile)    [NewFileName $run(rundir) hscn]
    set run(curcycle)   0
    set run(wbm)      {}
    set run(tbm)      {}
    set run(tottime)  0.0
    set run(interval) 0.0 

    # Initialize entire shared memory histogram to fill value
    acquire fill
    acquire share total

    # Write entry to log file
    
    set outlun [expr $logs(stdout) + $logs(corelog) + $logs(userlog)]
    set msg "Scan started:"
    putmsg $outlun $msg
    set msg "     Filename = $run(rundir)/$run(runfile)"
    putmsg $outlun $msg
    set msg "     Comments = $run(comments)"
    putmsg $outlun $msg
    set msg "     Npts     = $run(ncycles)  Time/pt = $run(cyctime)"
    putmsg $outlun $msg
    putmsg $logs(status) [status full]

    set run(starttime)   [clock seconds]
    if {$run(varytemp)} {
	ScanTempSet
    } else {
	ScanStartPoint
    }
    return "OK:\nScan started. Filename = $run(rundir)/$run(runfile)"
}

proc ScanStart { args } {
    global action run tc logs time action
    
    if { $action(doingrun) } {
	return -code error "ERR:\nRun already in progress"
    }

    # Process arguments for scan parameters
    set largs [llength $args]
    if {$largs < 3} {
	set result "ERR:\nUsage:scan start <npts> <time/pt> <temp|time>"
	return -code error $result
    }

    set ncycles   [expr 1 * [lindex $args 0]]
    set cycletime [expr 1 * [lindex $args 1]]

    if {[string compare temp [lindex $args 2]] == 0} {
	set run(varytemp) 1
	set run(readtemp) 1
	# Check to see whether the temperature controller is defined
	if [string match null $tc(controller)] {
	    return -code error "ERR:\nNo controller selected"
	}

	if {$largs < 6} {
	    set result "ERR:\n"
	    append result "Usage: scan start <npts> <time/pt> "
	    append result "temp <Tstart> <deltaT> <holdtime> <comments>"
	}
	set run(tempstart) [expr 1.0 * [lindex $args 3]]
	set run(tempinc)   [expr 1.0 * [lindex $args 4]]
	set run(holdtime)  [expr 1   * [lindex $args 5]]
	set run(comments)  [join [lrange $args 6 end] " "]
	set run(curset)    $run(tempstart)
	set run(curtemp)   $run(tempstart)
    } else {
	set run(varytemp)  0
	set run(tempstart) 300.0
	set run(curset)    300.0
	set run(curtemp)   300.0
	set run(tempinc)   0.0
	set run(holdtime)  0
	set run(comments)  [join [lrange $args 3 end] " "] 
	set run(wbm)       {}
	set run(tbm)       {}
	set run(tottime)   0.0
    }
    set run(wbm)       {}
    set run(tbm)       {}
    set run(tottime)   0.0
    set run(interval)  0.0

    # Set run mode to "scan" - scan mode is really just standard mode
    set errcount 0
    while [catch {acquire mode scan} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not set run mode: $result"
	incr errcount
	if {$errcount > 4} {
	    putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		"   Could not set run mode. Final attempt"
	    return -code error "ERR:\nCould not set run mode: $result"
	}
    }

    # Create directory for storing data if it does not exist
    set run(rundir)     /usr/local/hfbs/data/$logs(rxcycle)
    if [catch {MakeDirectory $run(rundir)} result] {
	putmsg [expr $logs(corelog) + $logs(errlog) + $logs(stdout)] \
		$result
	return -code error "ERR:\n$result"
    }

    set run(runfile)    [NewFileName $run(rundir) hscn]
    set run(ncycles)    $ncycles
    set run(curcycle)   0
    set run(cyctime)    $cycletime
    set run(interval)   0.0
    set run(mode)       scan
    set run(starttime)  [clock seconds]
    set run(user)       USER

    # Initialize entire shared memory histogram to fill value
    catch {acquire clear}
    catch {acquire fill}

    # Write entry to log file
    
    set outlun [expr $logs(stdout) + $logs(corelog) + $logs(userlog)]
    set msg "Scan started:"
    putmsg $outlun $msg
    set msg "     Filename = $run(rundir)/$run(runfile)"
    putmsg $outlun $msg
    set msg "     Comments = $run(comments)"
    putmsg $outlun $msg
    set msg "     Npts     = $run(ncycles)  Time/pt = $run(cyctime)"
    putmsg $outlun $msg
    putmsg $logs(status) [status full]

    if {$run(varytemp)} {
	ScanTempSet
    } else {
	ScanStartPoint
    }
    return "OK:\nScan started. Filename = $run(rundir)/$run(runfile)"
}

proc ScanTempSet {{firstpt 0}} {
    global action run logs time

    set setpoint [expr $run(tempstart) + $run(curcycle) * $run(tempinc)]
    set run(curset) $setpoint
    if [catch {temperature set $setpoint} result] {
	# Need to figure out what to do if we can't set the temperature
    }
    set action(doingrun) 1
    # Wait for temperature to equilibrate before measuring next point
    if {$action(doingrun)} {
	putmsg [expr $logs(corelog) + $logs(stdout)] \
		"     Waiting $run(holdtime) sec for temp to settle"
	after cancel $time(run,id)
	set time(run,id) [after [expr 1000 * $run(holdtime)] ScanStartPoint]
    }
    return $action(doingrun)
}

proc ScanStartPoint { } {
    global action run tc time logs

    puts "ScanStartPoint"
    # Start run now
    set errcount 0

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
    
    # Clear remote histogram
    catch {acquire clear}

    set ticks [ expr 1000 * $run(cyctime)]

    if [catch {acquire preset $ticks} result] {
	return -code error "ScanStartPoint error: $result"
    }
    
    while [catch {acquire start} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] \
		"Could not start run: $result"
	incr errcount
	if {$errcount > 4} {
	    return -code error "ERR:\nUnable to start run: $result"
	}
    }
    set run(cycstart)    [clock seconds]
    set action(doingrun) 1

    # Write to status file
    putmsg $logs(status) [status full]
    set time(run,id) [after [expr 1000 * $run(cyctime)] ScanProgressPoint]

    return $action(doingrun)
}

proc ScanProgressPoint { } {
    global action run tc time logs livetime comm

    puts "ScanProgressPoint"
    if [catch {acquire counting} counting] {
	return code -error "ERR:\nUnable to determine if we're counting/scan"
    }
    if {$counting} {
	# We're not done, poll again
	set time(run,id) [after 5000 ScanProgressPoint]
	return $action(doingrun)
    } else {
	return [ScanFinishPoint]
    }
}

proc ScanFinishPoint { } {
    global action run tc time logs livetime comm

    puts "ScanFinishPoint"
    if {!$action(doingrun)} { return }

    catch {acquire counting} counting

    if {$counting} {
	set errcount 0
	while [catch {acquire stop} result] {
	    putmsg $logs(errlog) "Error stopping run: $result"
	    incr errcount
	    if {$errcount > 2} {
		set keeprun 0
		break
	    }
	}
    }

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

    set run(interval) [acquire timer]
    set run(tottime)  [expr $run(tottime) + $run(interval)]

    catch {acquire scalers} scalers
    set run(wbm) [lindex $scalers 1]
    set run(tbm) [lindex $scalers 2]

    # Distill the data down to what we actually want to keep
    # vvvvvvvvvvvvvvvvvvvv
    set work1 [acquire dsum increment]

    if {$run(readtemp)} {
	catch {temperature read} curtemp
	set run(curtemp) $tc(sample)
    }

    if {$run(varytemp)} {
	set ptval $run(curset)
    } else {
	set ptval [expr $run(curcycle) * $run(cyctime)]
    }

    # Stuff the data into shared memory segment
    if {$run(curcycle) < 4096} {
	catch {acquire backfill $run(curcycle) $ptval $work1}
    }

    lappend run(scanvec) $ptval

    catch {acquire share total}

    # Write data to disk
    if [catch {ScanWrite}] { 
	set action(doingrun) 0
	set time(run,id) -1
	putmsg $logs(status) [status full]
	return 
    }
    
    # increment current cycle counter
    incr run(curcycle)

    if {$run(curcycle) >= $run(ncycles)} {	
	set action(doingrun) 0
	set time(run,id) -1
	putmsg $logs(status) [status full]

	# Notify any subscribed clients that there is new data to plot
	foreach c $comm(clients) {
	    if [catch {puts $c RemoteUpdate}] {
		# Unsubscribe nonresponsive clients
		set comm(clients) [ldelete $c $comm(clients)]
	    }
	}

	return 0
    }
    
    # Notify any subscribed clients that there is new data to plot
    foreach c $comm(clients) {
	if [catch {puts $c RemoteUpdate}] {
	    # Unsubscribe nonresponsive clients
	    set comm(clients) [ldelete $c $comm(clients)]
	    catch {close $c} ;# Close RPC channel
	}
    }


    if {$run(varytemp)} {
	return [ScanTempSet]
    } else {
	return [ScanStartPoint]
    }
}

proc ScanWrite { } {
    global action run tc time logs

    LockFileWrite    
    #puts "ScanWrite - Start"
    if {$run(curcycle) == 0} {
	if [catch {open $run(rundir)/$run(runfile) w} outfile] {
	    putmsg [expr $logs(stdout) + $logs(errlog)] \
		    "Could not open data file for writing"
	    catch {close $outfile}
	    return -code error "Could not open data file for writing"
	}

	if {$run(varytemp)} { set scanmode temp } else { set scanmode time }
	set output ""
	append output "\#Comments   : $run(comments)\n"
	append output "\#User       : $run(user)\n"
	append output "\#Scan Mode  : $scanmode\n"
	append output "\#Start Time : [FormatTime $run(starttime)]\n"
	append output "\#Time/pt    : $run(cyctime)\n"
	append output "\#TIME  LIVE   WBM   TBM    SETPT  "
	append output "CTEMP  STEMP  ATEMP  "
	for {set i 1} {$i < 17} {incr i} {
	    append output [format "DETECTOR%02d " $i]
	}
	append output MONITORFC
	puts $outfile $output
    } else {
	if [catch {open $run(rundir)/$run(runfile) a} outfile] {
	    putmsg [expr $logs(stdout) + $logs(errlog)] \
		    "Could not open data file for appending"
	    puts "Could not open data file for appending"
	    return -code error "Could not open data file for writing"
	}
    }
    putmsg $logs(stdout) "Writing scan $run(curcycle)/$run(ncycles) to $outfile"
    # Write raw data to disk - let it be the client program's responsibility to
    # reduce the data

    set work1 [acquire dsum increment]
    set len [llength $work1]
    # Write out increment sum

    set tval [expr $run(curcycle) * $run(cyctime)]
    set output [format "%6.2f " $tval]
    append output [format "%6.2f " $run(interval)]
    append output [format "%6.2f " $run(wbm)]
    append output [format "%6.2f " $run(tbm)]
    append output [format "%6.2f " $run(curset)]
    append output [format "%6.2f " $tc(control)]
    append output [format "%6.2f " $tc(sample)]
    append output [format "%6.2f " $tc(auxiliary)]
    if {$len > 17} {
	for {set i 1} {$i < 17} {incr i} {
	    append output [format "%10.2f " [lindex $work1 $i]]
	}
	append output [format "%10.2f " [lindex $work1 0]]
    }
    puts $outfile $output
    close $outfile

    # Mirror data file to analysis computer
    # TURN OFF BROADCASTS 
    # catch { ScanBroadcast }

    LockFileRemove
    #puts "ScanWrite - End"
    return
}

proc ScanAbort { } {
    global action run tc time logs

    if {!$action(doingrun)} {
	return -code error "ERR:\nNo scan in progress!"
    }

    # Cover ourselves in case we're doing a run instead of a scan
    if {![regexp scan $run(mode)]} {
	return -code error "ERR:\nNo scan in progress!"
    }

    if [catch {acquire stop} result] {
	set action(doingrun) 0
    }
    after cancel $time(run,id)
    set action(doingrun) 0
    putmsg $logs(corelog) "Scan aborted"
    putmsg $logs(status) [status full]

    return "OK:\nScan stopped"
}

proc ScanBroadcast { } {
    global action run logs increment total tc work1

    if {$run(curcycle) == 1} {
	set msg "NG2:START\tNPTS=$run(ncycles)\tFILE=$run(rundir)/$run(runfile)\t"
	if {$run(varytemp)} {
	    append msg "VARY=T\t"
	} else {
	    append msg "VARY=TIME\t"
	}
	append msg "$run(comments)"
	catch {acquire broadcast $msg}
    }

    set msg "NG2:\t"
    append msg "PT=$run(curcycle)\t"
    if {$run(varytemp)} {
	set ptval $run(curset)
	append msg "T=$ptval\t"
    } else {
	set ptval [expr $run(curcycle) * $run(cyctime)]
	append msg "TIME=$ptval\t"
	if {$run(readtemp)} {
	    append msg "T=$run(curtemp)\t"
	}
    }
    for {set i 0} {$i <  5} {incr i} {lappend mask 0}
    for {set i 5} {$i < 17} {incr i} {lappend mask 1}

    #increment collapse {0 1} work1
    set work1 [acquire dsum increment]
    set counts 0
    for {set i 1} {$i < 17} {incr i} {
	set msk [lindex $mask $i]
	set cts [lindex work1 $i]
	if {$msk} {
	    set counts [expr $counts + $cts]
	}
    }

    append msg "DATA=$counts"
    catch {acquire broadcast $msg}
}

proc ScanBroadcastEnd { } {
    set msg "NG2:FINISH";
    catch {acquire broadcast $msg}
}
