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
#include <Uc120GPIO.h>
#include <WorkItems.h>

#include "Interrupt.tmh"

BOOLEAN EvtUc120InterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  WDFDEVICE       Device;
  NTSTATUS        Status;
  PDEVICE_CONTEXT pDeviceContext;

  Device = WdfInterruptGetDevice(Interrupt);
  pDeviceContext = DeviceGetContext(Device);

  WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  Status = ReadRegister(
      pDeviceContext, 2, &pDeviceContext->Register2,
      sizeof(pDeviceContext->Register2));

  if (!NT_SUCCESS(Status)) {
    pDeviceContext->Register2 = 0xFF;
  }
  else {
    UC120_HandleInterrupt(pDeviceContext);
  }

  // EOI
  WriteRegister(
      pDeviceContext, 2, &pDeviceContext->Register2,
      sizeof(pDeviceContext->Register2));
  WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);

  return TRUE;
}

BOOLEAN EvtPlugDetInterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);
  UNREFERENCED_PARAMETER(Interrupt);

  // It actually does nothing
  return TRUE;
}

BOOLEAN EvtPmicInterrupt1Isr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  WdfInterruptQueueWorkItemForIsr(Interrupt);
  return TRUE;
}

BOOLEAN EvtPmicInterrupt2Isr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  NTSTATUS Status = STATUS_SUCCESS;
  UCHAR    Reg5   = 0;

  WDFDEVICE Device = WdfInterruptGetDevice(Interrupt);
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  ASSERT(NT_SUCCESS(Status));

  Status = ReadRegister(pDeviceContext, 5, &Reg5, sizeof(Reg5));
  if (NT_SUCCESS(Status)) {
    pDeviceContext->Register5 &= 0xbf;
    Status = WriteRegister(
        pDeviceContext, 5, &pDeviceContext->Register5,
        sizeof(pDeviceContext->Register5));

    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);

    pDeviceContext->PdStateMachineIndex = 7;
    pDeviceContext->State3              = 4;
    pDeviceContext->IncomingPdHandled   = TRUE;
    pDeviceContext->Polarity            = 0;
    pDeviceContext->PowerSource         = 2;

    UC120_ToggleReg4Bit1(pDeviceContext, TRUE);
  }
  else {
    WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
  }
  
  return TRUE;
}
