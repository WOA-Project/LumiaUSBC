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

DEFINE_GUID(
    PowerControlGuid, 0x9942B45EL, 0x2C94, 0x41F3, 0xA1, 0x5C, 0xC1, 0xA5, 0x91,
    0xC7, 4, 0x69);

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT {
  WDFDEVICE     Device;
  UCMCONNECTOR  Connector;
  POHANDLE      PoHandle;
  LARGE_INTEGER SpiId;
  WDFIOTARGET   Spi;
  LARGE_INTEGER VbusGpioId;
  WDFIOTARGET   VbusGpio;
  LARGE_INTEGER PolGpioId;
  WDFIOTARGET   PolGpio;
  LARGE_INTEGER AmselGpioId;
  WDFIOTARGET   AmselGpio;
  LARGE_INTEGER EnGpioId;
  WDFIOTARGET   EnGpio;
  WDFINTERRUPT  PlugDetectInterrupt;
  WDFINTERRUPT  Uc120Interrupt;
  WDFINTERRUPT  PmicInterrupt1;
  WDFINTERRUPT  PmicInterrupt2;
  BOOLEAN       Connected;

  WDFWAITLOCK DeviceWaitLock;

  USHORT  PdStateMachineIndex;
  UCHAR   State3;
  BOOLEAN IncomingPdHandled;
  UCHAR   PowerSource;
  UCHAR   Polarity;
  UCHAR   IncomingPdMessageState;

  UCHAR Register0;
  UCHAR Register1;
  UCHAR Register2;
  UCHAR Register3;
  UCHAR Register4;
  UCHAR Register5;
  UCHAR Register6;
  UCHAR Register7;
  UCHAR Register13;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

typedef struct _CONNECTOR_CONTEXT {
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
LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);

#define UC120_CALIBRATIONFILE_SIZE 11

typedef enum _UC120_CONTEXT_STATES {
  UC120_INIT          = 0,
  UC120_PLUGDET       = 1,
  UC120_NOTIFICATION  = 2,
  UC120_MYSTERY1_VBUS = 3,
  UC120_MYSTERY2_HDMI = 4
} UC120_CONTEXT_STATES;

EVT_WDF_DEVICE_PREPARE_HARDWARE     LumiaUSBCDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     LumiaUSBCDeviceReleaseHardware;
EVT_UCM_CONNECTOR_SET_DATA_ROLE     LumiaUSBCSetDataRole;
EVT_WDF_DEVICE_D0_ENTRY             LumiaUSBCDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT              LumiaUSBCDeviceD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT LumiaUSBCSelfManagedIoInit;

NTSTATUS LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);

EVT_WDF_INTERRUPT_ISR EvtPlugDetInterruptIsr;
EVT_WDF_INTERRUPT_ISR EvtUc120InterruptIsr;
EVT_WDF_INTERRUPT_ISR EvtPmicInterrupt1Isr;
EVT_WDF_INTERRUPT_ISR EvtPmicInterrupt2Isr;

EVT_WDF_INTERRUPT_WORKITEM PmicInterrupt1WorkItem;

NTSTATUS
Uc120InterruptEnable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice);
NTSTATUS
Uc120InterruptDisable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice);

NTSTATUS OpenIOTarget(
    PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use,
    WDFIOTARGET *target);

NTSTATUS LumiaUSBCProbeResources(
    PDEVICE_CONTEXT DeviceContext, WDFCMRESLIST ResourcesTranslated,
    WDFCMRESLIST ResourcesRaw);

NTSTATUS LumiaUSBCOpenResources(PDEVICE_CONTEXT ctx);
void     LumiaUSBCCloseResources(PDEVICE_CONTEXT ctx);

NTSTATUS
LumiaUSBCSetPowerRole(UCMCONNECTOR Connector, UCM_POWER_ROLE PowerRole);

#define AFFINITY_MASK(n) ((ULONG_PTR)1 << (n))

EXTERN_C_END
