/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

#include "public.h"
#include <UcmCx.h>

EXTERN_C_START

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE Device;
	UCMCONNECTOR Connector;
	LARGE_INTEGER SpiId;
	LARGE_INTEGER VbusGpioId;
	LARGE_INTEGER PolGpioId;
	LARGE_INTEGER AmselGpioId;
	LARGE_INTEGER EnGpioId;
	LARGE_INTEGER ResetGpioId;
	BOOLEAN HaveResetGpio;
	WDFINTERRUPT PlugDetectInterrupt;
	WDFINTERRUPT Uc120Interrupt;
	WDFINTERRUPT MysteryInterrupt1;
	WDFINTERRUPT MysteryInterrupt2;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

typedef struct _CONNECTOR_CONTEXT
{
	int dummy;

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
