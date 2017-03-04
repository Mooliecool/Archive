MAJORCOMP=ntos
MINORCOMP=pnpmgr

TARGETNAME=pnpmgr
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntospnp$(NT_UP)

GPSIZE=32

INCLUDES=..;..\..\..\inc;..\..\..\ke;$(BASE_INC_PATH);$(MINWIN_INC_PATH)

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\assign.c   \
        ..\busenum.c  \
        ..\control.c  \
        ..\devices.c  \
        ..\enumsys.c  \
        ..\internal.c \
        ..\pnpdata.c  \
        ..\pnpdd.c    \
        ..\pnpevent.c \
        ..\pnpinit.c  \
        ..\pnpioapi.c \
        ..\pnpsubs.c  \
        ..\report.c   \

UMTYPE=nt
UMTEST=

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
