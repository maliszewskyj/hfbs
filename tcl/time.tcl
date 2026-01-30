#!/usr/bin/tclsh
# $Id: time.tcl,v 1.5 2000/11/27 13:11:49 nickm Exp $
# Procedures for manipulating time and scheduling
#

# monitor - monolithic command for interfacing with hardware monitoring
#
#
proc monitor { req_action {mtasks all} {checks 1}} {
    global action time logs hdw
    switch $req_action {
	cancel {
	    switch -regexp $mtasks {
		temp {
		    after cancel $time(temp,id)
		    set action(monitor,temp) 0
		    return "OK:\nMonitoring cancelled."

		}
		hdw {
		    after cancel $time(hdw,id)
		    set action(monitor,hdw)  0 
		    after cancel $time(logfile,id); 
		    return "OK:\nMonitoring cancelled."

		}
		all {
		    after cancel $time(temp,id)
		    set action(monitor,temp) 0
		    after cancel $time(hdw,id)
		    set action(monitor,hdw)  0 
		    after cancel $time(logfile,id); 
		    return "OK:\nMonitoring cancelled."
		}
	    }
	}
	start {
	    switch -regexp $mtasks {
		temp {
		    if {$action(monitor,temp)} {
			return -code error "ERR:\nMonitoring already in progress."
		    }
		    global tc
		    if [regexp null $tc(controller)] {
			return -code error "ERR:\nNo controller defined."
		    }
		    MonTemp
		    return "OK:\nMonitoring temperature."
		}
		hdw {
		    if { $action(monitor,hdw) } {
			return -code error "ERR:\nMonitoring already in progress."
		    }
		    MonHdw
		    # set logs(monfile) [NewFileName $logs(logdir) mon]
		    putmsg $logs(monlog) "# [FormatTime [clock seconds]]"
		    putmsg $logs(monlog) "# $logs(logdir)/$logs(monfile)"
		    WriteLogs
		    return "OK:\nMonitoring started."
		}
		all {
		    if {!$action(monitor,temp)} {
			global tc
			if {![regexp null $tc(controller)]} {
			    MonTemp
			}
		    }
		    if {!$action(monitor,hdw)} {
			MonHdw
			# set logs(monfile) [NewFileName $logs(logdir) mon]
			putmsg $logs(monlog) "# [FormatTime [clock seconds]]"
			putmsg $logs(monlog) "# $logs(logdir)/$logs(monfile)"
			WriteLogs
			return "OK:\nMonitoring started."	
		    }
		    return -code error "ERR:\nMonitoring not started"
		}
	    }
	}
	default {
	    return -code error "ERR:\nOptions: start cancel"
	}
    }
}


#
# TimeYet - compare a reference time against the current time
#           returns: 1 if the reference time has passed or has been reached
#                    0 if the reference time is still in the future
#
proc TimeYet { reference_time } {
  set current_time [clock seconds]
  if {current_time >= reference_time} {
    return 1;
  }
  return 0;
}

#
# ScheduleAction - return next time
#
proc ScheduleAction { LastTime UpdateIncrement } {
  return [expr $LastTime + $UpdateIncrement]
}

# Stale - returns 0 if LastTime occurred within the last 30 sec, 1 otherwise
#
proc Stale { LastTime } {
    global time
    if { [expr $LastTime - $time(corestart)] < 30 } { return 1}
    if { [expr [clock seconds] - $LastTime] >= 30 } {
	return 1
    } 
    return 0
}

#
# GetElapsedTime - return elapsed time in days, hours, minutes, and seconds
#
proc GetElapsedTime { StartTime Elapsed } {
  upvar $Elapsed et

  set elapsedtime [expr [clock seconds] - $StartTime]
  set et(ss) [expr $elapsedtime % 60]
  set elapsedtime [expr [expr $elapsedtime - $et(ss)] / 60]
  set et(mm) [expr $elapsedtime % 60]
  set elapsedtime [expr [expr $elapsedtime - $et(mm)] / 60]
  set et(hh) [expr $elapsedtime % 24]
  set elapsedtime [expr [expr $elapsedtime - $et(hh)] / 24]
  set et(dd) [expr $elapsedtime % 7 ]
  set elapsedtime [expr [expr $elapsedtime - $et(dd)] / 7 ]
  set et(ww) $elapsedtime
}

#
# FormatTime - format time in the form "dd-mmm-yyyy hh:mm:ss"
#
proc FormatTime { Timeval } {
    return [clock format $Timeval -format "%d-%b-%Y %H:%M:%S"]
}

#
# IsoTime - format time in ISO-8601 style: "YYYY-MM-DD hh:mm:ss"
#
proc IsoTime { Timeval } {
    return [clock format $Timeval -format "%Y-%m-%d %H:%M:%S"]
}

#
# MonHdw - 
#
proc MonHdw { } {
    global fd hdw time logs action
    set time(hdw,last) [clock seconds]
    # Read hardware values

    set action(monitor,hdw) 1
    set errflag 0

    #
    # Really should have some error checking code hereabouts.
    catch {ReadHdw}

    set time(hdw,id) [after [expr $time(hdw,incr) * 1000] MonHdw]
}

#
# MonTemp -
#
proc MonTemp { } {
    global tc time logs action

    if [regexp null $tc(controller)] {
	putmsg $logs(corelog) "No controller defined. Monitoring stopped"
	set action(monitor,temp) 0
	return
    }


    set action(monitor,temp) 1
    set errflag 0

    
    #
    #
    catch {temperature read}
    set time(temp,id) [after [expr $time(temp,incr) * 1000] MonTemp]
}


#
# WriteLogs - 
#
proc WriteLogs { } {
    global time logs hdw action
    
    set time(logfile,last) [clock seconds]
    if {!$action(plcdown)} {
	# Do not write to logfile unless we have valid data
	putmsg $logs(monlog) [FormatLogBlk]
    }
    putmsg $logs(status) [status full]
    set time(logfile,id) [after [expr $time(logfile,incr) * 1000] WriteLogs]
}


