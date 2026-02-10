#!/usr/bin/tclsh
#
# $Id: hdw.tcl,v 1.3 2008/02/11 15:34:27 nickm Exp $
# 
#
# hdw - monolithic command for communicating with instrument hardware
#


proc hdw { cmd args } {
    global fd logs hdw time action

    switch $cmd {
	read {
	    return [ReadHdw]
	}
	init {
	    return [InitHdw]
	}
	hv {
	    if {[llength $args] > 0} {
		switch [lindex $args 0] {
		    enable {
			return [VacHVEnable]
		    }
		    disable {
			return [VacHVDisable]
		    }
		    default {
			return -code error \
				"ERR:\nOptions: enable disable"
		    }
		}
	    } else {
		return -code error "ERR:\nUsage: hdw hv <enable/disable>"
	    }
	}
	clear {
	    return [HdwClear]
	}
	default {
	    return -code error \
		    "ERR:\nOptions: read init"
	}
    }
}

# pst - monolithic command for controlling phase space transform chopper
#
proc pst { args } {
    global fd hdw logs action
    
    if {[llength $args] < 1} {
	return -code error "ERR:\nPST actions: speed enable disable"
    }

    switch [lindex $args 0] {
	speed {
	    if {[llength $args] < 2} {
		return -code error "ERR:\nSupply speed for PST"
	    }
	    return [PSTSpeed [lindex $args 1]]
	}
	enable {
	    return [PSTEnable]
	}
	disable {
	    return [PSTDisable]
	}
	info {
	    return [format "%.1f" $hdw(pst_speed,current)]
	}
	default {
	    return -code error "ERR:\nValid options: speed enable disable info"
	}
    }
}

# doppler - monolithic command for controlling doppler drive
#
proc doppler { args } {
    global fd hdw logs action

    if {[llength $args] < 1} {
	return -code error "ERR:\nSyntax: doppler <options>"
    }

    switch [lindex $args 0] {
	speed {
	    if {[llength $args] < 2} {
		return -code error "ERR:\nSupply speed for Doppler"
	    }
	    return [DopplerSpeed [lindex $args 1]]
	}
	enable {
	    return [DopplerEnable]
	}
	disable {
	    return [DopplerDisable]
	}
	actspeed {
	    if [catch {plc read $fd(plc) $hdw(doppler_hrate,vmem) 1 1} \
		    result] {
		return "0.0"
	    }
	    if [catch {expr $result / 10.0} result] {
		return "0.0"
	    }
	    return [format "%.1f" $result]
	}
	default {
	    return -code error \
		    "ERR:\nDoppler actions: speed enable oscillate disable info"
	}
    }
}

proc InitHdw { } {
    global fd logs hdw time action

    if [catch {modbus create $hdw(modbus,server)} result] {
	return "ERR:\nUnable to connect to PLC"
    } else {
	set hdw(modbus,cmd) $result
    }

    return "OK:\nPLC successfully initialized."
}

proc HdwClear { } {
    global fd logs hdw time action
    
    if [catch {$hdw(modbus,cmd) coil $hdw(clear_flags,coil) 1} result] {
	return -code error $result
    }
    return "OK:\nFlags cleared"
}

proc ReadRegister { fld } {
    global fd logs hdw time action
    
    set validdata $hdw(plc,registers)
    lappend validdata pst_speed
    lappend validdata doppler_speed

    if {[lsearch $validdata $fld] < 0} {
	set result "Valid arguments = [join [lsort $hdw(plc,defined)]]"
	return -code error $result
    }

    if [catch {$hdw(modbus,cmd) register $hdw($fld,reg)} result] {
	return -code error $result
    }

    set hdw($fld,current) $result
    return $result
}

proc WriteRegister { fld val } {
    global fd logs hdw time action
    
    set validdata $hdw(plc,registers)
    lappend validdata pst_speed
    lappend validdata doppler_speed

    if {[lsearch $validdata $fld] < 0} {
	set result "Valid arguments = [join [lsort $hdw(plc,defined)]]"
	return -code error $result
    }

    if [catch {$hdw(modbus,cmd) register $hdw($fld,reg) $val} result] {
	return -code error $result
    }
    return "OK:Register $fld set to $val"
}

proc ReadCoil { fld } {
    global fd logs hdw time action
    
    set validdata $hdw(plc,coils)

    if {[lsearch $validdata $fld] < 0} {
	set result "Valid arguments = [join [lsort $hdw(plc,coils)]]"
	return -code error $result
    }

    if [catch {$hdw(modbus,cmd) coil $hdw($fld,coil)} result] {
	return -code error $result
    }
    return $result
}

proc SetCoil { fld } {
    global fd logs hdw time action

    set validdata $hdw(plc,coils)

    if {[lsearch $validdata $fld] < 0} {
	set result "Valid arguments = [join [lsort $hdw(plc,coils)]]"
	return -code error $result
    }

    if [catch {$hdw(modbus,cmd) coil $hdw($fld,coil) 1} result] {
	return -code error $result
    }
    return $result
}

proc ClearCoil { fld } {
    global fd logs hdw time action

    set validdata $hdw(plc,coils)
    if {[lsearch $validdata $fld] < 0} {
	set result "Valid arguments = [join [lsort $hdw(plc,coils)]]"
	return -code error $result
    }

    if [catch {$hdw(modbus,cmd) coil $hdw($fld,coil) 0} result] {
	return -code error $result
    }
    return $result
}

proc ReadHdw { } {
    global fd logs hdw time action

    set time(hdw,last) [clock seconds]

    # Note: Really should include facility for retrying communications
    # 
    foreach i $hdw(plc,registers) {
	if {![info exists $hdw($i,current)]} {
	    set hdw($i,current) -1
	}
	set hdw($i,last) $hdw($i,current)
	if [catch {ReadRegister $i} result] {
	    set hdw($i,current) -1.0
	    return -code error $result
	} else {
	    set hdw($i,current) $result
	}
    }

    # Read PLC contact inputs
    if [catch {ReadCoils} result] {
	return -code error $result
    }

    return [FormatHdwLogLine]
}

proc ReadCoils { } { 
    global fd logs hdw time action

    foreach i $hdw(plc,coils) {
	if {![info exists $hdw($i,current)]} {
	    set hdw($i,current) -1
	}
	set hdw($i,last) $hdw($i,current)
	if {![catch {ReadCoil $i} result]} {
	    set hdw($i,current) $result
	}
    }
}

# If the PLC has ceased to communicate, periodically
# attempt to reestablish communication with it.
#
proc PLCRecover { } {
    global fd logs hdw time action

    if {!$action(plcdown)} {
	return
    }

   
    set action(plcdown) 1
    set time(ckhdw,last) [clock seconds]
    set time(ckhdw,id) [after [expr 1000 * $time(ckhdw,incr)] PLCRecover]
    return $action(plcdown)
}

# Set PST speed in RPM
#
# Design speed = 4730 rpm
proc PSTSpeed { speed } {
    global fd logs hdw time

    if {$speed > $hdw(pst_speed,limit)} {
	return -code error \
		"ERR:\nRequested PST speed exceeds current limit"
    }

    putmsg $logs(corelog) "Setting PST speed to $speed"
    
    if [catch {WriteRegister pst_setp $speed} result] {
	return -code error $result
    }

    return "OK:\nPST speed set to $speed"
}

proc PSTEnable { } {
    global fd hdw logs

    putmsg $logs(corelog) "Enabling PST"    
    if [catch {SetCoil pst_enable} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] "PLC : $result"
	return "ERR:\nPLC: $result"
    }

    return "OK:\nPST enabled"
}

proc PSTDisable { } {
    global fd hdw logs
    
    putmsg $logs(corelog) "Disabling PST"
    if [catch {ClearCoil pst_enable} result] {
	putmsg [expr $logs(corelog) + $logs(errlog)] "PLC : $result"
	return "ERR:\nPLC: $result"
    }

    return "OK:\nPST disabled"
}

# Set Doppler cam speed in RPM
#
# Old table 
#   Speed(m/s)  Frequency(Hz)  RPM
#     1.0           8.30       
#     2.0          16.64
#     3.0          24.97
#     4.0          33.30
#     5.0          41.63
#     6.0          49.96
#     7.0          58.30
#     8.0          66.63
#     9.0          74.96

proc DopplerSpeed { speed {silent 0}} {
    global fd logs hdw time

    if {$speed > $hdw(doppler_speed,limit)} {
	return -code error \
		"ERR:\nRequested Doppler speed exceeds current limit"
    }

    if [catch {WriteRegister doppler_setp $speed} result] {
	return -code error $result
    }

    return "OK:\nDoppler speed set to $speed"
}

proc DopplerEnable { } {
    global fd hdw logs

    putmsg $logs(corelog) "Enabling doppler drive"
    if [catch {SetCoil doppler_enable} result] {
	return -code error $result
    }

    return "OK:\nDoppler drive enabled"
}

proc DopplerDisable { } {
    global fd hdw logs

    putmsg $logs(corelog) "Disabling doppler drive"    
    if [catch {ClearCoil doppler_enable} result] {
	return -code error $result
    }

    return "OK:\nDoppler drive disabled"
}

proc VacHVEnable { } {
    global hdw time fd logs

    putmsg $logs(corelog) "Enabling HV"
    if [catch {SetCoil vac_hv_en} result] {
	return -code error $result
    }

    return "OK:\nChamber HV enabled"
}

proc VacHVDisable { } {
    global hdw time fd logs

    putmsg $logs(corelog) "Disabling HV"
    if [catch {ClearCoil vac_hv_en} result] {
	return -code error $result
    }

    return "OK:\nChamber HV disabled"
}

