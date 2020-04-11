#include <Driver.h>
#include <SPI.h>
#include <UC120.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "Queue.tmh"
#endif

NTSTATUS LumiaUSBCInitializeIoQueue(WDFDEVICE Device)
{
  PDEVICE_CONTEXT     pDeviceContext = DeviceGetContext(Device);
  WDF_IO_QUEUE_CONFIG QueueConfig;
  NTSTATUS            Status = STATUS_SUCCESS;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCInitializeIoQueue Enter");

  WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchSequential);
  QueueConfig.DefaultQueue       = TRUE;
  QueueConfig.EvtIoDeviceControl = EvtUc120Ioctl;

  Status = WdfIoQueueCreate(
      Device, &QueueConfig, NULL, &pDeviceContext->DefaultQueue);
  if (!NT_SUCCESS(Status)) {
    goto Exit;
  }

  WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchManual);
  QueueConfig.PowerManaged = WdfFalse;

  Status = WdfIoQueueCreate(
      Device, &QueueConfig, NULL, &pDeviceContext->DeviceIoQueue);
  if (!NT_SUCCESS(Status)) {
    goto Exit;
  }

  WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchManual);
  QueueConfig.PowerManaged = WdfFalse;

  Status = WdfIoQueueCreate(
      Device, &QueueConfig, NULL, &pDeviceContext->PdMessageInQueue);
  if (!NT_SUCCESS(Status)) {
    goto Exit;
  }

  WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchSequential);
  QueueConfig.EvtIoWrite = EvtPdQueueWrite;

  Status = WdfIoQueueCreate(
      Device, &QueueConfig, NULL, &pDeviceContext->PdMessageOutQueue);
  if (!NT_SUCCESS(Status)) {
    goto Exit;
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCInitializeIoQueue Exit: %!STATUS!", Status);

  return Status;
}

VOID EvtUc120Ioctl(
    _In_ WDFQUEUE Queue, _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength, _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
  UNREFERENCED_PARAMETER(InputBufferLength);
  UNREFERENCED_PARAMETER(OutputBufferLength);

  WDFDEVICE       Device;
  PDEVICE_CONTEXT pDeviceContext;
  NTSTATUS        Status;

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtUc120Ioctl Enter");

  Device         = WdfIoQueueGetDevice(Queue);
  pDeviceContext = DeviceGetContext(Device);

  if (IoControlCode != 0x22c006) {
    Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
    ASSERT(NT_SUCCESS(Status));
  }

  switch (IoControlCode) {
  case 0x22c019:
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtUc120Ioctl - EnableGoodCRC");
    Uc120_Ioctl_EnableGoodCRC(pDeviceContext, Request);
    break;
  case 0x22c01c:
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - ExecuteHardReset");
    Uc120_Ioctl_ExecuteHardReset(pDeviceContext, Request);
    break;
  case 0x22c015:
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - SetVConnRoleSwitch");
    Uc120_Ioctl_SetVConnRoleSwitch(pDeviceContext, Request);
    break;
  case 0x22c00a:
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - IsCableConnected");
    Uc120_Ioctl_IsCableConnected(pDeviceContext, Request);
    break;
  case 0x22c00d:
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - ReportNewDataRole");
    Uc120_Ioctl_ReportNewDataRole(pDeviceContext, Request);
    break;
  case 0x22c011:
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - ReportNewPowerRole");
    Uc120_Ioctl_ReportNewPowerRole(pDeviceContext, Request);
    break;
  case 0x22c006:
  default:
    // No need to handle here
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - 0x22c006");
    // Forwarded
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtUc120Ioctl - Forward IOCTL 0x%x", IoControlCode);
    Status = WdfRequestForwardToIoQueue(Request, pDeviceContext->DeviceIoQueue);
    if (!NT_SUCCESS(Status)) {
      WdfRequestComplete(Request, Status);
    }
    break;
  }

  if (IoControlCode != 0x22c006) {
    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
  }
  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtUc120Ioctl Exit");
}

VOID EvtPdQueueWrite(
    _In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
  WDFDEVICE Device;
  PDEVICE_CONTEXT pDeviceContext;
  NTSTATUS        Status;

  UCHAR *RegVal;
  size_t RegSize = Length;

  UCHAR Reg0Val = (UCHAR) Length;

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtPdQueueWrite Enter");

  Device = WdfIoQueueGetDevice(Queue);
  pDeviceContext = DeviceGetContext(Device);

  if (!Length) {
    WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtPdQueueWrite failed - STATUS_INVALID_PARAMETER: Length is invalid");
    return;
  }

  pDeviceContext->IncomingPdMessageState = -1;
  KeClearEvent(&pDeviceContext->PdEvent);

  Status = WdfRequestRetrieveInputBuffer(Request, Length, &RegVal, &RegSize);
  if (!NT_SUCCESS(Status)) {
    WdfRequestComplete(Request, Status);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtPdQueueWrite failed - WdfRequestRetrieveInputBuffer %!STATUS!", Status);
    return;
  }

  Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  ASSERT(NT_SUCCESS(Status));

  Status = WriteRegister(pDeviceContext, 16, RegVal, (ULONG) RegSize);
  if (!NT_SUCCESS(Status)) {
    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
    WdfRequestComplete(Request, Status);
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtPdQueueWrite failed - WriteRegister R16 %!STATUS!",
        Status);
    return;
  }

  Status = WriteRegister(pDeviceContext, 0, &Reg0Val, 1);
  if (!NT_SUCCESS(Status)) {
    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
    WdfRequestComplete(Request, Status);
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtPdQueueWrite failed - WriteRegister R0 %!STATUS!", Status);
    return;
  }

  WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
  Status = KeWaitForSingleObject(&pDeviceContext->PdEvent, 0, 0, 0, 0);
  if (Status == 0x102) {
    WdfRequestComplete(Request, STATUS_TIMEOUT);
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtPdQueueWrite failed - KeWaitForSingleObject timed out");
    return;
  }

  switch (pDeviceContext->IncomingPdMessageState) {
  case 1:
    Status = STATUS_PIPE_BUSY;
    break;
  case 2:
    Status = STATUS_TIMEOUT;
    break;
  case 6:
    Status = STATUS_TRANSPORT_FULL;
    break;
  default:
    Status = STATUS_SUCCESS;
    break;
  }
  
  if (!NT_SUCCESS(Status)) {
    WdfRequestComplete(Request, Status);
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "EvtPdQueueWrite failed - PD State invalid: %!STATUS!", Status);
  }
  else {
    WdfRequestCompleteWithInformation(Request, Status, Reg0Val);
  }

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtPdQueueWrite Exit");
}
