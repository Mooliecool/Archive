/*++ BUILD Version: 0002

Copyright (c) 2017  Microsoft Corporation
Copyright (c) 2017  OpenNT Project

Module Name:

    iop.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

--*/

#ifndef _IOPCMN_
#define _IOPCMN_

//
// Title Index to set registry key value
//

#define TITLE_INDEX_VALUE 0

NTSTATUS
IopOpenRegistryKeyPersist(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
IopInitializePlugPlayServices(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );
	
NTSTATUS
IopDriverLoadingFailed(
    IN HANDLE KeyHandle OPTIONAL,
    IN PUNICODE_STRING KeyName OPTIONAL
    );

NTSTATUS
IopPrepareDriverLoading (
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle
    );
	
	

#endif