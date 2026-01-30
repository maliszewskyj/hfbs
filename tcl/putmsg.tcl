#!/usr/bin/tclsh
#
# putmsg: write message in buffer to units indicated
#
#       where =  1 : write msg to monitor log file
#       where =  2 : write msg to dcscore.log
#       where =  4 : write msg to dcslog.log
#       where =  8 : write msg to hdwerr.log
#       where = 16 : write msg to status file
#       where = 32 : write msg to stdout
#
#       One can OR these values together to write to multiple locations
#
# $Id: putmsg.tcl,v 1.5 2002/06/13 13:06:11 nickm Exp $


proc putmsg { where msg } {
    global logs

    set curtime [FormatTime [clock seconds]]
    #
    # Send message to stdout - could eventually be a broadcast message.
    if [expr $where & $logs(stdout)] {
	catch {puts -nonewline " $curtime : " } 
        catch {puts stdout $msg} 
    }

    #
    #
    if [expr $where & $logs(status)] {
        set fd [open $logs(statdir)/$logs(statfile) w]
        puts $fd $msg
        close $fd
    }

    #
    # Write message to error log file
    if [expr $where &  $logs(errlog)] {
        set fd [open $logs(logdir)/$logs(errfile) a]
        puts -nonewline $fd " $curtime : "
        puts $fd $msg
        close $fd
    }

    #
    # Write message to dcslog.log
    if [expr $where &  $logs(userlog)] {
        set fd [open $logs(logdir)/$logs(userfile) a]
        puts -nonewline $fd " $curtime : "
        puts $fd $msg
        close $fd
    }

    # Write message to dcscore.log
    if [expr $where &  $logs(corelog)] {
        set fd [open $logs(logdir)/$logs(corefile) a]
        puts -nonewline $fd " $curtime : "
        puts $fd $msg
        close $fd
    }

    # Write message to monitor log file
    if [expr $where &  $logs(monlog)] {
        # This is rather more complicated.
	set logs(monfile) "hdwmon_$logs(rxcycle).log"
        set fd [open $logs(logdir)/$logs(monfile) a]
	puts $fd $msg
        close $fd

    }
}

# Catch any background errors
proc bgerror { args } {
    set curtime [FormatTime [clock seconds]]
    puts "$curtime : BACKGROUND $args"
    return -code break
}