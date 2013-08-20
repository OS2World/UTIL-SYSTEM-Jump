#$#
#$#  This is the makefile for JUMP 1.0 by Thomas Opheys
#$#  Invoke make with one of the following options:
#$#
#$#  ibm       IBM C Set compiler
#$#  ibmd      IBM C Set compiler, debugging info
#$#  ibmdllw   IBM C Set compiler, DLL, WARP v3 EXEPACK
#$#  ibmwarp   IBM C Set compiler, WARP v3 EXEPACK
#$#  watcom    Watcom C compiler
#$#  emx       emx-gcc, a.out, EMX-DLL
#$#  emxs      emx-gcc, LINK386, EMX-DLLs, WARP EXEPACK
#$#  emxomf    emx-gcc, LINK386, EMX-DLL
#$#  emxomfs   emx-gcc, LINK386, stand-alone
#$#  clean     delete all temporary files
#$#

ALLCFLAGS=-DPRELOAD

.c.obj:
          $(CC) $(CFLAGS) $(ALLCFLAGS) -c $*.c

.c.o:
          $(CC) $(CFLAGS) $(ALLCFLAGS) -c $*.c

default:
          @find "#$$#" <makefile

ibm:
          @$(MAKE)  CC=icc \
                   "CFLAGS=-O -Rn -G4 -Gi -Gs -Oi-" \
                    LFLAGS= \
                    LINK386=/BAT \
                    OBJ=obj \
                   "ICC=-q -Si -Ss" \
                    DEF=jump.def \
                    jump.exe

ibmdllw:
          @$(MAKE)  CC=icc \
                   "CFLAGS=-O -Rn -G4 -Gi -Gs -Oi- -Gd" \
                    LFLAGS=-Gd \
                    LINK386=/BAT/EXEPACK:2 \
                    OBJ=obj \
                   "ICC=-q -Si -Ss" \
                    DEF=jump.def \
                    jump.exe

ibmd:
          @$(MAKE)  CC=icc \
                   "CFLAGS=-O -Ti -Rn -G4 -Gi -Gs -Oi-" \
                    LFLAGS=-Ti \
                    LINK386=/BAT \
                    OBJ=obj \
                   "ICC=-q -Si -Ss" \
                    DEF=jump.def \
                    jump.exe

ibmwarp:
          @$(MAKE)  CC=icc \
                   "CFLAGS=-O -Rn -G4 -Gi -Gs -Oi-" \
                    LFLAGS= \
                    LINK386=/BAT/EXEPACK:2 \
                    OBJ=obj \
                   "ICC=-q -Si -Ss" \
                    DEF=jump.def \
                    jump.exe

watcom:
          @$(MAKE)  CC=wcl386 \
                   "CFLAGS=-Oaxt -3r" \
                    LFLAGS=/"@jump.lnk" \
                    OBJ=obj \
                    jump.exe

emx:
          @$(MAKE)  CC=gcc \
                    CFLAGS=-O2 \
                   "LFLAGS=-o jump.exe" \
                    OBJ=o \
                    GCCLOAD=10 \
                   "GCCOPT=-pipe -ansi -W -ZC++-comments -Wall" \
                   "XOPTS=-Xlinker -s" \
                   "EMXOPT=-c -t" \
                    DEF=jump.def \
                    ALLCFLAGS= \
                    jump.exe

emxomf:
          @$(MAKE)  CC=gcc \
                    CFLAGS=-O2 \
                   "LFLAGS=-o jump.exe" \
                    OBJ=obj \
                    GCCLOAD=10 \
                   "GCCOPT=-Zomf -pipe -ansi -W \
                    -ZC++-comments -Wall" \
                   "XOPTS=-Xlinker -s" \
                   "EMXOPT=-c -t" \
                    DEF=jump.def \
                    jump.exe

emxomfs:
          @$(MAKE)  CC=gcc \
                    CFLAGS=-O2 \
                   "LFLAGS=-o jump.exe" \
                    OBJ=obj \
                    GCCLOAD=10 \
                   "GCCOPT=-Zomf -Zsys -pipe -ansi -W \
                    -ZC++-comments -Wall" \
                   "XOPTS=-Xlinker -s" \
                   "EMXOPT=-c -t" \
                    DEF=jump.def \
                    jump.exe

emxs:
          @$(MAKE)  CC=gcc \
                    CFLAGS=-O2 \
                   "LFLAGS=-lwrap -o jump.exe" \
                    LINK386=/BAT/EXEPACK:2 \
                    OBJ=obj \
                    GCCLOAD=10 \
                   "GCCOPT=-Zomf -Zcrtdll -pipe -ansi \
                    -W -ZC++-comments -Wall" \
                   "XOPTS=-Xlinker -s" \
                   "EMXOPT=-c -t" \
                    DEF=jump.def \
                    jump.exe

jump.exe:       jump.$(OBJ)
                $(CC) $(LFLAGS) $(DEF) jump.$(OBJ) $(XOPTS)

jump.$(OBJ):    jump.c

clean:
                if exist *.o?? del *.o?? >nul
                if exist *.exe del *.exe >nul

