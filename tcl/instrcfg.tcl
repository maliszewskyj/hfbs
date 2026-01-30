#!/usr/bin/tclsh

#
# Configure instrument hardware
#
set device(plc,line)     /dev/ttyPlc
set device(plc,baud)     9600
set device(plc,bytesize) 8
set device(plc,parity)   odd
set device(tc,line)      /dev/ttyTemp
set device(tc,baud)      9600
set device(tc,bytesize)  7
set device(tc,parity)    odd

#
# Detector configuration
#
#   set angles in degrees
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
# Known temperature controllers
#
set tc(known)    { lk93c lk330 lk340 w5010 omcni scmag eurot cryo34 }
set tc(setpoint) 300.0
set tc(sensor)     0.0
