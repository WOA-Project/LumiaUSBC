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

#ifndef DBG_PRINT_EX_LOGGING
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LumiaUSBCKmCreateDevice)
#pragma alloc_text(PAGE, LumiaUSBCDevicePrepareHardware)
#pragma alloc_text(PAGE, LumiaUSBCSetDataRole)
#endif

NTSTATUS OpenIOTarget(
    PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use,
    WDFIOTARGET *target)
{
  NTSTATUS                  status = STATUS_SUCCESS;
  WDF_OBJECT_ATTRIBUTES     ObjectAttributes;
  WDF_IO_TARGET_OPEN_PARAMS OpenParams;
  UNICODE_STRING            ReadString;
  WCHAR                     ReadStringBuffer[260];

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget Entry");

  RtlInitEmptyUnicodeString(
      &ReadString, ReadStringBuffer, sizeof(ReadStringBuffer));

  status =
      RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString, res.LowPart, res.HighPart);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "RESOURCE_HUB_CREATE_PATH_FROM_ID failed 0x%x", status);
    goto Exit;
  }

  WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
  ObjectAttributes.ParentObject = ctx->Device;

  status = WdfIoTargetCreate(ctx->Device, &ObjectAttributes, target);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfIoTargetCreate failed 0x%x",
        status);
    goto Exit;
  }

  WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams, &ReadString, use);
  status = WdfIoTargetOpen(*target, &OpenParams);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfIoTargetOpen failed 0x%x",
        status);
    goto Exit;
  }

Exit:
  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget Exit");
  return status;
}

NTSTATUS
LumiaUSBCProbeResources(
    PDEVICE_CONTEXT DeviceContext, WDFCMRESLIST ResourcesTranslated,
    WDFCMRESLIST ResourcesRaw)
{
  PAGED_CODE();

  NTSTATUS                        Status = STATUS_SUCCESS;
  WDF_INTERRUPT_CONFIG            InterruptConfig;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDescriptor = NULL;

  BOOLEAN SpiFound       = FALSE;
  ULONG   GpioFound      = 0;
  ULONG   InterruptFound = 0;

  ULONG PlugDetInterrupt = 0;
  ULONG UC120Interrupt   = 0;
  ULONG PmicInterrupt1   = 0;
  ULONG PmicInterrupt2   = 0;

  ULONG ResourceCount;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "-> LumiaUSBCProbeResources");

  ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);

  for (ULONG i = 0; i < ResourceCount; i++) {
    ResDescriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

    switch (ResDescriptor->Type) {
    case CmResourceTypeConnection:
      // Check for GPIO resource
      if (ResDescriptor->u.Connection.Class ==
              CM_RESOURCE_CONNECTION_CLASS_GPIO &&
          ResDescriptor->u.Connection.Type ==
              CM_RESOURCE_CONNECTION_TYPE_GPIO_IO) {
        switch (GpioFound) {
        case 0:
          DeviceContext->VbusGpioId.LowPart =
              ResDescriptor->u.Connection.IdLowPart;
          DeviceContext->VbusGpioId.HighPart =
              ResDescriptor->u.Connection.IdHighPart;
          break;
        case 1:
          DeviceContext->PolGpioId.LowPart =
              ResDescriptor->u.Connection.IdLowPart;
          DeviceContext->PolGpioId.HighPart =
              ResDescriptor->u.Connection.IdHighPart;
          break;
        case 2:
          DeviceContext->AmselGpioId.LowPart =
              ResDescriptor->u.Connection.IdLowPart;
          DeviceContext->AmselGpioId.HighPart =
              ResDescriptor->u.Connection.IdHighPart;
          break;
        case 3:
          DeviceContext->EnGpioId.LowPart =
              ResDescriptor->u.Connection.IdLowPart;
          DeviceContext->EnGpioId.HighPart =
              ResDescriptor->u.Connection.IdHighPart;
          break;
        default:
          break;
        }

        TraceEvents(
            TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
            "Found GPIO resource id=%lu index=%lu", GpioFound, i);

        GpioFound++;
      }
      // Check for SPI resource
      else if (
          ResDescriptor->u.Connection.Class ==
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

    case CmResourceTypeInterrupt:
      // We've found an interrupt resource.

      switch (InterruptFound) {
      case 0:
        PlugDetInterrupt = i;
        break;
      case 1:
        UC120Interrupt = i;
        break;
      case 2:
        PmicInterrupt1 = i;
        break;
      case 3:
        PmicInterrupt2 = i;
        break;
      default:
        break;
      }

      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "Found Interrupt resource id=%lu index=%lu", InterruptFound, i);

      InterruptFound++;
      break;

    default:
      // We don't care about other descriptors.
      break;
    }
  }

  if (!SpiFound || GpioFound < 4 || InterruptFound < 4) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER, "Not all resources were found");
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto Exit;
  }

  WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtUc120InterruptIsr, NULL);
  InterruptConfig.InterruptTranslated =
      WdfCmResourceListGetDescriptor(ResourcesTranslated, UC120Interrupt);
  InterruptConfig.InterruptRaw =
      WdfCmResourceListGetDescriptor(ResourcesRaw, UC120Interrupt);
  InterruptConfig.EvtInterruptEnable  = Uc120InterruptEnable;
  InterruptConfig.EvtInterruptDisable = Uc120InterruptDisable;
  InterruptConfig.PassiveHandling     = TRUE;

  Status = WdfInterruptCreate(
      DeviceContext->Device, &InterruptConfig, WDF_NO_OBJECT_ATTRIBUTES,
      &DeviceContext->Uc120Interrupt);

  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WdfInterruptCreate failed for UC120 interrupt, 0x%x", Status);
    goto Exit;
  }

  WdfInterruptSetPolicy(
      DeviceContext->Uc120Interrupt, WdfIrqPolicyAllProcessorsInMachine,
      WdfIrqPriorityHigh, AFFINITY_MASK(0));

  WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtPlugDetInterruptIsr, NULL);
  InterruptConfig.InterruptTranslated =
      WdfCmResourceListGetDescriptor(ResourcesTranslated, PlugDetInterrupt);
  InterruptConfig.InterruptRaw =
      WdfCmResourceListGetDescriptor(ResourcesRaw, PlugDetInterrupt);
  InterruptConfig.PassiveHandling = TRUE;

  Status = WdfInterruptCreate(
      DeviceContext->Device, &InterruptConfig, WDF_NO_OBJECT_ATTRIBUTES,
      &DeviceContext->PlugDetectInterrupt);

  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WdfInterruptCreate failed for plug detection, 0x%x", Status);
    goto Exit;
  }

  WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtPmicInterrupt1Isr, NULL);
  InterruptConfig.PassiveHandling = TRUE;
  InterruptConfig.InterruptTranslated =
      WdfCmResourceListGetDescriptor(ResourcesTranslated, PmicInterrupt1);
  InterruptConfig.InterruptRaw =
      WdfCmResourceListGetDescriptor(ResourcesRaw, PmicInterrupt1);
  InterruptConfig.EvtInterruptWorkItem = PmicInterrupt1WorkItem;

  Status = WdfInterruptCreate(
      DeviceContext->Device, &InterruptConfig, WDF_NO_OBJECT_ATTRIBUTES,
      &DeviceContext->PmicInterrupt1);

  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WdfInterruptCreate failed for PMIC 1, 0x%x", Status);
    goto Exit;
  }

  WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, EvtPmicInterrupt2Isr, NULL);
  InterruptConfig.PassiveHandling = TRUE;
  InterruptConfig.InterruptTranslated =
      WdfCmResourceListGetDescriptor(ResourcesTranslated, PmicInterrupt2);
  InterruptConfig.InterruptRaw =
      WdfCmResourceListGetDescriptor(ResourcesRaw, PmicInterrupt2);

  Status = WdfInterruptCreate(
      DeviceContext->Device, &InterruptConfig, WDF_NO_OBJECT_ATTRIBUTES,
      &DeviceContext->PmicInterrupt2);

  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WdfInterruptCreate failed for PMIC 2, 0x%x", Status);
    goto Exit;
  }

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<- LumiaUSBCProbeResources");

Exit:
  return Status;
}

NTSTATUS
LumiaUSBCOpenResources(PDEVICE_CONTEXT ctx)
{
  NTSTATUS status = STATUS_SUCCESS;
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources Entry");

  status =
      OpenIOTarget(ctx, ctx->SpiId, GENERIC_READ | GENERIC_WRITE, &ctx->Spi);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "OpenIOTarget failed for SPI 0x%x Falling back to fake SPI.", status);
    goto Exit;
  }

  status = OpenIOTarget(
      ctx, ctx->VbusGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->VbusGpio);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "OpenIOTarget failed for VBUS GPIO 0x%x", status);
    goto Exit;
  }

  status = OpenIOTarget(
      ctx, ctx->PolGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->PolGpio);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "OpenIOTarget failed for polarity GPIO 0x%x", status);
    goto Exit;
  }

  status = OpenIOTarget(
      ctx, ctx->AmselGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->AmselGpio);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "OpenIOTarget failed for alternate mode selection GPIO 0x%x", status);
    goto Exit;
  }

  status = OpenIOTarget(
      ctx, ctx->EnGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->EnGpio);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "OpenIOTarget failed for mux enable GPIO 0x%x", status);
    goto Exit;
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources Exit");
  return status;
}

void LumiaUSBCCloseResources(PDEVICE_CONTEXT ctx)
{
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCCloseResources Entry");

  if (ctx->Spi) {
    WdfIoTargetClose(ctx->Spi);
  }

  if (ctx->VbusGpio) {
    WdfIoTargetClose(ctx->VbusGpio);
  }

  if (ctx->PolGpio) {
    WdfIoTargetClose(ctx->PolGpio);
  }

  if (ctx->AmselGpio) {
    WdfIoTargetClose(ctx->AmselGpio);
  }

  if (ctx->EnGpio) {
    WdfIoTargetClose(ctx->EnGpio);
  }

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCCloseResources Exit");
}

NTSTATUS
LumiaUSBCDeviceD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE TargetState)
{
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Exit Entry");

  PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);

  switch (TargetState) {
  case WdfPowerDeviceInvalid: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceInvalid");
    break;
  }
  case WdfPowerDeviceD0: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD0");
    break;
  }
  case WdfPowerDeviceD1: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD1");
    break;
  }
  case WdfPowerDeviceD2: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD2");
    break;
  }
  case WdfPowerDeviceD3: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD3");
    break;
  }
  case WdfPowerDeviceD3Final: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD3Final");
    break;
  }
  case WdfPowerDevicePrepareForHibernation: {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "WdfPowerDevicePrepareForHibernation");
    break;
  }
  case WdfPowerDeviceMaximum: {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceMaximum");
    break;
  }
  }

  LumiaUSBCCloseResources(devCtx);

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Exit Exit");
  return STATUS_SUCCESS;
}

NTSTATUS LumiaUSBCDeviceReleaseHardware(
    WDFDEVICE Device, WDFCMRESLIST ResourcesTranslated)
{
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCDeviceReleaseHardware Entry");

  UNREFERENCED_PARAMETER((Device, ResourcesTranslated));

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCDeviceReleaseHardware Exit");

  return STATUS_SUCCESS;
}

NTSTATUS
LumiaUSBCDevicePrepareHardware(
    WDFDEVICE Device, WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated)
{
  NTSTATUS                   status = STATUS_SUCCESS;
  PDEVICE_CONTEXT            devCtx;
  UCM_MANAGER_CONFIG         ucmCfg;
  UCM_CONNECTOR_CONFIG       connCfg;
  UCM_CONNECTOR_TYPEC_CONFIG typeCConfig;
  UCM_CONNECTOR_PD_CONFIG    pdConfig;
  WDF_OBJECT_ATTRIBUTES      attr;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCDevicePrepareHardware Entry");

  devCtx = DeviceGetContext(Device);

  status = LumiaUSBCProbeResources(devCtx, ResourcesTranslated, ResourcesRaw);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "LumiaUSBCProbeResources failed 0x%x\n", status);
    goto Exit;
  }

  if (devCtx->Connector) {
    goto Exit;
  }

  //
  // Initialize UCM Manager
  //
  UCM_MANAGER_CONFIG_INIT(&ucmCfg);

  status = UcmInitializeDevice(Device, &ucmCfg);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "UcmInitializeDevice failed 0x%x\n", status);
    goto Exit;
  }

  //
  // Assemble the Type-C and PD configuration for UCM.
  //

  UCM_CONNECTOR_CONFIG_INIT(&connCfg, 0);

  UCM_CONNECTOR_TYPEC_CONFIG_INIT(
      &typeCConfig,
      UcmTypeCOperatingModeDrp | UcmTypeCOperatingModeUfp |
          UcmTypeCOperatingModeDfp,
      UcmTypeCCurrent3000mA | UcmTypeCCurrent1500mA |
          UcmTypeCCurrentDefaultUsb);

  typeCConfig.EvtSetDataRole        = LumiaUSBCSetDataRole;
  typeCConfig.AudioAccessoryCapable = FALSE;

  UCM_CONNECTOR_PD_CONFIG_INIT(
      &pdConfig, UcmPowerRoleSink | UcmPowerRoleSource);

  pdConfig.EvtSetPowerRole = LumiaUSBCSetPowerRole;
  connCfg.TypeCConfig      = &typeCConfig;
  connCfg.PdConfig         = &pdConfig;

  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, CONNECTOR_CONTEXT);

  //
  // Create the UCM connector object.
  //

  status = UcmConnectorCreate(Device, &connCfg, &attr, &devCtx->Connector);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "UcmConnectorCreate failed 0x%x\n", status);
    goto Exit;
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCDevicePrepareHardware Exit");
  return status;
}

NTSTATUS
LumiaUSBCDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
  NTSTATUS status = STATUS_SUCCESS;
  UNREFERENCED_PARAMETER(PreviousState);

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Entry Entry");

  PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);

  status = LumiaUSBCOpenResources(devCtx);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
        "LumiaUSBCOpenResources failed 0x%x\n", status);
    goto Exit;
  }

  unsigned char value = (unsigned char)0;
  SetGPIO(devCtx, devCtx->PolGpio, &value);

  value = (unsigned char)0;
  SetGPIO(
      devCtx, devCtx->AmselGpio,
      &value); // high = HDMI only, medium (unsupported) = USB only, low = both

  value = (unsigned char)1;
  SetGPIO(devCtx, devCtx->EnGpio, &value);

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Entry Exit");
  return status;
}

NTSTATUS LumiaUSBCSelfManagedIoInit(WDFDEVICE Device)
{
  NTSTATUS      Status = STATUS_SUCCESS;
  ULONG         Input[8], Output[6];
  LARGE_INTEGER Delay;

  PO_FX_DEVICE               PoFxDevice;
  PO_FX_COMPONENT_IDLE_STATE IdleState;

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
  BOOLEAN  SkipCalibration     = FALSE;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCSelfManagedIoInit Entry");

  PDEVICE_CONTEXT pDeviceContext = DeviceGetContext(Device);

  memset(&PoFxDevice, 0, sizeof(PoFxDevice));
  memset(&IdleState, 0, sizeof(IdleState));
  PoFxDevice.Version                      = PO_FX_VERSION_V1;
  PoFxDevice.ComponentCount               = 1;
  PoFxDevice.Components[0].IdleStateCount = 1;
  PoFxDevice.Components[0].IdleStates     = &IdleState;
  PoFxDevice.DeviceContext                = pDeviceContext;
  IdleState.NominalPower                  = PO_FX_UNKNOWN_POWER;

  Status = PoFxRegisterDevice(
      WdfDeviceWdmGetPhysicalDevice(Device), &PoFxDevice,
      &pDeviceContext->PoHandle);
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "PoFxRegisterDevice failed 0x%x",
        Status);
    goto Exit;
  }

  PoFxActivateComponent(pDeviceContext->PoHandle, 0, PO_FX_FLAG_BLOCKING);
  PoFxStartDevicePowerManagement(pDeviceContext->PoHandle);

  // Tell PEP to turn on the clock
  memset(Input, 0, sizeof(Input));
  Input[0] = 2;
  Input[7] = 2;
  Status   = PoFxPowerControl(
      pDeviceContext->PoHandle, &PowerControlGuid, &Input, sizeof(Input),
      &Output, sizeof(Output), NULL);
  if (!NT_SUCCESS(Status)) {
    TraceEvents(
        TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "PoFxPowerControl failed 0x%x",
        Status);
    goto Exit;
  }

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

    // TODO: Write Unk1 that have not yet understood
  }

  Delay.QuadPart = -2000000;
  KeDelayExecutionThread(UserMode, TRUE, &Delay);

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSelfManagedIoInit Exit");
  return Status;
}

NTSTATUS
LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
  WDF_OBJECT_ATTRIBUTES        DeviceAttributes;
  WDF_OBJECT_ATTRIBUTES        WaitLockAttrib;
  WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
  PDEVICE_CONTEXT              DeviceContext;
  WDFDEVICE                    Device;
  UCM_MANAGER_CONFIG           UcmConfig;
  NTSTATUS                     Status;

  PAGED_CODE();

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCKmCreateDevice Entry");

  WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
  PnpPowerCallbacks.EvtDevicePrepareHardware   = LumiaUSBCDevicePrepareHardware;
  PnpPowerCallbacks.EvtDeviceReleaseHardware   = LumiaUSBCDeviceReleaseHardware;
  PnpPowerCallbacks.EvtDeviceD0Entry           = LumiaUSBCDeviceD0Entry;
  PnpPowerCallbacks.EvtDeviceD0Exit            = LumiaUSBCDeviceD0Exit;
  PnpPowerCallbacks.EvtDeviceSelfManagedIoInit = LumiaUSBCSelfManagedIoInit;
  WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, DEVICE_CONTEXT);

  Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &Device);

  if (NT_SUCCESS(Status)) {
    DeviceContext = DeviceGetContext(Device);

    //
    // Initialize the context.
    //
    DeviceContext->Device    = Device;
    DeviceContext->Connector = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT(&WaitLockAttrib);
    WaitLockAttrib.ExecutionLevel = WdfExecutionLevelInheritFromParent;
    WaitLockAttrib.SynchronizationScope =
        WdfSynchronizationScopeInheritFromParent;
    WaitLockAttrib.ParentObject = Device;
    Status = WdfWaitLockCreate(&WaitLockAttrib, &DeviceContext->DeviceWaitLock);
    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice failed to WdfWaitLockCreate: %!STATUS!\n", Status);
      goto Exit;
    }

    UCM_MANAGER_CONFIG_INIT(&UcmConfig);
    Status = UcmInitializeDevice(Device, &UcmConfig);
    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice Exit: 0x%x\n", Status);
      goto Exit;
    }

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
        &idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeoutType = DriverManagedIdleTimeout;
    idleSettings.Enabled         = WdfFalse;

    Status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
    if (!NT_SUCCESS(Status)) {
      TraceEvents(
          TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
          "LumiaUSBCKmCreateDevice Exit: 0x%x\n", Status);
      goto Exit;
    }
  }

Exit:
  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
      "LumiaUSBCKmCreateDevice Exit: 0x%x\n", Status);
  return Status;
}
