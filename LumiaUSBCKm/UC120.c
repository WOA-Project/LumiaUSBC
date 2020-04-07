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

NTSTATUS UC120_SetVConn(PDEVICE_CONTEXT DeviceContext, BOOLEAN VConnStatus)
{
  NTSTATUS Status;

  Status = ReadRegister(
      DeviceContext, 5, &DeviceContext->Register5,
      sizeof(DeviceContext->Register5));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

  DeviceContext->Register5 ^=
      (DeviceContext->Register5 ^ 0x20 * VConnStatus) & 0x20;
  Status = WriteRegister(
      DeviceContext, 5, &DeviceContext->Register5,
      sizeof(DeviceContext->Register5));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

exit:
  return Status;
}

NTSTATUS UC120_SetPowerRole(PDEVICE_CONTEXT DeviceContext, BOOLEAN PowerRole)
{
  NTSTATUS Status;

  Status = ReadRegister(
      DeviceContext, 5, &DeviceContext->Register5,
      sizeof(DeviceContext->Register5));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

  DeviceContext->Register5 ^= (DeviceContext->Register5 ^ PowerRole) & 1;
  Status = WriteRegister(
      DeviceContext, 5, &DeviceContext->Register5,
      sizeof(DeviceContext->Register5));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

exit:
  return Status;
}

NTSTATUS UC120_SetDataRole(PDEVICE_CONTEXT DeviceContext, BOOLEAN DataRole)
{
  NTSTATUS Status;

  Status = ReadRegister(
      DeviceContext, 5, &DeviceContext->Register5,
      sizeof(DeviceContext->Register5));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

  DeviceContext->Register5 ^= (DeviceContext->Register5 ^ 4 * DataRole) & 4;
  Status = WriteRegister(
      DeviceContext, 5, &DeviceContext->Register5,
      sizeof(DeviceContext->Register5));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

exit:
  return Status;
}

NTSTATUS UC120_ToggleReg4Bit1(PDEVICE_CONTEXT DeviceContext, BOOLEAN PowerRole)
{
  NTSTATUS Status;

  Status = ReadRegister(
      DeviceContext, 4, &DeviceContext->Register4,
      sizeof(DeviceContext->Register4));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

  DeviceContext->Register4 ^= (DeviceContext->Register4 ^ 2 * PowerRole) & 2;
  Status = WriteRegister(
      DeviceContext, 4, &DeviceContext->Register4,
      sizeof(DeviceContext->Register4));

  if (!NT_SUCCESS(Status)) {
    goto exit;
  }

exit:
  return Status;
}

NTSTATUS UC120_HandleInterrupt(PDEVICE_CONTEXT DeviceContext)
{
  NTSTATUS Status = STATUS_SUCCESS;
  UCHAR    Role;
  UCHAR    Polarity = 0;
  USHORT   State;
  UCHAR    CableType;
  UCHAR    Reg2B6;

  BOOLEAN IsPowerSource = FALSE;
  UCHAR   State3        = 4;

  if (DeviceContext->Register2 & 0xFC) {
    Status = ReadRegister(
        DeviceContext, 7, &DeviceContext->Register7,
        sizeof(DeviceContext->Register7));
    if (!NT_SUCCESS(Status))
      goto exit;

    Role  = (DeviceContext->Register2 & 0x3c) >> 2;
    State = 3;

    if (Role) {
      Polarity  = (DeviceContext->Register7 & 0x40) ? 2 : 1;
      CableType = (DeviceContext->Register7 >> 4) & 3;

      switch (CableType) {
      case 1:
        State = 0;
        break;
      case 2:
        State = 1;
        break;
      case 3:
        State = 2;
        break;
      }

      switch (Role) {
      case 1:
      case 4:
        if (!DeviceContext->IncomingPdHandled) {
          // Handle PD
          // 406c8c
          DeviceContext->PowerSource         = 2;
          DeviceContext->PdStateMachineIndex = 7;
          DeviceContext->IncomingPdHandled   = TRUE;
          DeviceContext->Polarity            = 0;
          DeviceContext->State3              = 4;
          UC120_ToggleReg4Bit1(DeviceContext, TRUE);
          UC120_SetVConn(DeviceContext, FALSE);
          UC120_SetPowerRole(DeviceContext, FALSE);
        }
        break;
      case 2:
        IsPowerSource = FALSE;
        State = 1;
        break;
      case 3:
        IsPowerSource = FALSE;
        State = 3;
        break;
      case 5:
        State         = 0;
        IsPowerSource = TRUE;
        break;
      case 6:
        State    = 5;
        Polarity = 0;
        IsPowerSource = FALSE;
        break;
      case 7:
        State    = 4;
        Polarity = 0;
        IsPowerSource = FALSE;
        break;
      case 8:
        IsPowerSource = TRUE;
        State         = 6;
        break;
      }

      // Powered cable?
      if (Role == 2 || Role == 3) {
        Status = UC120_SetVConn(DeviceContext, TRUE);
        if (!NT_SUCCESS(Status))
          goto exit;

        Status = UC120_SetPowerRole(DeviceContext, TRUE);
        if (!NT_SUCCESS(Status))
          goto exit;
      }

      if (DeviceContext->IncomingPdHandled) {
        // 406c8c
        DeviceContext->IncomingPdHandled   = FALSE;
        DeviceContext->PowerSource         = IsPowerSource;
        DeviceContext->Polarity            = Polarity;
        DeviceContext->PdStateMachineIndex = State;
        UC120_ToggleReg4Bit1(DeviceContext, FALSE);
      }
    }

    Reg2B6 = DeviceContext->Register2 >> 6;
    switch (Reg2B6) {
    case 1:
      State3 = 0;
      break;
    case 2:
      State3 = 1;
      break;
    case 3:
      State3 = 2;
      break;
    }

    if (State3 == DeviceContext->State3) {
      goto exit;
    }
    // 406c8c
    DeviceContext->State3 = State3;
  }

  if (DeviceContext->Register2 && 2) {
    // Handle incoming PD message
    Status = UC120_ProcessIncomingPdMessage(DeviceContext);
  }

  if (DeviceContext->Register2 && 1) {
    // Read incoming PD message status
    Status = UC120_ReadIncomingMessageStatus(DeviceContext);
  }

exit:
  return Status;
}

NTSTATUS UC120_ProcessIncomingPdMessage(PDEVICE_CONTEXT DeviceContext)
{
  NTSTATUS Status = STATUS_SUCCESS;
  UCHAR    IncomingMessgaeState;

  Status = ReadRegister(
      DeviceContext, 1, &DeviceContext->Register1,
      sizeof(DeviceContext->Register1));
  if (!NT_SUCCESS(Status))
    goto exit;

  IncomingMessgaeState = DeviceContext->Register1 >> 5;
  if (IncomingMessgaeState == 6) {
    DeviceContext->IncomingPdMessageState = 6;
    KeSetEvent(&DeviceContext->PdEvent, 0, 0);
  }
  else if (DeviceContext->Register1 & 0x1F || IncomingMessgaeState == 5) {
    // TODO: Read message
  }

exit:
  return Status;
}

NTSTATUS UC120_ReadIncomingMessageStatus(PDEVICE_CONTEXT DeviceContext) {
  NTSTATUS Status = STATUS_SUCCESS;
  Status          = ReadRegister(
      DeviceContext, 2, &DeviceContext->Register1,
      sizeof(DeviceContext->Register1));
  if (!NT_SUCCESS(Status))
    goto exit;

  DeviceContext->IncomingPdMessageState = DeviceContext->Register1 >> 5;
  KeSetEvent(&DeviceContext->PdEvent, 0, 0);

exit:
  return Status;
}
