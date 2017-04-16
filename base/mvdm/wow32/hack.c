//
// This file contains functions that are contained in wshimdb.c, but do not rely on SHIM.
// Once SHIM is implemented, wshimdb.c should be included for compilation again and this file should
// be deleted.
//

#include "precomp.h"
#pragma hdrstop

CHAR g_szCompatLayerVar[]    = "__COMPAT_LAYER";
CHAR g_szProcessHistoryVar[] = "__PROCESS_HISTORY";
CHAR g_szShimFileLogVar[]    = "SHIM_FILE_LOG";

UNICODE_STRING g_ustrProcessHistoryVar = RTL_CONSTANT_STRING(L"__PROCESS_HISTORY");
UNICODE_STRING g_ustrCompatLayerVar    = RTL_CONSTANT_STRING(L"__COMPAT_LAYER"   );
UNICODE_STRING g_ustrShimFileLogVar    = RTL_CONSTANT_STRING(L"SHIM_FILE_LOG"    );

//
// This function is called in the context of pass_environment
//
//
//


BOOL
CreateWowChildEnvInformation(
    PSZ           pszEnvParent
    )
{
    PTD         pTD; // parent TD

    WOWENVDATA  EnvData;
    PWOWENVDATA pData = NULL;
    PWOWENVDATA pEnvChildData = NULL;
    DWORD       dwLength;
    PCH         pBuffer;


    RtlZeroMemory(&EnvData, sizeof(EnvData));

    //
    // check where we should inherit process history and layers from
    //
    pTD = CURRENTPTD();

    if (pTD->pWowEnvDataChild) {
        free_w(pTD->pWowEnvDataChild);
        pTD->pWowEnvDataChild = NULL;
    }

    //
    // check whether we are starting the root task (meaning that this task IS wowexec)
    // if so, we shall inherit things from pParamBlk->envseg
    // else we use TD of the parent task (this TD that is) to
    // inherit things
    // How to detect that this is wowexec:
    // ghShellTDB could we compared to *pCurTDB
    // gptdShell->hTask16 could be compared to *pCurTDB
    // we check for not having wowexec -- if gptdShell == NULL then we're doing boot
    //

    if (pCurTDB == NULL) {
        //
        // presumably we are wowexec
        // use current environment ptr to get stuff (like pParamBlk->envseg)
        // or the original ntvdm environment
        //
        pData  = &EnvData;

        pData->pszProcessHistory = WOWFindEnvironmentVar(g_szProcessHistoryVar,
                                                         pszEnvParent,
                                                         &pData->pszProcessHistoryVal);
        pData->pszCompatLayer    = WOWFindEnvironmentVar(g_szCompatLayerVar,
                                                         pszEnvParent,
                                                         &pData->pszCompatLayerVal);
        pData->pszShimFileLog    = WOWFindEnvironmentVar(g_szShimFileLogVar,
                                                         pszEnvParent,
                                                         &pData->pszShimFileLogVal);

    } else {

        //
        // this current task is not a dastardly wowexec
        // clone current + enhance process history
        //

        pData = pTD->pWowEnvData; // if this is NULL
        if (pData == NULL) {
            pData = &EnvData; // all the vars are empty
        }

    }

    //
    //
    //
    //

    dwLength = sizeof(WOWENVDATA) +
               (NULL == pData->pszProcessHistory        ? 0 : (strlen(pData->pszProcessHistory)        + 1) * sizeof(CHAR)) +
               (NULL == pData->pszCompatLayer           ? 0 : (strlen(pData->pszCompatLayer)           + 1) * sizeof(CHAR)) +
               (NULL == pData->pszShimFileLog           ? 0 : (strlen(pData->pszShimFileLog)           + 1) * sizeof(CHAR)) +
               (NULL == pData->pszCurrentProcessHistory ? 0 : (strlen(pData->pszCurrentProcessHistory) + 1) * sizeof(CHAR));


    pEnvChildData = (PWOWENVDATA)malloc_w(dwLength);

    if (pEnvChildData == NULL) {
        return FALSE;
    }

    RtlZeroMemory(pEnvChildData, dwLength);

    //
    // now this entry has to be setup
    // process history is first
    //

    pBuffer = (PCH)(pEnvChildData + 1);

    if (pData->pszProcessHistory != NULL) {

        //
        // Copy process history. The processHistoryVal is a pointer into the buffer
        // pointed to by pszProcessHistory: __PROCESS_HISTORY=c:\foo;c:\docs~1\install
        // then pszProcessHistoryVal will point here ---------^
        //
        // we are copying the data and moving the pointer using the calculated offset

        pEnvChildData->pszProcessHistory = pBuffer;
        strcpy(pEnvChildData->pszProcessHistory, pData->pszProcessHistory);
        pEnvChildData->pszProcessHistoryVal = pEnvChildData->pszProcessHistory +
                                                 (INT)(pData->pszProcessHistoryVal - pData->pszProcessHistory);
        //
        // There is enough space in the buffer to accomodate all the strings, so
        // move pointer past current string to point at the "empty" space
        //

        pBuffer += strlen(pData->pszProcessHistory) + 1;
    }

    if (pData->pszCompatLayer != NULL) {
        pEnvChildData->pszCompatLayer = pBuffer;
        strcpy(pEnvChildData->pszCompatLayer, pData->pszCompatLayer);
        pEnvChildData->pszCompatLayerVal = pEnvChildData->pszCompatLayer +
                                              (INT)(pData->pszCompatLayerVal - pData->pszCompatLayer);
        pBuffer += strlen(pData->pszCompatLayer) + 1;
    }

    if (pData->pszShimFileLog != NULL) {
        pEnvChildData->pszShimFileLog = pBuffer;
        strcpy(pEnvChildData->pszShimFileLog, pData->pszShimFileLog);
        pEnvChildData->pszShimFileLogVal = pEnvChildData->pszShimFileLog +
                                              (INT)(pData->pszShimFileLogVal - pData->pszShimFileLog);
        pBuffer += strlen(pData->pszShimFileLog) + 1;
    }

    if (pData->pszCurrentProcessHistory != NULL) {
        //
        // Now process history
        //
        pEnvChildData->pszCurrentProcessHistory = pBuffer;

        if (pData->pszCurrentProcessHistory != NULL) {
            strcpy(pEnvChildData->pszCurrentProcessHistory, pData->pszCurrentProcessHistory);
        }

    }

    //
    // we are done, environment cloned
    //

    pTD->pWowEnvDataChild = pEnvChildData;

    return TRUE;
}

//
// In : pointer to environment(oem)
// out: pointer to unicode environment
//

NTSTATUS
WOWCloneEnvironment(
    LPVOID* ppEnvOut,
    PSZ     lpEnvironment
    )
{
    NTSTATUS Status    = STATUS_INVALID_PARAMETER;
    DWORD    dwEnvSize = 0;
    LPVOID   lpEnvNew  = NULL;

    MEMORY_BASIC_INFORMATION MemoryInformation;

    if (NULL == lpEnvironment) {
        Status = RtlCreateEnvironment(TRUE, &lpEnvNew);
    } else {
        dwEnvSize = WOWGetEnvironmentSize(lpEnvironment, NULL);

        MemoryInformation.RegionSize = (dwEnvSize + 2) * sizeof(UNICODE_NULL);
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         &lpEnvNew,
                                         0,
                                         &MemoryInformation.RegionSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
    }

    if (NULL != lpEnvironment) {

        UNICODE_STRING UnicodeBuffer;
        OEM_STRING     OemBuffer;

        OemBuffer.Buffer = (CHAR*)lpEnvironment;
        OemBuffer.Length = OemBuffer.MaximumLength = (USHORT)dwEnvSize; // size in bytes = size in chars, includes \0\0

        UnicodeBuffer.Buffer        = (WCHAR*)lpEnvNew;
        UnicodeBuffer.Length        = (USHORT)dwEnvSize * sizeof(UNICODE_NULL);
        UnicodeBuffer.MaximumLength = (USHORT)(dwEnvSize + 2) * sizeof(UNICODE_NULL); // leave room for \0

        Status = RtlOemStringToUnicodeString(&UnicodeBuffer, &OemBuffer, FALSE);
    }

    if (NT_SUCCESS(Status)) {
        *ppEnvOut = lpEnvNew;
    } else {
        if (NULL != lpEnvNew) {
            RtlDestroyEnvironment(lpEnvNew);
        }
    }

    return Status;
}

NTSTATUS
WOWFreeUnicodeEnvironment(
    LPVOID lpEnvironment
    )
{
    NTSTATUS Status;

    Status = RtlDestroyEnvironment(lpEnvironment);

    return Status;
}

//
// Call this function to produce a "good" unicode environment
//
//


LPWSTR
WOWForgeUnicodeEnvironment(
    PSZ pEnvironment,     // this task's sanitized environment
    PWOWENVDATA pEnvData    // parent-made environment data
    )
{
    NTSTATUS Status;
    LPVOID   lpEnvNew = NULL;

    DWORD    dwProcessHistoryLength = 0;
    PSZ      pszFullProcessHistory = NULL;


    Status = WOWCloneEnvironment(&lpEnvNew, pEnvironment);
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    //
    // we do have an env to work with
    //
    RtlSetEnvironmentVariable(&lpEnvNew, &g_ustrProcessHistoryVar, NULL);
    RtlSetEnvironmentVariable(&lpEnvNew, &g_ustrCompatLayerVar,    NULL);
    RtlSetEnvironmentVariable(&lpEnvNew, &g_ustrShimFileLogVar,    NULL);

    //
    // get stuff from envdata
    //

    if (pEnvData == NULL) {
        goto Done;
    }

    if (pEnvData->pszProcessHistory != NULL || pEnvData->pszCurrentProcessHistory != NULL) {

        //
        // Convert the process history which consists of 2 strings.
        //
        // The length is the existing process history length + 1 (for ';') +
        // new process history length + 1 (for '\0')
        //
        dwProcessHistoryLength = ((pEnvData->pszProcessHistory        == NULL) ? 0 : (strlen(pEnvData->pszProcessHistoryVal) + 1)) +
                                 ((pEnvData->pszCurrentProcessHistory == NULL) ? 0 :  strlen(pEnvData->pszCurrentProcessHistory)) + 1;

        //
        // Allocate process history buffer and convert it, allocating resulting unicode string.
        //
        pszFullProcessHistory = (PCHAR)malloc_w(dwProcessHistoryLength);

        if (NULL == pszFullProcessHistory) {
            Status = STATUS_NO_MEMORY;
            goto Done;
        }

        *pszFullProcessHistory = '\0';

        if (pEnvData->pszProcessHistory != NULL) {
            strcpy(pszFullProcessHistory, pEnvData->pszProcessHistoryVal);
        }

        if (pEnvData->pszCurrentProcessHistory != NULL) {

            //
            // Append ';' if the string was not empty.
            //
            if (*pszFullProcessHistory) {
                strcat(pszFullProcessHistory, ";");
            }

            strcat(pszFullProcessHistory, pEnvData->pszCurrentProcessHistory);
        }

        Status = WOWSetEnvironmentVar_Oem(&lpEnvNew,
                                          &g_ustrProcessHistoryVar,
                                          pszFullProcessHistory);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

    }

    //
    // deal with compatLayer
    //
    if (pEnvData->pszCompatLayerVal != NULL) {

        Status = WOWSetEnvironmentVar_Oem(&lpEnvNew,
                                          &g_ustrCompatLayerVar,
                                          pEnvData->pszCompatLayerVal);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

    }

    if (pEnvData->pszShimFileLog != NULL) {
        Status = WOWSetEnvironmentVar_Oem(&lpEnvNew,
                                          &g_ustrShimFileLogVar,
                                          pEnvData->pszShimFileLogVal);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }
    }




Done:

    if (pszFullProcessHistory != NULL) {
        free_w(pszFullProcessHistory);
    }


    if (!NT_SUCCESS(Status) && lpEnvNew != NULL) {
        //
        // This points to the cloned environment ALWAYS.
        //
        RtlDestroyEnvironment(lpEnvNew);
        lpEnvNew = NULL;
    }

    return(LPWSTR)lpEnvNew;
}

NTSTATUS
WOWSetEnvironmentVar_Oem(
    LPVOID*         ppEnvironment,
    PUNICODE_STRING pustrVarName,     // pre-made (cheap)
    PSZ             pszVarValue
    )
{
    OEM_STRING OemString = { 0 };
    UNICODE_STRING ustrVarValue = { 0 };
    NTSTATUS Status;

    if (pszVarValue != NULL) {
        RtlInitString(&OemString, pszVarValue);

        Status = RtlOemStringToUnicodeString(&ustrVarValue, &OemString, TRUE);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    Status = RtlSetEnvironmentVariable(ppEnvironment,
                                       pustrVarName,
                                       (NULL == pszVarValue) ? NULL : &ustrVarValue);

    if (NULL != pszVarValue) {
        RtlFreeUnicodeString(&ustrVarValue);
    }

    return Status;
}

PFLAGINFOBITS CheckFlagInfo(DWORD FlagType, DWORD dwFlag) {
PFLAGINFOBITS pFlagInfoBits;
      switch(FlagType){
          case WOWCOMPATFLAGSEX:
               pFlagInfoBits = CURRENTPTD()->pWOWCompatFlagsEx_Info;
               break;
          case WOWCOMPATFLAGS2:
               pFlagInfoBits = CURRENTPTD()->pWOWCompatFlags2_Info;
               break;
          default:
               //WOW32ASSERTMSG((FALSE), ("CheckFlagInfo called with invalid FlagType!"));
               return NULL;
      }
      while(pFlagInfoBits) {
            if(pFlagInfoBits->dwFlag == dwFlag) {
               return pFlagInfoBits;
            }
            pFlagInfoBits = pFlagInfoBits->pNextFlagInfoBits;
      }
      return NULL;
}
