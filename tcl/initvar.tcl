#!/usr/bin/tclsh
# $Id: initvar.tcl,v 1.14 2009/06/04 21:14:55 nickm Exp $
#
# Global arrays
#
#   time     - scheduling variables
#   fd       - device file descriptors
#   device   - device port attributes
#   hdw      - hardware values (pressure, temp, etc)
#   detector - detector attributes
#   logs     - log file flags
#   action   - current action flags
#   tc       - details of temperature control
#   comm     - details of RPC communication

# hdw - stores most recently read hardware values
#
set hdw(modbus,cmd)                 null
set hdw(modbus,server)              129.6.120.55
set hdw(doppler_t_in,last)          -1
set hdw(doppler_t_in,current)       -1
set hdw(doppler_t_out,last)         -1
set hdw(doppler_t_out,current)      -1
set hdw(doppler_t_bsh,last)         -1
set hdw(doppler_t_bsh,current)      -1
set hdw(doppler_p_in,last)          -1
set hdw(doppler_p_in,current)       -1
set hdw(doppler_p_out,last)         -1
set hdw(doppler_p_out,current)      -1
set hdw(doppler_flow,last)          -1
set hdw(doppler_flow,current)       -1
set hdw(doppler_speed,last)         -1
set hdw(doppler_speed,current)      -1
set hdw(doppler_hrate,last)         -1
set hdw(doppler_hrate,current)      -1
set hdw(doppler_setp,last)          -1
set hdw(doppler_setp,current)       -1
set hdw(pst_flow,last)              -1
set hdw(pst_flow,current)           -1
set hdw(pst_temp,last)              -1
set hdw(pst_temp,current)           -1
set hdw(pst_hrate,last)             -1
set hdw(pst_hrate,current)          -1
set hdw(pst_speed,last)             -1
set hdw(pst_speed,current)          -1
set hdw(pst_setp,last)              -1
set hdw(pst_setp,current)           -1
set hdw(vac,last)                   -1
set hdw(vac,current)                -1

# contact (logic) inputs
set hdw(doppler_enable,current)      0 ;# C120 Doppler software enable
set hdw(doppler_oilpressure,current) 0 ;# C20  Oil pressure switch
set hdw(doppler_oiltemp,current)     0 ;# C21  Oil temperature switch
set hdw(doppler_vibration,current)   0 ;# C22  Vibration fault
set hdw(pst_enable,current)          0 ;# C140 PST software enable
set hdw(pst_vibration,current)       0 ;# C23  Drive fault
set hdw(pst_drivefault,current)      0 ;# C24  Bearing damage
set hdw(vac_hv_en,current)           0 ;# C150 Vacuum HV enable

set hdw(doppler_t_in,range)        200.0
set hdw(doppler_t_out,range)       200.0

set hdw(doppler_t_bsh,range)       200.0
set hdw(doppler_p_in,range)        100.0
set hdw(doppler_p_out,range)       100.0
set hdw(doppler_flow,range)         31.2
set hdw(pst_flow,range)             31.2
set hdw(pst_temp,range)              1.0

set hdw(doppler_t_in,offset)         0.0
set hdw(doppler_t_out,offset)        0.0
set hdw(doppler_t_bsh,offset)        0.0
set hdw(doppler_p_in,offset)         0.0
set hdw(doppler_p_out,offset)        0.0
set hdw(doppler_flow,offset)         0.0
set hdw(pst_flow,offset)             0.0
set hdw(pst_temp,offset)          -273.15
set hdw(pst_hrate,offset)            0.0

set hdw(doppler_t_in,units)          "C"
set hdw(doppler_t_out,units)         "C"
set hdw(doppler_t_bsh,units)         "C"
set hdw(doppler_p_in,units)          "psig" 
set hdw(doppler_p_out,units)         "psig"
set hdw(pst_flow,units)              "fps"
set hdw(pst_temp,units)              "C"
set hdw(pst_hrate,units)             "?"
set hdw(vac,units)                   "torr"

# vmemory locations (in octal) in the plc for the monitored quantities
#         Note that V memory is specified in octal (add 1 to get hex address)
#

# Modbus holding register addresses. Should really be of the form "400009", but
# we can truncate the address to "9"
#
#
set hdw(doppler_t_in,reg)          1045
set hdw(doppler_t_out,reg)         1047
set hdw(doppler_t_bsh,reg)         1055
set hdw(doppler_p_in,reg)          1041
set hdw(doppler_p_out,reg)         1042
set hdw(doppler_flow,reg)          1053
set hdw(doppler_hrate,reg)         1001 ;# This _was_ a speed reading originally
set hdw(pst_flow,reg)              1063
set hdw(pst_temp,reg)              1059
set hdw(pst_hrate,reg)             1013 ;# This _was_ also a speed reading
set hdw(vac,reg)                   1029
set hdw(doppler_speed,reg)         1001
set hdw(doppler_setp,reg)          2001
set hdw(pst_speed,reg)             1013
set hdw(pst_setp,reg)              2009

# Software enables for doppler & PST; specify in hexadecimal
#

set hdw(doppler_enable,coil)       2001 ;# Doppler Enable Request
set hdw(pst_enable,coil)           2003 ;# PST Enable Request
set hdw(vac_hv_en,coil)            2002 ;# HV Enable Request
set hdw(doppler_oilpressure,coil)  1026 ;# Doppler Oil Pressure Flag
set hdw(doppler_oiltemp,coil)      1032 ;# Doppler Oil  Temp Flag
set hdw(doppler_vibration,coil)    1041 ;# Doppler Vibration Flag
set hdw(pst_vibration,coil)        1046 ;# PST Vibration Flag
set hdw(clear_flags,coil)          2004 ;# Clear All Flags Request

# Build a list of known devices/measured quantities
# 
set hdw(plc,registers)         {doppler_t_in doppler_t_out \
	                        doppler_p_in doppler_p_out \
                                doppler_t_bsh doppler_flow \
				doppler_hrate \
                                pst_flow pst_temp \
			        pst_hrate vac \
				doppler_speed pst_speed \
				doppler_setp pst_setp }
set hdw(plc,coils)             {doppler_enable doppler_oilpressure \
                                doppler_oiltemp doppler_vibration \
                                pst_enable pst_vibration \
  			        vac_hv_en }

# Upper limit for the speeds of the Doppler and PST
#      For now, just make a guess
set hdw(pst_speed,limit)        4900.0
set hdw(doppler_speed,limit)    75.0

# The Doppler drive motor and PST motor speeds are programmed though the
# PLC by setting a voltage. The range is the mapping between the digital
# value in the DAC and the actual speed of the device
#      PST Speed specified in RPM
#      Doppler Speed specified in Hz
#
set hdw(doppler_speed,slope)       0.01954
set hdw(pst_speed,slope)           1.217076
set hdw(doppler_speed,icept)      -0.3728
set hdw(pst_speed,icept)           2.176429

set hdw(pst_hrate,slope)           2.26919;# 2.32339 - Changed by Vicky 3/23/06


# Vmemory locations for the DAC's which program the speeds of the
# Doppler and PST
#
set hdw(pst_speed_set,vmem)      2311;
set hdw(doppler_speed_set,vmem)  2310; 
# New parameters for oscillation
set hdw(doppler_avg_set,vmem)    2340;
set hdw(doppler_amp_set,vmem)    2350;
set hdw(doppler_per_set,vmem)    2360; 


#
# Initialize port characteristics for instrumentation
#
foreach devicename { plc tc } {
    set fd(${devicename})                -1
    set device(${devicename},line)       /dev/null
    set device(${devicename},baud)       9600
    set device(${devicename},bytesize)   8
    set device(${devicename},parity)     none
}

#
# Detector configuration
#
#   set angles in degrees
set detector(0,angle)    0.00 ;# monitor
set detector(1,angle)   14.46
set detector(2,angle)   20.98
set detector(3,angle)   27.08
set detector(4,angle)   32.31
set detector(5,angle)   36.00
set detector(6,angle)   43.75
set detector(7,angle)   51.50
set detector(8,angle)   59.25
set detector(9,angle)   67.00
set detector(10,angle)  74.75
set detector(11,angle)  82.50
set detector(12,angle)  90.25
set detector(13,angle)  98.00
set detector(14,angle) 105.75
set detector(15,angle) 113.50
set detector(16,angle) 121.25
set detector(lambda)     6.271

#
# time - stores event and action times
#
set time(corestart)    [clock seconds]
set time(current)      [clock seconds]
set time(hdw,last)     [clock seconds]
set time(hdw,incr)     60
set time(hdw,id)       -1
set time(ckhdw,last)   [clock seconds]
set time(ckhdw,incr)   60
set time(ckhdw,id)     -1
set time(logfile,last) [clock seconds]
set time(logfile,incr) 600
set time(logfile,id)   -1
set time(run,last)     [clock seconds]
set time(run,incr)     60
set time(run,id)       -1
set time(doppler,last) [clock seconds]
set time(doppler,incr) 300
set time(doppler,id)   -1
set time(temp,last)    [clock seconds]
set time(temp,incr)    10
set time(temp,id)      -1



#
# action flags
#
set action(plcdown)          0 ;# PLC is not communicating with core
set action(doingrun)         0
set action(doingseq)         0
set action(oscillate)        0 ;# Oscillation of doppler drive speed
set action(monitor,hdw)      0
set action(check,hdw)        0
set action(monitor,temp)     0

#
# Log file flags - see putmsg.tcl
#
set logs(monlog)         1
set logs(corelog)        2
set logs(userlog)        4
set logs(errlog)         8
set logs(status)        16
set logs(stdout)        32
set logs(rxcycle)       200802
set logs(logdir)        /usr/local/hfbs/log
set logs(statdir)       /home/ncnr
set logs(corefile)      hfbscore.log
set logs(userfile)      user.log
set logs(errfile)       hdwerr.log
set logs(statfile)      status.hfbs
set logs(seqfile)       sequence.log
set logs(monfile)       "hdwmon_$logs(rxcycle).log"

set run(rundir)         /usr/local/hfbs/data/$logs(rxcycle)
set run(runfile)        ""
set run(lockfile)       /tmp/hfbs.lock
set run(mode)           velocity
set run(user)           ""
set run(starttime)      -1
set run(ncycles)        999
set run(cyctime)        600
set run(curcycle)       0
set run(cycstart)       -1
set run(tottime)        0.0
set run(inprogress)     0
set run(comments)       ""
set run(varytemp)       0
set run(npts)           0
set run(readtemp)       0
set run(checktemp)      0
set run(checkdop)       0
set run(holdtime)       5
set run(tempstart)      300.0
set run(tempinc)        5.0
set run(curtemp)        300.0
set run(curset)         300.0
set run(scanvec)        ""
set run(timebin)        30
set run(dopfreq)        {}
set run(wbm)            {}
set run(tbm)            {}
set run(sampletemp)     {}
set run(controltemp)    {}
set run(xprange)        200

set comm(clients)       {}
set comm(rpc_port)      51330

set tc(controller)      null
set tc(setpoint)        300.0
set tc(sensor)          sample
set tc(sample)          300.0
set tc(control)         300.0
set tc(auxiliary)       300.0

# Commit the histogram "total" to shared memory
# acquire share total
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#
# Status register. Each list item represents an activity in progress
#                  0: 1 if choppers shutting down
#                  1: 1 if setting chopper wl
#                  2: 1 if counting on scalers
#                  3: 1 if monitoring hardware
#                  4: 1 if pc not communicating
#
set statreg { 0 0 0 0 0 }
