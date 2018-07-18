# --------------------- INFORMATION --------------------------------

# This the Metapron3 Makefile for Windows and the Cygwin GNU g++
# compiler.  It assumes a Unix-like setup for some commands.

INCL_SOURCE	= Makefiles/sources.txt
INCL_DEPENDS	= Makefiles/depends_o.txt

# If your compiler name is not given here, change it.
CC		= g++

# We compile with aggressive warnings, but we have to turn off some
# of them as they appear in libraries in great numbers...

WARN_FLAGS	= 		\
	-Wshadow 		\
	-Wsign-conversion 	\
	-pedantic -Wall -Wextra  \
	-Wcast-align -Wcast-qual \
	-Wctor-dtor-privacy 	\
	-Wdisabled-optimization \
	-Wformat=2 		\
	-Winit-self 		\
	-Wlogical-op 		\
	-Wmissing-declarations 	\
	-Wmissing-include-dirs 	\
	-Wnoexcept 		\
	-Wold-style-cast 	\
	-Woverloaded-virtual 	\
	-Wredundant-decls 	\
	-Wsign-promo 		\
	-Wstrict-null-sentinel 	\
	-Wstrict-overflow=1 	\
	-Wswitch-default -Wundef \
	-Werror 		\
	-Wno-unused 		\
	-Wno-unknown-pragmas 	\
	-Wno-long-long

COMPILE_FLAGS	= -O3 -flto -fopenmp -mtune=generic -fno-use-linker-plugin \
		$(WARN_FLAGS) 

LINK1_FLAGS	= -shared -fopenmp

LINK2_FLAGS	= 		\
	-Wl,--subsystem,windows \
	-Wl,--output-def,$(DLLBASE).def	\
	-Wl,--dynamicbase 	\
	-Wl,--nxcompat 		\
	-Wl,--no-seh 		\
	-Wl,--enable-stdcall-fixup 

include $(INCL_SOURCE)

O_FILES 	= $(subst .cpp,.o,$(SOURCE_FILES)) $(VFILE).o

cygwin:	$(O_FILES)
	$(CC) $(LINK1_FLAGS) $(O_FILES) \
	$(LINK2_FLAGS) $(EXPORTER) -o $(DLL)

%.o:	%.cpp
	$(CC) $(COMPILE_FLAGS) -c $<

depend:
	makedepend -Y -- $(SOURCE_FILES)

clean:
	rm -f $(O_FILES) $(DLL) $(DLLBASE).{lib,def,exp,res}

# If you don't have a Linux-like setup, use "del" instead of "rm
# and "copy" instead of "cp".

include $(INCL_DEPENDS)

