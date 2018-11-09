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

COMPILE_FLAGS	= /O2 /Oi /Ot /Oy /Ox /GL /EHs /Zi /std:c++17 $(WARN_FLAGS)

LINK_FLAGS1	= /LTCG /DEBUG

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

Align.obj: Align.h struct.h Database.h Clusters.h Timers.h Timer.h print.h
CarGeom.obj: CarGeom.h
Clusters.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h Clusters.h
Clusters.obj: struct.h Database.h
CompStats.obj: CompStats.h
Database.obj: Database.h Clusters.h struct.h read.h
Disturb.obj: Disturb.h read.h struct.h
Except.obj: Except.h errors.h
Peak.obj: Peak.h struct.h Except.h errors.h
PeakDetect.obj: PeakDetect.h Peak.h struct.h PeakStats.h Except.h errors.h
PeakStats.obj: PeakStats.h struct.h Except.h errors.h
Regress.obj: Regress.h struct.h Database.h Clusters.h Timers.h Timer.h
Regress.obj: Except.h errors.h print.h regress/PolynomialRegression.h
SegActive.obj: SegActive.h Median/median.h PeakDetect.h Peak.h struct.h
SegActive.obj: Timers.h Timer.h write.h
SegQuiet.obj: SegQuiet.h struct.h write.h
SegTransient.obj: SegTransient.h struct.h Trace.h SegQuiet.h SegActive.h
SegTransient.obj: Median/median.h PeakDetect.h Peak.h write.h
Stats.obj: Stats.h StatCross.h StatEntry.h
StatEntry.obj: StatEntry.h
StatCross.obj: StatCross.h
SynthTrain.obj: SynthTrain.h struct.h Disturb.h
Timer.obj: Timer.h
Timers.obj: Timers.h Timer.h
Trace.obj: Trace.h SegTransient.h struct.h SegQuiet.h SegActive.h
Trace.obj: Median/median.h PeakDetect.h Peak.h Timers.h Timer.h read.h
TraceDB.obj: TraceDB.h struct.h Database.h Clusters.h Align.h Except.h
TraceDB.obj: errors.h read.h
args.obj: args.h struct.h
read.obj: Database.h Clusters.h struct.h TraceDB.h read.h
print.obj: print.h Database.h Clusters.h struct.h
write.obj: write.h struct.h
driver.obj: args.h struct.h read.h Trace.h SegTransient.h SegQuiet.h
driver.obj: SegActive.h Median/median.h PeakDetect.h Peak.h Database.h
driver.obj: Clusters.h TraceDB.h SynthTrain.h Regress.h Disturb.h Align.h
driver.obj: Timers.h Timer.h Stats.h StatCross.h StatEntry.h CompStats.h
driver.obj: PeakStats.h Except.h errors.h print.h
Ckmeans/Ckmeans.1d.dp.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h
Ckmeans/dynamic_prog.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h
Ckmeans/fill_log_linear.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h
Ckmeans/fill_quadratic.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h
Ckmeans/fill_SMAWK.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h
Ckmeans/select_levels.obj: Ckmeans/Ckmeans.1d.dp.h Ckmeans/within_cluster.h
Ckmeans/weighted_select_levels.obj: Ckmeans/Ckmeans.1d.dp.h
Ckmeans/weighted_select_levels.obj: Ckmeans/within_cluster.h
Median/median.obj: Median/median.h
regress/PolynomialRegression.obj: regress/PolynomialRegression.h
