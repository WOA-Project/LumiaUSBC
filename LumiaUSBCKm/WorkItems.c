#include <Driver.h>
#include <SPI.h>
#include <UC120.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "WorkItems.tmh"
#endif

#include <UC120Registers.h>
#include <USBRole.h>

void PmicInterrupt1WorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject)
{
  UNREFERENCED_PARAMETER(AssociatedObject);

  NTSTATUS Status = STATUS_SUCCESS;

  WDFDEVICE       Device         = WdfInterruptGetDevice(Interrupt);
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  Status = WdfWaitLockAcquire(pDeviceContext->DeviceWaitLock, 0);
  if (!NT_SUCCESS(Status))
    return;

  pDeviceContext->Register5 |= 0x40;
  Status = WriteRegister(
      pDeviceContext, 5, &pDeviceContext->Register5,
      sizeof(pDeviceContext->Register5));

  WdfWaitLockRelease(pDeviceContext->DeviceWaitLock);
}
