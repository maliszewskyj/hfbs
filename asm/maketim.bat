@echo off
del timbin.obj
del timbin.exe
del timbin.lst
del timbin.map
del timbin.ldr
asm21k -adsp21060 -v -l timbin.asm
PAUSE
echo             !!!!!!!!!!!!!!!!!!   LINKER  PROGRAM   !!!!!!!!!!!!!!!              
ld21k -m -a ..\ArchFile\arc_a061.ach timbin
PAUSE
echo             !!!!!!!!!!   CREATE LOADER-FILE  PROGRAM   !!!!!!!!!!!
ldr21k timbin.exe -blink -fascii -l ..\060_link\060_link.exe -o timbin.ldr

