MAJORCOMP=ntos
MINORCOMP=cache

TARGETNAME=cache
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntoscache$(NT_UP)

INCLUDES=..;..\..\inc

MIPS_OPTIONS=-nodwalign
GPSIZE=32

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\cachedat.c   \
        ..\cachesub.c   \
        ..\copysup.c    \
        ..\fssup.c      \
        ..\lazyrite.c   \
        ..\logsup.c     \
        ..\mdlsup.c     \
        ..\pinsup.c     \
        ..\vacbsup.c

PRECOMPILED_INCLUDE=..\cc.h
PRECOMPILED_PCH=cc.pch
PRECOMPILED_OBJ=cc.obj

SOURCES_USED=..\sources.inc
