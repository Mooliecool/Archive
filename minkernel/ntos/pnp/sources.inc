MAJORCOMP=ntos
MINORCOMP=pnp

TARGETNAME=pnp
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntospnp$(NT_UP)

GPSIZE=32

INCLUDES=..;..\..\inc;..\..\ke;$(BASE_INC_PATH)

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\pnpinit.c  \
        ..\busenum.c  \
        ..\control.c  \
        ..\pnpevent.c \
        ..\devices.c  \
        ..\internal.c \
        ..\enumsys.c  \
        ..\pnpdata.c

UMTYPE=nt
UMTEST=

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
