#!/usr/bin/tclsh
#
# $Id: logfile.tcl,v 1.2 2000/09/07 11:49:54 nickm Exp $

proc FormatNewLogHdr { Filename } {
    global time
    set    outstring "# "
    append outstring [FormatTime $time(corestart) ]
    append outstring "\n"
    append outstring "# $Filename"
    return $outstring
}

proc FormatLogTime { } {
    global time

    set curtime [FormatTime [clock seconds]]
    set plctime [FormatTime $time(hdw,last)   ]
    set    outstring "!$curtime"
    append outstring "   PLC:$plctime"
    return $outstring
}

proc FormatHdwLogLine { } {
    global time hdw

    GetElapsedTime $time(corestart) Elapsed
    append outstring "!  Absolute_Time"
    append outstring "  VACUUM: EN"
    append outstring "  DOPPLER: EN OP OT VF"
    append outstring "  PST:   EN DF BD\n"
    append outstring "0 "
    set abstime [expr [clock seconds]/3600.0]
    append outstring [format "%13.3f " $abstime]
    append outstring [format " %7.3f " $hdw(vac,current)]
    append outstring " [FormatOnOff $hdw(vac_hv_en,current)]  "
    append outstring [format " %6.1f   " $hdw(doppler_setp,current)]
    append outstring "[FormatOnOff $hdw(doppler_enable,current)]  "
    append outstring "[FormatOnOff $hdw(doppler_oilpressure,current)]  "
    append outstring "[FormatOnOff $hdw(doppler_oiltemp,current)]  "
    append outstring "[FormatOnOff $hdw(doppler_vibration,current)]  "
    append outstring [format " %6.1f" $hdw(pst_setp,current)]
    append outstring " "
    append outstring "[FormatOnOff $hdw(pst_enable,current)]  "
    append outstring "[FormatOnOff $hdw(pst_drivefault,current)]  "
    append outstring "[FormatOnOff $hdw(pst_vibration,current)]  "
    append outstring "\n"
    append outstring "! DOPSPD   P_IN  P_OUT   T_IN  T_OUT  T_BSH  DFLOW"
    append outstring " PSTSPD  T_WIN  PFLOW\n "
    append outstring [format " %6.1f" $hdw(doppler_speed,current)]
    append outstring [format " %6.1f" $hdw(doppler_p_in,current)]
    append outstring [format " %6.1f" $hdw(doppler_p_out,current)]
    append outstring [format " %6.1f" $hdw(doppler_t_in,current)]
    append outstring [format " %6.1f" $hdw(doppler_t_out,current)]
    append outstring [format " %6.1f" $hdw(doppler_t_bsh,current)]
    append outstring [format " %6.1f" $hdw(doppler_flow,current)]
    append outstring [format " %6.1f" $hdw(pst_speed,current)]
    append outstring [format " %6.1f" $hdw(pst_temp,current)]
    append outstring [format " %6.1f" $hdw(pst_flow,current)]
    return $outstring
}

#
#
#
proc FormatOnOff { val } {
    if { $val } {
	return "Y"
    } 
    return "N"
}

proc FormatLogBlk { } {
    global hdw
    set    outstring "[FormatLogTime]\n"
    append outstring "[FormatHdwLogLine]"
    return $outstring
}
