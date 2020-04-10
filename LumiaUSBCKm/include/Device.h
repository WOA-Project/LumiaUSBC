/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

DEFINE_GUID(
    PowerControlGuid, 0x9942B45EL, 0x2C94, 0x41F3, 0xA1, 0x5C, 0xC1, 0xA5, 0x91,
    0xC7, 4, 0x69);

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT {
  WDFDEVICE Device;

  LARGE_INTEGER SpiId;
  WDFIOTARGET   Spi;

  WDFINTERRUPT PlugDetectInterrupt;
  WDFINTERRUPT Uc120Interrupt;
  WDFINTERRUPT PmicInterrupt1;
  WDFINTERRUPT PmicInterrupt2;

  WDFWAITLOCK   DeviceWaitLock;
  WDFCOLLECTION DevicePendingIoReqCollection;

  WDFQUEUE DefaultQueue;
  WDFQUEUE DeviceIoQueue;
  WDFQUEUE PdMessageInQueue;
  WDFQUEUE PdMessageOutQueue;

  USHORT PdStateMachineIndex;
  UCHAR  State3;
  UCHAR  State9;
  UCHAR  State0;
  UCHAR  PowerSource;
  UCHAR  Polarity;
  CHAR   IncomingPdMessageState;
  KEVENT PdEvent;

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

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);

#define UC120_CALIBRATIONFILE_SIZE 11

EVT_WDF_DEVICE_PREPARE_HARDWARE    LumiaUSBCDevicePrepareHardware;
EVT_WDF_DEVICE_D0_ENTRY            LumiaUSBCDeviceD0Entry;
EVT_WDF_INTERRUPT_ISR              EvtPlugDetInterruptIsr;
EVT_WDF_INTERRUPT_ISR              EvtUc120InterruptIsr;
EVT_WDF_INTERRUPT_ISR              EvtPmicInterrupt1Isr;
EVT_WDF_INTERRUPT_ISR              EvtPmicInterrupt2Isr;
EVT_WDF_INTERRUPT_WORKITEM         PmicInterrupt1WorkItem;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtUc120Ioctl;
EVT_WDF_IO_QUEUE_IO_WRITE          EvtPdQueueWrite;

NTSTATUS LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);

NTSTATUS
Uc120InterruptEnable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice);

NTSTATUS
Uc120InterruptDisable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice);

NTSTATUS LumiaUSBCOpenIOTarget(
    PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use,
    WDFIOTARGET *target);

NTSTATUS LumiaUSBCProbeResources(
    PDEVICE_CONTEXT DeviceContext, WDFCMRESLIST ResourcesTranslated,
    WDFCMRESLIST ResourcesRaw);

NTSTATUS LumiaUSBCOpenResources(PDEVICE_CONTEXT ctx);

NTSTATUS LumiaUSBCInitializeIoQueue(WDFDEVICE Device);

NTSTATUS
Uc120_Ioctl_EnableGoodCRC(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

NTSTATUS
Uc120_Ioctl_ExecuteHardReset(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

NTSTATUS
Uc120_Ioctl_IsCableConnected(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

NTSTATUS Uc120_Ioctl_SetVConnRoleSwitch(
    PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

NTSTATUS Uc120_Ioctl_ReportNewPowerRole(
    PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

NTSTATUS Uc120_Ioctl_ReportNewDataRole(
    PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

NTSTATUS Uc120_Ioctl_ServeOther(
    PDEVICE_CONTEXT DeviceContext, int Flag, int State0, int State1, int Role,
    int Polarity);

#define AFFINITY_MASK(n) ((ULONG_PTR)1 << (n))

EXTERN_C_END
