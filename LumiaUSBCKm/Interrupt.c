#include <Driver.h>

#ifndef RESHUB_USE_HELPER_ROUTINES
#define RESHUB_USE_HELPER_ROUTINES
#endif

#include <reshub.h>
#include <wdf.h>

#include <GPIO.h>
#include <Registry.h>
#include <SPI.h>
#include <UC120.h>
#include <WorkItems.h>

#include "Interrupt.tmh"

UCHAR EvtUc120InterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  WDFDEVICE       Device;
  NTSTATUS        Status;
  PDEVICE_CONTEXT pDeviceContext;

  Device         = WdfInterruptGetDevice(Interrupt);
  pDeviceContext = DeviceGetContext(Device);

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "UC120 Interrupt Begin");

  Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  ASSERT(NT_SUCCESS(Status));

  Status = ReadRegister(
      pDeviceContext, 2, &pDeviceContext->Register2,
      sizeof(pDeviceContext->Register2));

  if (NT_SUCCESS(Status)) {
    UC120_HandleInterrupt(pDeviceContext);
  }

  // EOI
  pDeviceContext->Register2 = 0xFF;
  WriteRegister(
      pDeviceContext, 2, &pDeviceContext->Register2,
      sizeof(pDeviceContext->Register2));

  // Trace
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
      "UC120 EOI: PdStateMachineIndex = %u, IncomingPdHandled = %!bool!, "
      "PowerSource = %u, "
      "State3 = %u, State9 = %u, Polarity = %u, IncomingPdMessageState = %u",
      pDeviceContext->PdStateMachineIndex, pDeviceContext->State0,
      pDeviceContext->PowerSource, pDeviceContext->State3,
      pDeviceContext->State9, pDeviceContext->Polarity,
      pDeviceContext->IncomingPdMessageState);

  WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
  return TRUE;
}

UCHAR EvtPlugDetInterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);
  UNREFERENCED_PARAMETER(Interrupt);

  // It actually does nothing
  return TRUE;
}

UCHAR EvtPmicInterrupt1Isr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  WdfInterruptQueueWorkItemForIsr(Interrupt);
  return TRUE;
}

UCHAR EvtPmicInterrupt2Isr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  NTSTATUS Status = STATUS_SUCCESS;
  UCHAR    Reg5   = 0;

  WDFDEVICE       Device         = WdfInterruptGetDevice(Interrupt);
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "PMIC2 Interrupt Begin");

  Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  ASSERT(NT_SUCCESS(Status));

  Status = ReadRegister(pDeviceContext, 5, &Reg5, sizeof(Reg5));
  if (!NT_SUCCESS(Status) || Reg5 & 0x40) {
    pDeviceContext->Register5 &= 0xbf;
    Status = WriteRegister(
        pDeviceContext, 5, &pDeviceContext->Register5,
        sizeof(pDeviceContext->Register5));

    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);

    pDeviceContext->PdStateMachineIndex = 7;
    pDeviceContext->State3              = 4;
    pDeviceContext->State0   = TRUE;
    pDeviceContext->Polarity            = 0;
    pDeviceContext->PowerSource         = 2;

    UC120_ToggleReg4Bit1(pDeviceContext, TRUE);
  }
  else {
    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
  }

  // Trace
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
      "PMIC2 EOI: PdStateMachineIndex = %u, IncomingPdHandled = %!bool!, "
      "PowerSource = %u, "
      "State3 = %u, State9 = %u, Polarity = %u, IncomingPdMessageState = %u",
      pDeviceContext->PdStateMachineIndex, pDeviceContext->State0,
      pDeviceContext->PowerSource, pDeviceContext->State3,
      pDeviceContext->State9, pDeviceContext->Polarity,
      pDeviceContext->IncomingPdMessageState);

  return TRUE;
}
