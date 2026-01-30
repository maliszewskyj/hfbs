#!/usr/bin/tclsh
# $Id: main.tcl,v 1.8 2008/05/08 13:10:43 nickm Exp $

set shell     expect
set libdir    "/usr/local/lib/"
set scriptdir "/usr/local/hfbs/tcl/"
set configdir "/home/ncnr/configure/"
# Port for user control
set comm(rpc_port) 51330 
set comm(xpeek_port) 8080

set libs {libserialaux.so libacquire.so libmodbus.so}
foreach lib $libs {    
    if [catch {load $lib} result] {
        puts stderr "Failed to load $lib: $result"
        exit
    }
}

#
# Load scripts containing variable declarations and procedures
#
source ${scriptdir}initvar.tcl
source ${scriptdir}time.tcl
source ${scriptdir}putmsg.tcl
source ${scriptdir}fileutil.tcl
source ${scriptdir}logfile.tcl
source ${scriptdir}instrcfg.tcl
source ${scriptdir}hdw.tcl
source ${scriptdir}status.tcl
source ${scriptdir}run.tcl
source ${scriptdir}temperature.tcl
source ${scriptdir}scan.tcl
source ${scriptdir}sequence.tcl
source ${scriptdir}subscribe.tcl
source ${scriptdir}rpc.tcl

# Load (local) instrument configuration
if [file exists ${configdir}instrcfg.tcl] {
    source ${configdir}instrcfg.tcl
} 

source ${scriptdir}instrini.tcl

#<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
# Notify Backup system
proc MakeBackup { } {
    global comm
    return
    set s [udp_open]
    puts $s "NG2:FINISH"
    flush $s
    close $s
}

# RPC stuff
proc ClientLogin { file addr } {
    # First check for localhost
    if {[string match $addr 127.0.0.1] == 1} { return }
    # Now restrict to our subnet
    if {[regexp {129\.6\.} $addr match] != 1} {
	error "Client not authorized"
    }    
}

proc CheckCliCmd { input } {
    set args [split $input]
    set cmd  [lindex $args 0]
#    switch $cmd {
#	SubScribe return;
#	puts      return;
#	set       return;
#	array     return;
#	info      return;
#    }
#    return -code break
     # For now, allow all commands
     return
}

#if [catch {dp_MakeRPCServer $comm(rpc_port) ClientLogin CheckCliCmd} r] {
#    puts stderr "$r"
#    exit
#}
if [catch {RPCServer $comm(rpc_port)} result] {
    puts stderr $result
    exit
}


# Tell everyone we've started up
putmsg [expr $logs(stdout) + $logs(corelog)] "Core program started"
catch {exec wall "Core program started"} 
catch {hdw init} result
catch {monitor start hdw 1} result

# Recover last know temperature control settings
catch {TempRestore} 

# Spawn an event loop if we don't already have one
if {![regexp {wish|expectk} [info nameofexecutable] match]} {
    vwait forever
} else {
    catch {wm withdraw .}
}





