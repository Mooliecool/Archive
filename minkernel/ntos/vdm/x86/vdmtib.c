/*

Copyright (c) 2017  Microsoft Corporation
Copyright (c) 2017  OpenNT Project

Module name:

    vdmtib.c
    
*/

#include "vdmp.h"

NTSTATUS
VdmpGetVdmTib(
   PVDM_TIB *ppVdmTib,
   ULONG dwFlags
   )
{
    PVDM_TIB currentTib;
    
    currentTib = NtCurrentTeb()->Vdm;
    if (currentTib != NULL) {
        if (currentTib->Size != sizeof(VDM_TIB))
            return STATUS_INVALID_SYSTEM_SERVICE;
    } else {
        return STATUS_INVALID_SYSTEM_SERVICE;
    }
    
    // set the Tib to the output
    currentTib = ppVdmTib;
    
    return STATUS_SUCCESS;
}
