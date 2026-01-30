#!/usr/bin/tclsh

#
# Monolithic command for managing sample temperature
#
proc temperature { option args } {
    global tc fd
    switch $option {
	select {
	    # select and initialize temperature controller
	    if {[llength $args] < 1} {
		return -code error "ERR:\nSpecify temperature controller type"
	    }
	    return [TempSelect [lindex $args 0]]
	}
	set {
	    if [string match null $tc(controller)] {
		return -code error "ERR:\nNo controller selected"
	    }
	    if {[llength $args] < 1} {
		return -code error "ERR:\nSpecify setpoint in Kelvin"
	    }
	    if [catch {expr 1.0 * [lindex $args 0]} result] {
		return -code error "ERR:\nSpecify setpoint in Kelvin"
	    }
	    return [TempSet $result]
	}
	configure {
	    if [string match null $tc(controller)] {
		return -code error "ERR:\nNo controller selected"
	    }
	    if {[llength $args] < 2} {
		return -code error "ERR:\nSpecify parameter and value"
	    }
	    switch [lindex $args 0] {
		sampsensor {
		    set sensor [lindex $args 1]
		    if [catch {$tc(controller) $fd(tc) sampsensor $sensor} result] {
			return -code error $result
		    }
		    return "OK:\nSample sensor set to $sensor"
		}
		ctrlsensor {
		    set sensor [lindex $args 1]
		    if [catch {$tc(controller) $fd(tc) ctrlsensor $sensor} result] {
			return -code error $result
		    }
		    return "OK:\nControl sensor set to $sensor"
		}
		auxsensor {
		    set sensor [lindex $args 1]
		    if [catch {$tc(controller) $fd(tc) auxsensor $sensor} result] {
			return -code error $result
		    }
		    return "OK:\nAuxiliary sensor set to $sensor"
		}
		default {
		    return
		}
	    }
	}
	sensor {
	    if {[llength $args] < 1} {
		return -code error "ERR:\nSpecify temperature sensor"
	    }
	    switch [lindex $args 0] {
		control { set tc(sensor) control }
		sample  { set tc(sensor) sample  }
		both    { set tc(sensor) both    }
		default {
		    return -code error "ERR:\nValid sensors: control sample both"
		}
	    }
	    return "OK:\nTemperature sensor set to [lindex $args 0]"
	}
	read {
	    if [string match null $tc(controller)] {
		set tc(control)  0.0
		set tc(sample)   0.0
		set tc(setpoint) 0.0
		set tc(auxiliary) 0.0
		return -code error "ERR:\nNo controller selected"
	    }
	    return [TempRead]
	}
	ramp {
	    if {[llength $args] < 1} {
		return -code error "ERR:\nSpecify ramp rate"
	    }
	    if [string match null $tc(controller)] {
		return -code error "ERR:\nNo controller selected"
	    }
	    return [TempRamp [lindex $args 0]]
	}
	status {
	    if [string match null $tc(controller)] {
		return -code error "ERR:\nNo controller selected"
	    }
	    return [TempStatus]
	}
	default {
	    return -code  error "Options: select set read"
	}
    }
}

# Select a temperature controller from a list of those known
proc TempSelect { controller } {
    global tc fd device logs 

    if {$fd(tc) > 0} {
	if [catch {serialclose $fd(tc)} result] {
	    putmsg [expr $logs(corelog) + $logs(errlog)] $result
	}
	set fd(tc) -1
    }
    if {[lsearch $tc(known) $controller] < 0} {
	set tc(controller) null
	set tc(control)    0.0
	set tc(sample)     0.0
	set tc(setpoint)   0.0
	return -code error \
		"ERR:\nUnknown controller type"
    }
    set tc(controller) $controller
    # Reset sensor selection to sample
    set tc(sensor) sample
    # Open serial port for temperature controller
    switch $tc(controller) {
	w5010 {
	    set baud     4800
	    set bytesize 7
	    set parity   even    
	}
	cryo34 {
	    set baud     9600
	    set bytesize 8
	    set parity   none
	}
	lk340 {
	    set baud     9600
	    set bytesize 7
	    set parity   odd
	}
	default {
	    set baud     $device(tc,baud)
	    set bytesize $device(tc,bytesize)
	    set parity   $device(tc,parity)
	}
    }

    if [catch {serialinit $device(tc,line) $baud $bytesize $parity} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] $result
	set tc(controller) null
	return "ERR:\nCould not open serial port for temperature controller"
    }
    set fd(tc) $result

    # Cache the last known good controller
    catch {TempSave}
    return "OK:\nController set to $tc(controller)"
}

proc TempSave { } {
    global tc configdir
    set file ${configdir}/sampleenv.tcl
    if [catch {open $file w} f] {
	return -code error $f
    }
    puts $f "set tc(controller) $tc(controller)\n"
    close $f
}

proc TempRestore { } {
    global tc
    global tc configdir
    set file ${configdir}/sampleenv.tcl
    if {![file exists $file]} {
	return
    }
    if [catch {source $file} result] {
	return -code error $result
    }
    return [TempSelect $tc(controller)]
}

proc TempSet { setpoint } {
    global tc fd logs
    # Should be as simple as $tc(controller) $fd setpoint $setpoint

    if [catch {$tc(controller) $fd(tc) setpoint $setpoint} result] {
	set tc(setpoint) -1.0
	# Should write error to log file
	putmsg [expr $logs(corelog) + $logs(errlog)] "TC : $result"
	return -code error "ERR:\nUnable to set sample temp => $result"
    }

    set tc(setpoint) $setpoint
    putmsg [expr $logs(corelog) + $logs(stdout)] \
	    "Temperature set to $setpoint"

    return "OK:\nTemperature set to $setpoint"
}

# Read all status information from controllers
#
proc TempStatus { } {
    global tc fd logs

    catch {TempRead}
    
    if [catch {$tc(controller) $fd(tc) setpoint} result] {
	# Keep previous setpoint
    } else {
	set tc(setpoint) [expr $result * 1.0]
    }
    
    if [catch {$tc(controller) $fd(tc) heater} result] {
	set tc(heater) -1
    } else {
	set tc(heater) [expr $result * 1.0]
    }

    if [catch {$tc(controller) $fd(tc) range} result] {
	set tc(range) -1
    } else {
	set tc(range) [expr $result * 1.0]
    }

    if [catch {$tc(controller) $fd(tc) manual} result] {
	set tc(manual) -1
    } else {
	set tc(manual) [expr $result * 1.0]
    }


    # Return a list of values
    set output [list sample $tc(sample) control $tc(control) ]
    lappend output setpoint
    lappend output $tc(setpoint) 
    lappend output range 
    lappend output $tc(range) 
    lappend output heater 
    lappend output $tc(heater) 
    lappend output manual 
    lappend output $tc(manual)
    return $output
}

proc TempRamp { {rate 0.0} } {
    global tc fd logs

    # Rate of <= 0.0 turns off ramping
    set rampflg off
    if { $rate <= 0.0 } { set rate 0.0 }
    if { $rate > 0.0 } { set rampflg on }
    if [catch {$tc(controller) $fd(tc) ramp $rampflg $rate} result] {
	putmsg [expr $logs(stdout) + $logs(errlog)] \
	    "Error setting ramp: $result"
	return -code error "ERR:\nUnable to read sample temp => $result"
    }
    return "OK:\nSet ramp $rampflg, rate = $rate"
}

# Read both sample and control sensors
#
proc TempRead { } {
    global tc fd logs

    set iotest 0
    while [catch {$tc(controller) $fd(tc) sample} result] {
	set tc(sample)  -1.0
	set tc(control) -1.0
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Error reading from TC: $result"
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Retry reading from TC"
	after 5000
	incr iotest
	if {$iotest > 2} {
	    return -code error "ERR:\nUnable to read sample temp => $result"
	}
    }
    set tc(sample) [expr $result * 1.0]

    set iotest 0
    while [catch {$tc(controller) $fd(tc) control} result] {
	set tc(control) -1.0
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Error reading from TC: $result"
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Retry reading from TC"
	after 5000
	incr iotest
	if {$iotest > 2} {
	    # Could try to reopen serial port for tc
	    # Do this by "reselecting" the temperature controller
	    
	    return -code error "ERR:\nUnable to read control temp => $result"
	}
    }
    set tc(control) [expr $result * 1.0]

    set iotest 0
    while [catch {$tc(controller) $fd(tc) setpoint} result] {
	set tc(control) -1.0
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Error reading from TC: $result"
	putmsg [expr $logs(stdout) + $logs(errlog)] \
		"Retry reading from TC"
	after 5000
	incr iotest
	if {$iotest > 2} {
	    # Could try to reopen serial port for tc
	    # Do this by "reselecting" the temperature controller
	    
	    return -code error "ERR:\nUnable to read control temp => $result"
	}
    }
    set tc(setpoint) [expr $result * 1.0]

    set iotest 0
    while [catch {$tc(controller) $fd(tc) auxiliary} result] {
	set tc(control) -1.0
	putmsg [expr $logs(stdout) + $logs(errlog)] \
	    "Error reading from TC: $result"
	putmsg [expr $logs(stdout) + $logs(errlog)] \
	    "Retry reading from TC"
	after 5000
	incr iotest
	if {$iotest > 2} {
	    # Could try to reopen serial port for tc
	    # Do this by "reselecting" the temperature controller
	    
	    return -code error "ERR:\nUnable to read aux temp => $result"
	}
    }
    set tc(auxiliary) [expr $result * 1.0]


    set result    "OK:\n"
    append result "Setpnt  Temperature = $tc(setpoint)\n"
    append result "Sample  Temperature = $tc(sample)\n"
    append result "Control Temperature = $tc(control)\n"
    append result "Aux     Temperature = $tc(auxiliary)"

    return $result
}
