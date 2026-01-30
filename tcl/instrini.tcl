#!/usr/bin/tclsh
# $Id: instrini.tcl,v 1.2 2000/05/15 20:28:20 nickm Exp $
#
# Open and initialize serial devices
#
foreach dev { plc tc } {
    if [catch {serialinit $device($dev,line) $device($dev,baud) \
	    $device($dev,bytesize) $device($dev,parity)} fd($dev)] {
	puts stderr "Unable to open $device($dev,line) for $dev"
	exit
    }
}

# Initialize PLC
catch {InitHdw}

# Stop data acquisition in progress (if any)
catch {acquire stop}