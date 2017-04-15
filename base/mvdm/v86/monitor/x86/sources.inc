INCLUDES=$(INCLUDES); \
         $(WINDOWS_INC_PATH);

X86_SOURCES=x86\monitor.c  \
            x86\fastpm.asm \
            x86\sas.c      \
            x86\spcstubs.c \
            x86\thread.c   \
            x86\vdpm.c     \
            x86\int.c
!IF 0
Note: to build with profiling support, add the following to the i386_sources

             x86\proflib.c \
!ENDIF
