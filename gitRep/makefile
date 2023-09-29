CXX=g++ #compiler for C++
MPICXX=mpic++ #compiler for MPI
CXX_OPT=-ftree-vectorize -march=native -mtune=native -O3 -flto
CXX_OPT_DEBUG=-g -pg
WARN_DEFAULTS=-Wall -Wextra -Wpedantic -Wno-unused-parameter
VALFLAGS= -g -O0

#CPPFLAGS is typically used to hold compiler flags for the C preprocessor
#CFLAGS is used for C (gcc) compiler
#C compiler (gcc) uses both CPPFLAGS and CFLAGS variables
#C++ compiler (g++) only uses CPPFLAGS variables (which is why we have it here)
#-c option: compiler should only compile the source files and generate object files (.o) without performing the linking step.
#-std=c++0x option sets the C++ language standard to be used by the compiler: the C++0x (also known as C++11) language standard.
CPPFLAGS= -c \
-std=c++0x \
-I/usr/include \
-I/usr/local/include \
-I/usr/local/Cellar \
-I/data/apps/linux-centos8-cascadelake/gcc-9.2.0/gcc-9.3.0-bnvby67rgbqevwsd264rgz44xucnkhpm/lib/gcc/x86_64-pc-linux-gnu/9.3.0/include \
-I. \
${WARN_DEFAULTS}

#Linker flags are additional options and settings that are passed to the linker during the final linking phase of the build process.
LDFLAGS?= -pthread \
-std=c++0x \
-L/data/apps/linux-centos8-cascadelake/gcc-9.2.0/gcc-9.3.0-bnvby67rgbqevwsd264rgz44xucnkhpm/lib/gcc/x86_64-pc-linux-gnu/9.3.0/lib \
-L/usr/local/lib \
-lm \
-lboost_serialization \
-lboost_program_options \
-lboost_iostreams

all: Person.o Household.o Population.o Functions.o Stats.o Params.o \
Driver.o Run.o Main.o ihs.o
	$(CXX) $(CXX_OPT) Person.o Household.o Population.o Functions.o Stats.o Params.o \
	Driver.o Run.o Main.o ihs.o -o mdrtb.out $(LDFLAGS)
	
#making supervisor code
super: Params_mpi.o OpenMpiServer.o OpenMpiClient.o OpenMpiMain.o OpenMpiCommon.o
	$(MPICXX) $(CXX_OPT) Params_mpi.o OpenMpiServer.o OpenMpiClient.o OpenMpiMain.o OpenMpiCommon.o -o svc.out
	
debug: Person-debug.o Household-debug.o Population-debug.o Functions-debug.o Stats-debug.o Params-debug.o \
Driver-debug.o Run-debug.o Main-debug.o ihs-debug.o
	$(CXX) $(CXX_OPT_DEBUG) Person-debug.o Household-debug.o Population-debug.o Functions-debug.o Stats-debug.o Params-debug.o \
	Driver-debug.o Run-debug.o Main-debug.o ihs-debug.o -o mdrtb-debug.out $(LDFLAGS)

#cleaning the build folder (-rf recursively deleting all filed & forlder meeting criteria)
clean:
	@rm -rf *.o svc.out mdrtb.out


#Compiling source files into objects: X.o is the target of the rule, depending on X.cpp & x.hpp
#When you run make and the Run.o target is out-of-date (either Run.cpp or Run.hpp has been modified since the last build), the specified command will be executed to compile Run.cpp into Run.o using the compiler and flags specified by the variables CXX, CPPFLAGS, and CXX_OPT
Person.o: Person.cpp Person.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Person.cpp

Household.o: Household.cpp Household.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Household.cpp

Population.o: Population.cpp Population.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Population.cpp

Functions.o: Functions.cpp Functions.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Functions.cpp

Stats.o: Stats.cpp Stats.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Stats.cpp

Params.o: Params.cpp Params.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Params.cpp

Driver.o: Driver.cpp Driver.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Driver.cpp

Run.o: Run.cpp Run.hpp
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Run.cpp

Main.o: Main.cpp
	$(CXX) $(CPPFLAGS) $(CXX_OPT) Main.cpp

ihs.o: ihs.cpp
	$(CXX) $(CPPFLAGS) $(CXX_OPT) ihs.cpp

#Compiling source files for openMPI
Params_mpi.o: Params.cpp Params.h
	$(MPICXX) $(CPPFLAGS) $(CXX_OPT) Params.cpp -o Params_mpi.o

OpenMpiServer.o: OpenMpiServer.cpp OpenMpiServer.hpp
	$(MPICXX) $(CPPFLAGS) $(CXX_OPT) OpenMpiServer.cpp

OpenMpiClient.o: OpenMpiClient.cpp OpenMpiClient.hpp
	$(MPICXX) $(CPPFLAGS) $(CXX_OPT) OpenMpiClient.cpp
	
OpenMpiCommon.o: OpenMpiCommon.cpp OpenMpiCommon.hpp
	$(MPICXX) $(CPPFLAGS) $(CXX_OPT) OpenMpiCommon.cpp

OpenMpiMain.o: OpenMpiMain.cpp
	$(MPICXX) $(CPPFLAGS) $(CXX_OPT) OpenMpiMain.cpp

#Debug build
Person-debug.o: Person.cpp Person.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Person-debug.o Person.cpp

Household-debug.o: Household.cpp Household.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Household-debug.o Household.cpp

Population-debug.o: Population.cpp Population.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Population-debug.o Population.cpp

Functions-debug.o: Functions.cpp Functions.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Functions-debug.o Functions.cpp

Stats-debug.o: Stats.cpp Stats.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Stats-debug.o Stats.cpp

Params-debug.o: Params.cpp Params.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Params-debug.o Params.cpp

Driver-debug.o: Driver.cpp Driver.h
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Driver-debug.o Driver.cpp

Run-debug.o: Run.cpp Run.hpp
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Run-debug.o Run.cpp

Main-debug.o: Main.cpp
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o Main-debug.o Main.cpp

ihs-debug.o: ihs.cpp
	$(CXX) $(CPPFLAGS) $(CXX_OPT_DEBUG) -o ihs-debug.o ihs.cpp

