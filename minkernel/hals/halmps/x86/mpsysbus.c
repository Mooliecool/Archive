/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    mpsysbus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"

ULONG HalpDefaultInterruptAffinity;

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

extern UCHAR HalpVectorToIRQL[];
extern UCHAR HalpIRQLtoTPR[];
extern UCHAR HalpVectorToINTI[];
extern KSPIN_LOCK HalpAccountingLock;

UCHAR HalpINTItoVector[16*4];
UCHAR HalpPICINTToVector[16];
ULONG HalpDefaultInterruptAffinity;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpSetInternalVector)
#pragma alloc_text(PAGELK,HalpGetSystemInterruptVector)
#endif



BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusAddress        - Supplies the bus-relative address

    AddressSpace      -  Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => memory space
                         AddressSpace == 1 => I/O space

    TranslatedAddress - Supplies a pointer to return the translated address

Return Value:

    A return value of TRUE indicates that a system physical address
    corresponding to the supplied bus relative address and bus address
    number has been returned in TranslatedAddress.

    A return value of FALSE occurs if the translation for the address was
    not possible

--*/

{
    BOOLEAN             status;
    PSUPPORTED_RANGE    pRange;

    status  = FALSE;
    switch (*AddressSpace) {
        case 0:
            // verify memory address is within buses memory limits
            pRange = &BusHandler->BusAddresses->Memory;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;
                pRange = pRange->Next;
            }

            pRange = &BusHandler->BusAddresses->PrefetchMemory;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;

                pRange = pRange->Next;
            }
            break;

        case 1:
            // verify IO address is within buses IO limits
            pRange = &BusHandler->BusAddresses->IO;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;

                pRange = pRange->Next;
            }
            break;

        default:
            status = FALSE;
            break;
    }

    if (status) {
        *TranslatedAddress = BusAddress;
    } else {
        _asm { nop };       // good for debugging
    }

    return status;
}


#define MAX_SYSTEM_IRQL     31
#define MAX_FREE_IRQL       26
#define MIN_FREE_IRQL       4
#define MAX_FREE_VECTOR     0xbf
#define MIN_FREE_VECTOR     0x51
#define VECTOR_BASE         0x50
#define MAX_VBUCKET          7

#define AllocateVectorIn(index)     \
    vBucket[index]++;               \
    ASSERT (vBucket[index] < 16);

#define GetVectorFrom(index)  \
    (ULONG) ( index*16 + VECTOR_BASE + vBucket[index] )
    // note: device levels 50,60,70,80,90,A0,B0 are not allocatable

#define GetIrqlFrom(index)  (KIRQL) ( index + MIN_FREE_IRQL )

UCHAR   vBucket[MAX_VBUCKET];

ULONG
HalpGetSystemInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

    This function returns the system interrupt vector and IRQL
    corresponding to the specified bus interrupt level and/or
    vector.  The system interrupt vector and IRQL are suitable
    for use in a subsequent call to KeInitializeInterrupt.

Arguments:

    InterruptLevel - Supplies the bus specific interrupt level.

    InterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    ULONG           SystemVector, ApicInti;
    ULONG           Bucket, i, OldLevel;
    BOOLEAN         Found;
    PVOID           LockHandle;


    UNREFERENCED_PARAMETER( InterruptVector );

    //
    // Find closest child bus to this handler
    //

    if (RootHandler != BusHandler) {
        while (RootHandler->ParentHandler != BusHandler) {
            RootHandler = RootHandler->ParentHandler;
        }
    }

    //
    // Find Interrupt's APIC Inti connection
    //

    Found = HalpGetPcMpInterruptDesc (
                RootHandler->InterfaceType,
                RootHandler->BusNumber,
                InterruptLevel,
                &ApicInti
                );

    if (!Found) {
        return 0;
    }

    //
    // On Symetric MP systems the interrupt affinity is all processors.
    //

    *Affinity = HalpDefaultInterruptAffinity;

    //
    // If device interrupt vector mapping is not already allocated,
    // then do it now
    //

    if (!HalpINTItoVector[ApicInti]) {

        //
        // Vector is not allocated - synchronize and check again
        //

        LockHandle = MmLockPagableCodeSection (&HalpGetSystemInterruptVector);
        OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);
        if (!HalpINTItoVector[ApicInti]) {

            //
            // Still not allocated, Dynamically allocate a vector
            //

            Bucket = MAX_VBUCKET-1;
            for (i = MAX_VBUCKET-1; i; i--) {
                if (vBucket[i-1] < vBucket[Bucket]) {
                    Bucket = i-1;
                }
            }

            AllocateVectorIn (Bucket);
            SystemVector = GetVectorFrom (Bucket);
            *Irql = GetIrqlFrom (Bucket);

            ASSERT(*Irql <= MAX_FREE_IRQL);
            ASSERT(SystemVector <= MAX_FREE_VECTOR);
            ASSERT(SystemVector >= MIN_FREE_VECTOR);
            ASSERT((UCHAR) (HalpIRQLtoTPR[*Irql] & 0xf0) == (UCHAR) (SystemVector & 0xf0) );

            HalpVectorToIRQL[SystemVector >> 4] = (UCHAR) *Irql;
            HalpVectorToINTI[SystemVector] = (UCHAR) ApicInti;
            HalpINTItoVector[ApicInti]     = (UCHAR) SystemVector;

            //
            // If this assigned interrupt is connected to the machines PIC,
            // then remember the PIC->SystemVector mapping.
            //

            if (RootHandler->BusNumber == 0  &&  InterruptLevel < 16  &&
                 RootHandler->InterfaceType == DEFAULT_PC_BUS) {
                HalpPICINTToVector[InterruptLevel] = (UCHAR) SystemVector;
            }

        }

        HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
        MmUnlockPagableImageSection (LockHandle);
    }

    //
    // Return this ApicInti's system vector & irql

    SystemVector = HalpINTItoVector[ApicInti];
    *Irql = HalpVectorToIRQL[SystemVector >> 4];

    ASSERT(HalpVectorToINTI[SystemVector] == (UCHAR) ApicInti);
    ASSERT(*Affinity);

    return SystemVector;
}

VOID
HalpSetInternalVector (
    IN ULONG    InternalVector,
    IN VOID   (*HalInterruptServiceRoutine)(VOID)
    )
/*++

Routine Description:

    Used at init time to set IDT vectors for internal use.

--*/
{
    //
    // Remember this vector so it's reported as Hal internal usage
    //

    HalpRegisterVector (
        InternalUsage,
        InternalVector,
        InternalVector,
        HalpVectorToIRQL[InternalVector >> 4]
    );

    //
    // Connect the IDT
    //

    KiSetHandlerAddressToIDT(InternalVector, HalInterruptServiceRoutine);
}

BOOLEAN
HalpFindBusAddressTranslation (
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    )
/*++

Routine Description:

    This routine returns the bus data of a lookup.
    
    Note:  This routine isn't called directly, rather it is 
    implemented through HALPDISPATCH. This implementation of 
    HalFindBusAddressTranslation is meant for PC/AT systems, 
    and other HALs implement their own version of this routine.
    
Arguments:

    BusAddress          Address to be translated.
    AddressSpace        0 = Memory
                        1 = IO (There are other possibilities).
                        N.B. This argument is a pointer, the value
                        will be modified if the translated address
                        is of a different address space type from
                        the untranslated bus address.
    TranslatedAddress   Pointer to where the translated address
                        should be stored.
    Context             Pointer to a ULONG_PTR. On the initial call,
                        for a given BusAddress, it should contain
                        0.  It will be modified by this routine,
                        on subsequent calls for the same BusAddress
                        the value should be handed in again,
                        unmodified by the caller.
    NextBus             FALSE if we should attempt this translation
                        on the same bus as indicated by Context,
                        TRUE if we should be looking for another
                        bus.

Return Value:

    TRUE    if translation was successful,
    FALSE   otherwise.

--*/

{
    //
    // Check to see if we have a context or there's already data
    // in the context. 
    //
    
    if (Context == NULL || *Context && NextBus == TRUE)
        return FALSE;
    
    //
    // Return the bus data to the caller.
    //
    
    TranslatedAddress->QuadPart = BusAddress.QuadPart;
    
    *Context = 1;
    return TRUE;
}
