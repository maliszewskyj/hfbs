#include Makefile.arch
ObjSuf        = o
SrcSuf        = cxx
ExeSuf        =
DllSuf        = so
OutPutOpt     = -o # keep whitespace after "-o"



CXX           = g++
CXXFLAGS      = $(OPT2) -Wall -fPIC
LD            = g++
LDFLAGS       = $(OPT2)
SOFLAGS       = -shared




CXXFLAGS += -I../.. -I../../dev/pci

LDFLAGS  += -L../.. 



LDFLAGS  += -lrt



SIS3100_CLASS_CONTROL_READO      = sis3100_class_control_read.$(ObjSuf) sis1100linux_vme_class.o  
SIS3100_CLASS_CONTROL_READS      = sis3100_class_control_read.$(SrcSuf)
SIS3100_CLASS_CONTROL_READ       = sis3100_class_control_read$(ExeSuf) 

SIS3100_CLASS_CONTROL_WRITEO      = sis3100_class_control_write.$(ObjSuf) sis1100linux_vme_class.o  
SIS3100_CLASS_CONTROL_WRITES      = sis3100_class_control_write.$(SrcSuf)
SIS3100_CLASS_CONTROL_WRITE       = sis3100_class_control_write$(ExeSuf) 


SIS3100_CLASS_CONTROL_READO      = sis3100_class_control_read.$(ObjSuf) sis1100linux_vme_class.o  
SIS3100_CLASS_CONTROL_READS      = sis3100_class_control_read.$(SrcSuf)
SIS3100_CLASS_CONTROL_READ       = sis3100_class_control_read$(ExeSuf) 

SIS3100_CLASS_VMEA32D32_WRITEO      = sis3100_class_vmeA32D32write.$(ObjSuf) sis1100linux_vme_class.o  
SIS3100_CLASS_VMEA32D32_WRITES      = sis3100_class_vmeA32D32write.$(SrcSuf)
SIS3100_CLASS_VMEA32D32_WRITE       = sis3100_class_vmeA32D32write$(ExeSuf) 

SIS3100_CLASS_VMEA32D32_READO      = sis3100_class_vmeA32D32read.$(ObjSuf) sis1100linux_vme_class.o  
SIS3100_CLASS_VMEA32D32_READS      = sis3100_class_vmeA32D32read.$(SrcSuf)
SIS3100_CLASS_VMEA32D32_READ       = sis3100_class_vmeA32D32read$(ExeSuf) 





OBJS          =  $(SIS3100_CLASS_CONTROL_READO) $(SIS3100_CLASS_CONTROL_WRITEO)  $(SIS3100_CLASS_VMEA32D32_WRITEO)  $(SIS3100_CLASS_VMEA32D32_READO) 

PROGRAMS      =  $(SIS3100_CLASS_CONTROL_READ) $(SIS3100_CLASS_CONTROL_WRITE) $(SIS3100_CLASS_VMEA32D32_WRITE) $(SIS3100_CLASS_VMEA32D32_READ)  

.SUFFIXES: .$(SrcSuf) .$(ObjSuf) .$(DllSuf)
.PHONY:    

all:            $(PROGRAMS)


$(SIS3100_CLASS_CONTROL_READ):     $(SIS3100_CLASS_CONTROL_READO)   
		$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@
		$(MT_EXE)
		@echo "$@ done1"

$(SIS3100_CLASS_CONTROL_WRITE):       $(SIS3100_CLASS_CONTROL_WRITEO) 
		$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@
		$(MT_EXE)
		@echo "$@ done1"


$(SIS3100_CLASS_VMEA32D32_READ):       $(SIS3100_CLASS_VMEA32D32_READO) 
		$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@
		$(MT_EXE)
		@echo "$@ done1"

$(SIS3100_CLASS_VMEA32D32_WRITE):       $(SIS3100_CLASS_VMEA32D32_WRITEO) 
		$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@
		$(MT_EXE)
		@echo "$@ done1"


clean:
		@rm -f $(OBJS) $(SIS3100_CLASS_CONTROL_READ) $(SIS3100_CLASS_CONTROL_WRITE) $(SIS3100_CLASS_VMEA32D32_READ) $(SIS3100_CLASS_VMEA32D32_WRITE)   

distclean:      clean
		@rm -f $(PROGRAMS) $(EVENTSO) $(EVENTLIB) *Dict.* *.def *.exp \
		   *.root *.ps *.so *.lib *.dll *.d *.log .def so_locations \
		   files/*
#		@rm -rf cxx_repository
#		-@cd RootShower && $(MAKE) distclean
#		-@cd rhtml && $(MAKE) distclean
#		-@cd RootIDE && $(MAKE) distclean


.SUFFIXES: .$(SrcSuf)


.$(SrcSuf).$(ObjSuf):
	$(CXX)  $(CXXFLAGS)   -c $<








