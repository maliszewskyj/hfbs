#!/usr/bin/tclsh

if {[llength $argv] <= 0} { exit }
set filename [lindex $argv 0]
#set filename /home/hfbs/data/19990430_11.hfbs
#set filename /home/hfbs/data/19981028_04.hfbs
package require narray
load libnxtcl.so
load libmtxutil.so

array set chans { teach 4096 standard 2048 }
array set dets  { teach   20 standard   23 }
narray create fullsize     23 4096
narray create total        20 4096
narray create data         1 1
narray create normalize    1 1
narray create work2        1 1

proc FormatTime { Timeval } {
    return [clock format $Timeval -format "%d-%b-%Y %H:%M:%S"]
}

proc ReadHFBS { filename } {
    global runinfo mode

    # Reset runinfo variables
    set runinfo(filename)  ""
    set runinfo(tottime)   0.0
    set runinfo(starttime) "01-Jan-1970 00:00:00"
    set runinfo(mode)      teach
    set runinfo(comments)  ""

    if [catch {open $filename r} infile] {
	return -code error $infile
    }
    set runinfo(filename) $filename
    set curdet 0
    while {[gets $infile line] >= 0} {
	if [regexp {^#} $line] {
	    if [regexp {^#Live([^:]*):(.*)} $line match spaces livetime] {
		set livetime [expr 1.0 * $livetime]
		set runinfo(tottime) $livetime
	    }
	    if [regexp {^#Start([^:]*):(.*)} $line match spaces starttime] {
		set runinfo(starttime) $starttime
	    }
	    if [regexp {^#Comments([^:]*):(.*)} $line match spaces comments] {
		set runinfo(comments) $comments
	    }
	    if [regexp {^#Run Mode([^:]*):(.*)} $line match spaces vmemode] {
		set mode [string trimleft [string trimright $vmemode]]
		set runinfo(mode) $mode
		switch $mode {
		    standard {
			fullsize export {0 0} {23 2048} total
		    }
		    teach {
			fullsize export {0 0} {20 4096} total
		    }
		    default {
			close $infile
			return "ERR:\nUnrecognized mode"
		    }
		}
	    }
	    if [regexp {^# Detector(.*)} $line match det] {
		set curdet [expr 1 * $det]
	    }
	} else {
	    if {[scan $line "%d %f" chan val] == 2} {
		total aset $curdet $chan $val
	    }
	}
    }
    close $infile
    return $filename
}

#
# Split block of data received from VME system into portions suitable for
# writing to a file.
#
proc SplitMats { } {
    global total work2 mode livetime dets chans view

    total export {0 0} [list 20 $chans($mode)] data
    total export {0 0} [list 20 $chans($mode)] normalize
    switch $mode {
	teach {
	    # Build normalization histogram
	    total export {19 0} {1 4096} work2
	    for {set i 0} {$i < 20} {incr i} {
		normalize import [list $i 0] work2
	    }
	    work2 map {[] = 0;}
	    data import {19 0} work2
	}
	standard {

	    # Build monitor sample histogram
	    total export {20 0} {1 2048} work2
	    normalize import {0 0} work2
	    # Build low angle detector sample histogram
	    total export {21 0} {1 2048} work2
	    for {set i 1} {$i < 5} {incr i} {
		normalize import [list $i 0] work2
	    }
	    # Build high angle detector sample histogram
	    total export {22 0} {1 2048} work2
	    for {set i 5} {$i < 20} {incr i} {
		normalize import [list $i 0] work2
	    }
	}
	default {
	    return -code error "Available modes : standard teach"
	}
    }
    return 0
}


proc HFBSCreateFile { } {
    global run logs data normalize

    if [catch {nxopen $run(rundir)/$run(runfile) w} file] {
#	putmsg [expr $logs(stdout) + $logs(errlog)] \
#		"Could not open data file for writing"
#	set action(doingrun) 0
	return -code error "Could not open data file for writing"	
    }

    # I think the latest version of NAPI writes global attributes for us

    $file create VG {run1 NXentry}
    $file cd {run1 NXentry}

    set stime [FormatTime $run(starttime)]
    $file create SDS start_time DFNT_INT8 1 [string length $stime]
    $file write start_time $stime
    
    $file create SDS title DFNT_INT8 1 [string length $run(comments)]
    $file write title $run(comments)
    
    $file create SDS program_name DFNT_INT8 1 [string length "hfbscore"]
    $file write program_name "hfbscore"

    $file create SDS intent DFNT_INT8 1 [string length "data"]
    $file write intent "data"

    $file create SDS plot_type DFNT_INT8 1 [string length "image"]
    $file write plot_type "image"

    $file create VG {HFBS NXinstrument}
    $file cd {HFBS NXinstrument}

    set val "NIST Center for Neutron Research"
    $file create SDS location DFNT_INT8 1 [string length $val]
    $file write location $val

    set val "Neutron Backscattering Spectrometer"
    $file create SDS type DFNT_INT8 1 [string length $val]
    $file write type $val

    $file create VG {NBSR NXsource}
    $file cd {NBSR NXsource}

    set val "Neutron Beam Split-Core Reactor, NIST" 
    $file create SDS name DFNT_INT8 1 [string length $val]
    $file write name $val

    set val "neutron/reactor"
    $file create SDS type DFNT_INT8 1 [string length $val]
    $file write type $val

    set val "20 MW"
    $file create SDS power DFNT_INT8 1 [string length $val]
    $file write power $val

    $file cd ..
    $file create VG {selector NXvelocity_selector}
    $file create VG {chopper NXpst_chopper}
    $file create VG {doppler NXdoppler_drive}
    $file create VG {beam_monitor NXmonitor}
    $file create VG {analyzer NXanalyzer}

    $file create VG {detector NXdetector}
    $file cd .. ;# End instrument Vgroup

    $file create VG {sample NXsample}
    
    $file create VG {data NXdata}
    $file cd {data NXdata}

    $file create SDS wavelength DFNT_FLOAT32 1 1
    $file write wavelength 6.100
    $file attrib wavelength units angstrom

    $file create SDS sample_temperature DFNT_FLOAT32 1 1
    $file write sample_temperature 300.0
    $file attrib sample_temperature units K

    $file create SDS live_time DFNT_FLOAT32 1 1
    $file attrib live_time units seconds

    $file create SDS velocity DFNT_FLOAT32 1 2048

    # Write data only for the detectors we really use

    $file create SDS intensity DFNT_FLOAT32 2 {17 2048}

    $file create SDS normalize DFNT_FLOAT32 2 {17 2048}

    # Close file
    rename $file {}
    return
}

proc HFBSWriteCycle { } {
    global run logs data normalize

    if [catch {nxopen $run(rundir)/$run(runfile) w} file] {
#	putmsg [expr $logs(stdout) + $logs(errlog)] \
#		"Could not open data file for writing"
#	set action(doingrun) 0
	return -code error "Could not open data file for writing"	
    }

    $file cd {run1 NXentry}
    $file cd {data NXdata}
    $file write live_time $run(tottime)
    $file awrite intensity data
    $file awrite normalize normalize
    rename $file {}
    return
}

proc HFBSReadData { infile } {
    global runinfo data normalize

    set runinfo(filename)  ""
    set runinfo(tottime)   0.0
    set runinfo(starttime) "01-Jan-1970 00:00:00"
    set runinfo(mode)      teach
    set runinfo(comments)  ""

    if [catch {nxopen $infile r} file] {
	return -code error $file
    }

    if [catch {$file cd {run1 NXentry}} result] {
	rename $file {} 
	return -code error "ERR:\nNo NXentry Vgroup!"
    }

    

    return "OK:\nRead HFBS data file successfully"
}

####################################


if [catch {ReadHFBS $filename} result] {
    puts $result
    exit
}

array set run [array get runinfo]
set run(runfile) out.hdf
set run(rundir) [pwd]
set run(starttime) [clock seconds]
mtx split standard total data normalize
puts "Finished reading file"
puts "Creating HDF file"
HFBSCreateFile
puts "Writing HDF data"
HFBSWriteCycle

