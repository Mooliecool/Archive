/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixsysbus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"

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
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetSystemInterruptVector)
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
    PSUPPORTED_RANGE    pRange;

    pRange = NULL;
    switch (*AddressSpace) {
        case 0:
            // verify memory address is within buses memory limits
            for (pRange = &BusHandler->BusAddresses->PrefetchMemory; pRange; pRange = pRange->Next) {
                if (BusAddress.QuadPart >= pRange->Base &&
                    BusAddress.QuadPart <= pRange->Limit) {
                        break;
                }
            }

            if (!pRange) {
                for (pRange = &BusHandler->BusAddresses->Memory; pRange; pRange = pRange->Next) {
                    if (BusAddress.QuadPart >= pRange->Base &&
                        BusAddress.QuadPart <= pRange->Limit) {
                            break;
                    }
                }
            }

            break;

        case 1:
            // verify IO address is within buses IO limits
            for (pRange = &BusHandler->BusAddresses->IO; pRange; pRange = pRange->Next) {
                if (BusAddress.QuadPart >= pRange->Base &&
                    BusAddress.QuadPart <= pRange->Limit) {
                        break;
                }
            }
            break;
    }

    if (pRange) {
        TranslatedAddress->QuadPart = BusAddress.QuadPart + pRange->SystemBase;
        *AddressSpace = pRange->SystemAddressSpace;
        return TRUE;
    }

    return FALSE;
}


ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

Arguments:

    BusInterruptLevel - Supplies the bus specific interrupt level.

    BusInterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    ULONG SystemVector;

    UNREFERENCED_PARAMETER( BusHandler );
    UNREFERENCED_PARAMETER( RootHandler );
    UNREFERENCED_PARAMETER( BusInterruptVector );

    SystemVector = BusInterruptLevel + PRIMARY_VECTOR_BASE;
    if (SystemVector < PRIMARY_VECTOR_BASE                           ||
        SystemVector > PRIMARY_VECTOR_BASE + HIGHEST_LEVEL_FOR_8259  ||
        HalpIDTUsage[SystemVector].Flags & IDTOwned ) {

        //
        // This is an illegal BusInterruptVector and cannot be connected.
        //

        return(0);
    }

    *Irql = (KIRQL)(HIGHEST_LEVEL_FOR_8259 + PRIMARY_VECTOR_BASE - SystemVector);
    *Affinity = HalpDefaultInterruptAffinity;
    ASSERT(HalpDefaultInterruptAffinity);
    return SystemVector;
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

