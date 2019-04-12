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

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE Device;
	UCMCONNECTOR Connector;
	LARGE_INTEGER SpiId;
	WDFIOTARGET Spi;
	BOOLEAN UseFakeSpi;
	LARGE_INTEGER FakeSpiMosiId;
	WDFIOTARGET FakeSpiMosi;
	LARGE_INTEGER FakeSpiMisoId;
	WDFIOTARGET FakeSpiMiso;
	LARGE_INTEGER FakeSpiCsId;
	WDFIOTARGET FakeSpiCs;
	LARGE_INTEGER FakeSpiClkId;
	WDFIOTARGET FakeSpiClk;
	LARGE_INTEGER VbusGpioId;
	WDFIOTARGET VbusGpio;
	LARGE_INTEGER PolGpioId;
	WDFIOTARGET PolGpio;
	LARGE_INTEGER AmselGpioId;
	WDFIOTARGET AmselGpio;
	LARGE_INTEGER EnGpioId;
	WDFIOTARGET EnGpio;
	LARGE_INTEGER ResetGpioId;
	WDFIOTARGET ResetGpio;
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
