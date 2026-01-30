@echo off
del encbin.obj
del encbin.exe
del encbin.lst
del encbin.map
del encbin.ldr
asm21k -adsp21060 -v -l encbin.asm
PAUSE
echo             !!!!!!!!!!!!!!!!!!   LINKER  PROGRAM   !!!!!!!!!!!!!!!              
ld21k -m -a ..\ArchFile\arc_a061.ach encbin
PAUSE
echo             !!!!!!!!!!   CREATE LOADER-FILE  PROGRAM   !!!!!!!!!!!
ldr21k encbin.exe -blink -fascii -l ..\060_link\060_link.exe -o encbin.ldr

