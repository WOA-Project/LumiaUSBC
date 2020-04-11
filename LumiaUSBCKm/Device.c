#include <Driver.h>

#ifndef RESHUB_USE_HELPER_ROUTINES
#define RESHUB_USE_HELPER_ROUTINES
#endif

#include <reshub.h>
#include <wdf.h>

#include <Registry.h>
#include <SPI.h>
#include <UC120.h>
#include <WorkItems.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LumiaUSBCKmCreateDevice)
#pragma alloc_text(PAGE, LumiaUSBCDevicePrepareHardware)
#endif

NTSTATUS LumiaUSBCOpenIOTarget(
    PDEVICE_CONTEXT pDeviceContext, LARGE_INTEGER Resource, ACCESS_MASK AccessMask,
    WDFIOTARGET *IoTarget)
{
  NTSTATUS                  status = STATUS_SUCCESS;
  WDF_OBJECT_ATTRIBUTES     ObjectAttributes;
  WDF_IO_TARGET_OPEN_PARAMS OpenParams;
  UNICODE_STRING            ReadString;
  WCHAR                     ReadStringBuffer[260];

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget Entry");
  DbgPrintEx(DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCOpenIOTarget entry\n");

  RtlInitEmptyUnicodeString(
      &ReadString, ReadStringBuffer, sizeof(ReadStringBuffer));

  status =
      RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString, Resource.LowPart, Resource.HighPart);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "RESOURCE_HUB_CREATE_PATH_FROM_ID failed 0x%x", status);
    goto Exit;
  }

  WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
  ObjectAttributes.ParentObject = pDeviceContext->Device;

  status = WdfIoTargetCreate(pDeviceContext->Device, &ObjectAttributes, IoTarget);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfIoTargetCreate failed 0x%x",
        status);
    goto Exit;
  }

  WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams, &ReadString, AccessMask);
  status = WdfIoTargetOpen(*IoTarget, &OpenParams);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfIoTargetOpen failed 0x%x",
        status);
    goto Exit;
  }

Exit:
  DbgPrintEx(DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCOpenIOTarget exit: 0x%x\n", status);
  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget Exit");
  return status;
}

NTSTATUS
LumiaUSBCProbeResources(
    PDEVICE_CONTEXT DeviceContext, WDFCMRESLIST ResourcesTranslated,
    WDFCMRESLIST ResourcesRaw)
{
  PAGED_CODE();
  UNREFERENCED_PARAMETER(ResourcesRaw);

  NTSTATUS                        Status        = STATUS_SUCCESS;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDescriptor = NULL;

  UCHAR SpiFound  = FALSE;

  ULONG ResourceCount;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "-> LumiaUSBCProbeResources");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCProbeResources entry\n");

  ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);

  for (ULONG i = 0; i < ResourceCount; i++) {
    ResDescriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

    switch (ResDescriptor->Type) {
    case CmResourceTypeConnection:
      // Check for SPI resource
      if (ResDescriptor->u.Connection.Class ==
              CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
          ResDescriptor->u.Connection.Type ==
              CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI) {
        DeviceContext->SpiId.LowPart  = ResDescriptor->u.Connection.IdLowPart;
        DeviceContext->SpiId.HighPart = ResDescriptor->u.Connection.IdHighPart;

        TraceEvents(
            TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
            "Found SPI resource index=%lu", i);

        SpiFound = TRUE;
      }
      break;
    default:
      // We don't care about other descriptors.
      break;
    }
  }

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<- LumiaUSBCProbeResources");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCProbeResources exit: 0x%x\n",
      Status);
  return Status;
}

NTSTATUS
LumiaUSBCOpenResources(PDEVICE_CONTEXT pDeviceContext)
{
  NTSTATUS status = STATUS_SUCCESS;
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources Entry");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCOpenResources entry\n");

  status = LumiaUSBCOpenIOTarget(
      pDeviceContext, pDeviceContext->SpiId, GENERIC_READ | GENERIC_WRITE,
      &pDeviceContext->Spi);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "OpenIOTarget failed for SPI 0x%x Falling back to fake SPI.", status);
    goto Exit;
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources Exit");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL,
      "LumiaUSBCOpenResources exit: 0x%x\n", status);
  return status;
}

NTSTATUS
LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
  WDF_OBJECT_ATTRIBUTES        DeviceAttributes;
  WDF_OBJECT_ATTRIBUTES        ObjAttrib;
  WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
  PDEVICE_CONTEXT              DeviceContext;
  WDFDEVICE                    Device;
  NTSTATUS                     Status;

  WDF_INTERRUPT_CONFIG InterruptConfig;

  PAGED_CODE();

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCKmCreateDevice Entry");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCKmCreateDevice entry\n");

  WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
  PnpPowerCallbacks.EvtDevicePrepareHardware = LumiaUSBCDevicePrepareHardware;
  PnpPowerCallbacks.EvtDeviceD0Entry         = LumiaUSBCDeviceD0Entry;
  WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, DEVICE_CONTEXT);

  Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &Device);

  if (NT_SUCCESS(Status)) {
    DeviceContext = DeviceGetContext(Device);

    //
    // Initialize the context.
    //
    DeviceContext->Device              = Device;
    DeviceContext->State3              = 4;
    DeviceContext->PdStateMachineIndex = 7;
    DeviceContext->State0   = TRUE;
    DeviceContext->Polarity            = 0;
    DeviceContext->PowerSource         = 2;

    // Initialize all interrupts (consistent with IDA result)
    // UC120
    WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtUc120InterruptIsr, NULL);
    InterruptConfig.EvtInterruptEnable  = Uc120InterruptEnable;
    InterruptConfig.EvtInterruptDisable = Uc120InterruptDisable;
    InterruptConfig.PassiveHandling     = TRUE;

    Status = WdfInterruptCreate(
        Device, &InterruptConfig, NULL, &DeviceContext->Uc120Interrupt);

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_ERROR, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice WdfInterruptCreate Uc120Interrupt failed: "
          "%!STATUS!\n",
          Status);
      goto Exit;
    }

    WdfInterruptSetPolicy(
        DeviceContext->Uc120Interrupt, WdfIrqPolicyAllProcessorsInMachine,
        WdfIrqPriorityHigh, 0);

    // Plug detection
    WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtPlugDetInterruptIsr, NULL);
    InterruptConfig.PassiveHandling = TRUE;

    Status = WdfInterruptCreate(
        Device, &InterruptConfig, NULL, &DeviceContext->PlugDetectInterrupt);

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_ERROR, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice WdfInterruptCreate PlugDetectInterrupt "
          "failed: %!STATUS!\n",
          Status);
      goto Exit;
    }

    // PMIC1 Interrupt
    WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtPmicInterrupt1Isr, NULL);
    InterruptConfig.PassiveHandling      = TRUE;
    InterruptConfig.EvtInterruptWorkItem = PmicInterrupt1WorkItem;

    Status = WdfInterruptCreate(
        Device, &InterruptConfig, NULL, &DeviceContext->PmicInterrupt1);

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_ERROR, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice WdfInterruptCreate PmicInterrupt1 failed: "
          "%!STATUS!\n",
          Status);
      goto Exit;
    }

    // PMIC2 Interrupt
    WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtPmicInterrupt2Isr, NULL);
    InterruptConfig.PassiveHandling = TRUE;

    Status = WdfInterruptCreate(
        Device, &InterruptConfig, NULL, &DeviceContext->PmicInterrupt2);

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_ERROR, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice WdfInterruptCreate PmicInterrupt2 failed: "
          "%!STATUS!\n",
          Status);
      goto Exit;
    }

    // DeviceColletion
    WDF_OBJECT_ATTRIBUTES_INIT(&ObjAttrib);
    ObjAttrib.ParentObject = Device;

    Status = WdfCollectionCreate(&ObjAttrib, &DeviceContext->DevicePendingIoReqCollection);
    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice failed to WdfCollectionCreate: %!STATUS!\n",
          Status);
      goto Exit;
    }

    // Waitlock
    WDF_OBJECT_ATTRIBUTES_INIT(&ObjAttrib);
    ObjAttrib.ParentObject = Device;

    Status = WdfWaitLockCreate(&ObjAttrib, &DeviceContext->DeviceWaitLock);

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice failed to WdfWaitLockCreate: %!STATUS!\n",
          Status);
      goto Exit;
    }

    // IO Queue
    Status = LumiaUSBCInitializeIoQueue(Device);
    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice failed to LumiaUSBCInitializeIoQueue: %!STATUS!\n",
          Status);
      goto Exit;
    }
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCKmCreateDevice Exit: 0x%x\n", Status);
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL,
      "LumiaUSBCKmCreateDevice exit: 0x%x\n", Status);
  return Status;
}

NTSTATUS
LumiaUSBCDevicePrepareHardware(
    WDFDEVICE Device, WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated)
{
  NTSTATUS        Status = STATUS_SUCCESS;
  PDEVICE_CONTEXT pDeviceContext;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCDevicePrepareHardware Entry");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCDevicePrepareHardware entry\n");

  pDeviceContext = DeviceGetContext(Device);

  Status = LumiaUSBCProbeResources(
      pDeviceContext, ResourcesTranslated, ResourcesRaw);

  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "LumiaUSBCProbeResources failed 0x%x\n", Status);
    goto Exit;
  }

  // Open SPI device
  Status = LumiaUSBCOpenResources(pDeviceContext);
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "LumiaUSBCOpenResources failed 0x%x\n", Status);
    goto Exit;
  }

  // Create Device Interface
  Status = WdfDeviceCreateDeviceInterface(
      Device, &GUID_DEVINTERFACE_LumiaUSBCKm, NULL);
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "WdfDeviceCreateDeviceInterface failed 0x%x\n", Status);
    goto Exit;
  }

  // Initialize PD event
  KeInitializeEvent(&pDeviceContext->PdEvent, NotificationEvent, FALSE);

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCDevicePrepareHardware Exit");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL,
      "LumiaUSBCDevicePrepareHardware exit: 0x%x\n", Status);
  return Status;
}

NTSTATUS
LumiaUSBCDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
  UNREFERENCED_PARAMETER(PreviousState);

  NTSTATUS        Status         = STATUS_SUCCESS;
  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  LARGE_INTEGER Delay;

  UNICODE_STRING            CalibrationFileString;
  OBJECT_ATTRIBUTES         CalibrationFileObjectAttribute;
  HANDLE                    hCalibrationFile;
  IO_STATUS_BLOCK           CalibrationIoStatusBlock;
  UCHAR                     CalibrationBlob[UC120_CALIBRATIONFILE_SIZE + 2];
  LARGE_INTEGER             CalibrationFileByteOffset;
  FILE_STANDARD_INFORMATION CalibrationFileInfo;
  // { 0x0C, 0x7C, 0x31, 0x5E, 0x9D, 0x0D, 0x7D, 0x32, 0x5F, 0x9E };
  UCHAR DefaultCalibrationBlob[] = {0x0C, 0x7C, 0x31, 0x5E, 0x9D,
                                    0x0A, 0x7A, 0x2F, 0x5C, 0x9B};

  LONGLONG CalibrationFileSize = 0;
  UCHAR  SkipCalibration     = FALSE;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Entry Entry");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL,
      "LumiaUSBCDeviceD0Entry entry\n");

  // Read calibration file
  RtlInitUnicodeString(
      &CalibrationFileString, L"\\DosDevices\\C:\\DPP\\MMO\\ice5lp_2k_cal.bin");
  InitializeObjectAttributes(
      &CalibrationFileObjectAttribute, &CalibrationFileString,
      OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

  // Should not happen
  if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
    Status = STATUS_INVALID_DEVICE_STATE;
    goto Exit;
  }

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "acquire calibration file handle");
  Status = ZwCreateFile(
      &hCalibrationFile, GENERIC_READ, &CalibrationFileObjectAttribute,
      &CalibrationIoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN,
      FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "failed to open calibration file 0x%x, skipping calibration. Is this a "
        "FUSBC device?",
        Status);
    Status          = STATUS_SUCCESS;
    SkipCalibration = TRUE;
  }

  if (!SkipCalibration) {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "stat calibration file");
    Status = ZwQueryInformationFile(
        hCalibrationFile, &CalibrationIoStatusBlock, &CalibrationFileInfo,
        sizeof(CalibrationFileInfo), FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "failed to stat calibration file 0x%x", Status);
      ZwClose(hCalibrationFile);
      goto Exit;
    }

    CalibrationFileSize = CalibrationFileInfo.EndOfFile.QuadPart;
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "calibration file size %lld",
        CalibrationFileSize);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "read calibration file");
    RtlZeroMemory(CalibrationBlob, sizeof(CalibrationBlob));
    CalibrationFileByteOffset.LowPart  = 0;
    CalibrationFileByteOffset.HighPart = 0;
    Status                             = ZwReadFile(
        hCalibrationFile, NULL, NULL, NULL, &CalibrationIoStatusBlock,
        CalibrationBlob, UC120_CALIBRATIONFILE_SIZE, &CalibrationFileByteOffset,
        NULL);

    ZwClose(hCalibrationFile);
    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "failed to read calibration file 0x%x", Status);
      goto Exit;
    }
  }

  // UC120 init sequence
  pDeviceContext->Register4 |= 6;
  Status = WriteRegister(
      pDeviceContext, 4, &pDeviceContext->Register4,
      sizeof(pDeviceContext->Register4));
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "Initseq Write Register 4 failed 0x%x", Status);
    goto Exit;
  }

  pDeviceContext->Register5 = 0x88;
  Status                    = WriteRegister(
      pDeviceContext, 5, &pDeviceContext->Register5,
      sizeof(pDeviceContext->Register5));
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "Initseq Write Register 5 failed 0x%x", Status);
    goto Exit;
  }

  pDeviceContext->Register13 = pDeviceContext->Register13 & 0xFC | 2;
  Status                     = WriteRegister(
      pDeviceContext, 13, &pDeviceContext->Register13,
      sizeof(pDeviceContext->Register13));
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "Initseq Write Register 13 failed 0x%x", Status);
    goto Exit;
  }

  if (!SkipCalibration) {
    // Initialize the UC120 accordingly
    if (CalibrationFileSize == 11) {
      // Skip the first byte
      Status =
          UC120_UploadCalibrationData(pDeviceContext, &CalibrationBlob[1], 10);
    }
    else if (CalibrationFileSize == 8) {
      // No skip
      Status = UC120_UploadCalibrationData(pDeviceContext, CalibrationBlob, 8);
    }
    else {
      // Not recognized, use default
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "Unknown calibration data, fallback to the default");
      Status = UC120_UploadCalibrationData(
          pDeviceContext, DefaultCalibrationBlob,
          sizeof(DefaultCalibrationBlob));
    }

    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "UC120_UploadCalibrationData failed 0x%x", Status);
      goto Exit;
    }

    // TODO: understand what's this
    pDeviceContext->State9 = 1;
  }

  Delay.QuadPart = -2000000;
  KeDelayExecutionThread(UserMode, TRUE, &Delay);

Exit:
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL,
      "LumiaUSBCDeviceD0Entry exit: 0x%x\n", Status);
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Entry Exit");
  return Status;
}

