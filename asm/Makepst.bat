@echo off
del pstbin.obj
del pstbin.exe
del pstbin.lst
del pstbin.map
del pstbin.ldr
asm21k -adsp21060 -v -l pstbin.asm
PAUSE
echo             !!!!!!!!!!!!!!!!!!   LINKER  PROGRAM   !!!!!!!!!!!!!!!              
ld21k -m -a ..\ArchFile\arc_a061.ach pstbin
PAUSE
echo             !!!!!!!!!!   CREATE LOADER-FILE  PROGRAM   !!!!!!!!!!!
ldr21k pstbin.exe -blink -fascii -l ..\060_link\060_link.exe -o pstbin.ldr

