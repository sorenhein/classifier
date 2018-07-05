# --------------------- INFORMATION --------------------------------

# This the Makefile for Windows and the Microsoft Visual C++
# compiler.  It assumes a Unix-like setup for some commands.

INCL_SOURCE	= Makefiles/sources.txt
INCL_DEPENDS	= Makefiles/depends_obj.txt

# If your Microsoft compiler is not called cl, change it here.
CC		= cl
LINK		= link

# We compile with aggressive warnings, but we have to turn off some
# of them as they appear in Windows libraries in great numbers...

WARN_FLAGS	=		\
	/Wall			\
	/wd4365			\
	/wd4464			\
	/wd4514			\
	/wd4555			\
	/wd4571			\
	/wd4623			\
	/wd4625			\
	/wd4626			\
	/wd4668			\
	/wd4710			\
	/wd4711			\
	/wd4774			\
	/wd4820			\
	/wd4996			\
	/wd5026			\
	/wd5027			\
	/wd5045			\
	/WX

# If you don't have /std:c++17, you won't get THR_STLIMPL.

COMPILE_FLAGS	= /O2 /Oi /Ot /Oy /Ox /GL /EHs /std:c++17 $(WARN_FLAGS)

LINK_FLAGS1	= /LTCG

TARGET		= driver

include $(INCL_SOURCE)

OBJ_FILES 	= $(subst .cpp,.obj,$(SOURCE_FILES))

vs:	$(OBJ_FILES)
	$(LINK) $(LINK_FLAGS1) $(OBJ_FILES) /OUT:$(TARGET).exe

%.obj:	%.cpp
	$(CC) $(COMPILE_FLAGS) /c $< /Fo$@

depend:
	makedepend -Y -o.obj -- $(SOURCE_FILES)

clean:
	rm -f $(OBJ_FILES) $(DLL) $(DLLBASE).{lib,exp,def,obj,res}

# If you don't have a Linux-like setup, use "del" instead of "rm"
# and "copy" instead of "cp".

include $(INCL_DEPENDS)

# DO NOT DELETE
