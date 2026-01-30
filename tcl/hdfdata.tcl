#!/usr/bin/tclsh

proc HFBSCreateFile { } {
    global run logs counts pv detector

    if [catch {nxopen $run(rundir)/$run(runfile) w} file] {
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Could not open data file for writing"
	return -code error "Could not open data file for writing"	
    }

    # I think the latest version of NAPI writes global attributes for us

    $file create VG {run1 NXentry}
    $file cd {run1 NXentry}

    set val [IsoTime $run(starttime)]
    $file create SDS start_time DFNT_INT8 1 [string length $stime]
    $file write start_time $val
    
    $file create SDS title DFNT_INT8 1 [string length $run(comments)]
    $file write title $run(comments)
    
    $file create SDS duration DFNT_FLOAT32 1 1
    $file write duration 0.0

    set val NBScat
    $file create SDS analysis DFNT_INT8 1 [string length $val]
    $file write analysis $val

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
    $file write wavelength $detector(lambda)
    $file attrib wavelength units angstrom

    $file create SDS sample_temperature DFNT_FLOAT32 1 1
    $file write sample_temperature 300.0
    $file attrib sample_temperature units K

    set val $run(mode)
    $file create SDS daq_mode DFNT_INT8 [string length $val]
    $file write daq_mode $val

    $file create SDS live_time DFNT_FLOAT32 1 1
    $file attrib live_time units seconds

    $file create SDS energy DFNT_FLOAT32 1 2048
    $file attrib energy axis 1

    # Write data only for the detectors we really use
    $file create SDS counts DFNT_FLOAT32 2 {17 2048}
    $file attrib counts signal 1

    # P(v) data - used to extract rate information
    $file create SDS pv DFNT_FLOAT32 2 {17 2048}

    set val 0.0
    for {set i 1} {$i < 17} {incr i} { lappend val $detector($i,angle) }
    $file create SDS angles DFNT_FLOAT32 17
    $file write angles $val
    $file attrib angles axis 0

    # Write dimension scale for the second axis - need to pay some
    # attention as to which mode we're operating in


    # Close file
    rename $file {}
    return
}

proc HFBSWriteCycle { } {
    global run logs counts pv

    if [catch {nxopen $run(rundir)/$run(runfile) w} file] {
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Could not open data file for writing"
	return -code error "Could not open data file for writing"	
    }

    $file cd {run1 NXentry}

    $file write duration $run(tottime)

    $file cd {data NXdata}
    $file awrite counts counts
    $file awrite pv pv
    rename $file {}
    return
}

proc HFBSReadData { infile } {
    global runinfo counts normalize

    set runinfo(filename)  ""
    set runinfo(tottime)   0.0
    set runinfo(starttime) 0
    set runinfo(mode)      standard
    set runinfo(comments)  ""

    if [catch {nxopen $infile r} file] {
	return -code error $file
    }

    if [catch {$file cd {run1 NXentry}} result] {
	rename $file {} 
	return -code error "ERR:\nNo NXentry Vgroup!"
    }

    if [catch {$file read daq_mode} result] {
	rename $file {}
	return -code error "ERR:\n$result"
    }
    set runinfo(mode) $result
    
    # Read raw detector counts
    $file aread counts counts

    # Read P(v)
    $file aread pv pv

    # Read dimension scale
    # $file aread energy

    return "OK:\nRead HFBS data file successfully"
}


