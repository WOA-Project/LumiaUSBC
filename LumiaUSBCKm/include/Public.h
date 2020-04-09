/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID(GUID_DEVINTERFACE_LumiaUSBCKm,
	0x444a1011, 0xe9c2, 0x4546, 0xae, 0x9f, 0xb8, 0xaa, 0x36, 0xad, 0xd9, 0x61);
// {444a1011-e9c2-4546-ae9fb8aa36add961}

