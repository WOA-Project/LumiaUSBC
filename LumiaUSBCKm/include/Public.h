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
	0xeca28b08, 0x54d9, 0x4c81, 0x95, 0xbd, 0xe5, 0x5c, 0xf9, 0xc6, 0xf3, 0xfd);
// {eca28b08-54d9-4c81-95bd-e55cf9c6f3fd}
