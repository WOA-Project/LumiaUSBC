/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"
#include <UcmCx.h>

EXTERN_C_START

DEFINE_GUID(PowerControlGuid, 0x9942B45EL, 0x2C94, 0x41F3, 0xA1, 0x5C, 0xC1, 0xA5, 0x91, 0xC7, 4, 0x69);

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE Device;
	UCMCONNECTOR Connector;
	POHANDLE PoHandle;
	LARGE_INTEGER SpiId;
	WDFIOTARGET Spi;
	LARGE_INTEGER VbusGpioId;
	WDFIOTARGET VbusGpio;
	LARGE_INTEGER PolGpioId;
	WDFIOTARGET PolGpio;
	LARGE_INTEGER AmselGpioId;
	WDFIOTARGET AmselGpio;
	LARGE_INTEGER EnGpioId;
	WDFIOTARGET EnGpio;
	WDFINTERRUPT PlugDetectInterrupt;
	WDFINTERRUPT Uc120Interrupt;
	WDFINTERRUPT MysteryInterrupt1;
	WDFINTERRUPT MysteryInterrupt2;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

typedef struct _CONNECTOR_CONTEXT
{
	UCM_TYPEC_PARTNER partner;

} CONNECTOR_CONTEXT, *PCONNECTOR_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONNECTOR_CONTEXT, ConnectorGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
LumiaUSBCKmCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EXTERN_C_END
