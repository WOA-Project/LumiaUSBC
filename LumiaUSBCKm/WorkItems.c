#include <Driver.h>
#include <SPI.h>
#include <UC120.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "WorkItems.tmh"
#endif

#include <UC120Registers.h>

void PmicInterrupt1WorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject)
{
  UNREFERENCED_PARAMETER(AssociatedObject);

  NTSTATUS Status = STATUS_SUCCESS;

  WDFDEVICE       Device         = WdfInterruptGetDevice(Interrupt);
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "PMIC1 Interrupt Begin");

  Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  ASSERT(NT_SUCCESS(Status));

  pDeviceContext->Register5 |= 0x40;
  Status = WriteRegister(
      pDeviceContext, 5, &pDeviceContext->Register5,
      sizeof(pDeviceContext->Register5));

  WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);

  // Trace
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
      "PMIC1 EOI: PdStateMachineIndex = %u, IncomingPdHandled = %!bool!, "
      "PowerSource = %u, "
      "State3 = %u, State9 = %u, Polarity = %u, IncomingPdMessageState = %u",
      pDeviceContext->PdStateMachineIndex, pDeviceContext->State0,
      pDeviceContext->PowerSource, pDeviceContext->State3,
      pDeviceContext->State9, pDeviceContext->Polarity,
      pDeviceContext->IncomingPdMessageState);
}
