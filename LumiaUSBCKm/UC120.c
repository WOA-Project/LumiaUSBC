#include <Driver.h>
#include <Registry.h>
#include <SPI.h>
#include <UC120.h>
#include <USBRole.h>
#include <wchar.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "UC120.tmh"
#endif

#include <UC120Registers.h>

NTSTATUS UC120_InterruptHandled(PDEVICE_CONTEXT deviceContext)
{
  NTSTATUS      status = STATUS_SUCCESS;
  unsigned char data   = 0;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptHandled Entry");

  data   = 0xFF; // Clear to 0xFF
  status = WriteRegister(deviceContext, 2, &data, 1);
  if (!NT_SUCCESS(status)) {
    goto Exit;
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptHandled Exit");
  return status;
}

NTSTATUS UC120_D0Entry(PDEVICE_CONTEXT deviceContext)
{
  NTSTATUS status = STATUS_SUCCESS;
  UCHAR    Val    = 0;

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_D0Entry Entry");

  // Write as is
  Val    = 0x6;
  status = WriteRegister(deviceContext, 4, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  Val    = 0x88;
  status = WriteRegister(deviceContext, 5, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  Val    = 0x2;
  status = WriteRegister(deviceContext, 13, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  Val    = 0xff;
  status = WriteRegister(deviceContext, 2, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  Val    = 0xff;
  status = WriteRegister(deviceContext, 3, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  Val    = 0x7;
  status = WriteRegister(deviceContext, 4, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  Val    = 0x8;
  status = WriteRegister(deviceContext, 5, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  status = ReadRegister(deviceContext, 2, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "ReadRegister failed with %!STATUS!",
        status);
    goto exit;
  }

  status = ReadRegister(deviceContext, 7, &Val, 1);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DEVICE, "ReadRegister failed with %!STATUS!",
        status);
    goto exit;
  }

exit:
  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_D0Entry Exit");
  return status;
}

NTSTATUS UC120_UploadCalibrationData(
    PDEVICE_CONTEXT deviceContext, unsigned char *calibrationFile,
    unsigned int length)
{
  NTSTATUS status = STATUS_SUCCESS;
#if 1
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "UC120_UploadCalibrationData Entry");

  switch (length) {
  case 10: {
    status = WriteRegister(deviceContext, 18, calibrationFile + 0, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 19, calibrationFile + 1, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 20, calibrationFile + 2, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 21, calibrationFile + 3, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 26, calibrationFile + 4, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 22, calibrationFile + 5, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 23, calibrationFile + 6, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 24, calibrationFile + 7, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 25, calibrationFile + 8, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 27, calibrationFile + 9, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    break;
  }
  case 8: {
    status = WriteRegister(deviceContext, 18, calibrationFile + 0, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 19, calibrationFile + 1, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 20, calibrationFile + 2, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 21, calibrationFile + 3, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 22, calibrationFile + 4, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 23, calibrationFile + 5, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 24, calibrationFile + 6, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    status = WriteRegister(deviceContext, 25, calibrationFile + 7, 1);
    if (!NT_SUCCESS(status)) {
      goto exit;
    }
    break;
  }
  default: {
    status = STATUS_BAD_DATA;
    goto exit;
  }
  }

exit:
#else
  UNREFERENCED_PARAMETER(deviceContext);
  UNREFERENCED_PARAMETER(calibrationFile);
  UNREFERENCED_PARAMETER(length);
#endif

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "UC120_UploadCalibrationData Exit");
  return status;
}

NTSTATUS
Uc120InterruptEnable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice)
{
  UNREFERENCED_PARAMETER(AssociatedDevice);

  NTSTATUS        Status;
  WDFDEVICE       Device         = WdfInterruptGetDevice(Interrupt);
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  pDeviceContext->Register2 = 0xFF;
  Status                    = WriteRegister(
      pDeviceContext, 2, &pDeviceContext->Register2,
      sizeof(pDeviceContext->Register2));

  if (!NT_SUCCESS(Status))
    goto exit;

  pDeviceContext->Register3 = 0xFF;
  Status                    = WriteRegister(
      pDeviceContext, 3, &pDeviceContext->Register3,
      sizeof(pDeviceContext->Register3));

  if (!NT_SUCCESS(Status))
    goto exit;

  pDeviceContext->Register4 |= 1;
  Status = WriteRegister(
      pDeviceContext, 4, &pDeviceContext->Register4,
      sizeof(pDeviceContext->Register4));

  if (!NT_SUCCESS(Status))
    goto exit;

  pDeviceContext->Register5 &= 0x7F;
  Status = WriteRegister(
      pDeviceContext, 5, &pDeviceContext->Register5,
      sizeof(pDeviceContext->Register5));

  if (!NT_SUCCESS(Status))
    goto exit;

  Status = ReadRegister(
      pDeviceContext, 2, &pDeviceContext->Register2,
      sizeof(pDeviceContext->Register2));

  if (!NT_SUCCESS(Status))
    goto exit;

  Status = ReadRegister(
      pDeviceContext, 7, &pDeviceContext->Register7,
      sizeof(pDeviceContext->Register7));

exit:
  return Status;
}

NTSTATUS
Uc120InterruptDisable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice)
{
  UNREFERENCED_PARAMETER(AssociatedDevice);

  NTSTATUS        Status;
  WDFDEVICE       Device         = WdfInterruptGetDevice(Interrupt);
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  pDeviceContext->Register4 &= 0xFE;
  Status = WriteRegister(
      pDeviceContext, 4, &pDeviceContext->Register4,
      sizeof(pDeviceContext->Register4));

  if (!NT_SUCCESS(Status))
    goto exit;

exit:
  return Status;
}
