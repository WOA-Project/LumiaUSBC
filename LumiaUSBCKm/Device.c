/*++

Module Name:

	device.c - Device handling events for the UC120 driver.

Abstract:

   This file contains the device entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include <wdf.h>

#include "GPIO.h"
#include "SPI.h"
#include "UC120.h"
#include "Registry.h"
#include "WorkItems.h"

#ifndef DBG_PRINT_EX_LOGGING
#include "device.tmh"
#endif

EVT_WDF_DEVICE_PREPARE_HARDWARE LumiaUSBCDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE LumiaUSBCDeviceReleaseHardware;
EVT_UCM_CONNECTOR_SET_DATA_ROLE LumiaUSBCSetDataRole;
EVT_WDF_DEVICE_D0_ENTRY LumiaUSBCDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT LumiaUSBCDeviceD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT LumiaUSBCSelfManagedIoInit;

NTSTATUS LumiaUSBCKmCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);

// Interrupt routines
EVT_WDF_INTERRUPT_ISR EvtInterruptIsr;

NTSTATUS Uc120InterruptEnable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice);
NTSTATUS Uc120InterruptDisable(WDFINTERRUPT Interrupt, WDFDEVICE AssociatedDevice);

// Interrupt work items
void    Uc120InterruptWorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);
void    PlugDetInterruptWorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);
void    Mystery1InterruptWorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);
void    Mystery2InterruptWorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);

// IO routines
NTSTATUS OpenIOTarget(PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use, WDFIOTARGET* target);
NTSTATUS LumiaUSBCProbeResources(PDEVICE_CONTEXT DeviceContext, WDFCMRESLIST ResourcesTranslated, WDFCMRESLIST ResourcesRaw);
NTSTATUS LumiaUSBCOpenResources(PDEVICE_CONTEXT ctx);
void LumiaUSBCCloseResources(PDEVICE_CONTEXT ctx);


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LumiaUSBCKmCreateDevice)
#pragma alloc_text (PAGE, LumiaUSBCDevicePrepareHardware)
#pragma alloc_text (PAGE, LumiaUSBCSetDataRole)
#endif

NTSTATUS
LumiaUSBCSetDataRole(
	UCMCONNECTOR  Connector,
	UCM_DATA_ROLE  DataRole
)
{
	PCONNECTOR_CONTEXT connCtx;

	//UNREFERENCED_PARAMETER(DataRole);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetDataRole Entry");

	connCtx = ConnectorGetContext(Connector);

	RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"DataRoleRequested",
		REG_DWORD,
		&DataRole,
		sizeof(ULONG));

	UcmConnectorDataDirectionChanged(Connector, TRUE, DataRole);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetDataRole Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
LumiaUSBCSetPowerRole(
	UCMCONNECTOR Connector,
	UCM_POWER_ROLE PowerRole
)
{
	PCONNECTOR_CONTEXT connCtx;

	//UNREFERENCED_PARAMETER(PowerRole);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetPowerRole Entry");

	connCtx = ConnectorGetContext(Connector);

	RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"PowerRoleRequested",
		REG_DWORD,
		&PowerRole,
		sizeof(ULONG));

	UcmConnectorPowerDirectionChanged(Connector, TRUE, PowerRole);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetPowerRole Exit");

	return STATUS_SUCCESS;
}

BOOLEAN EvtInterruptIsr(
	WDFINTERRUPT Interrupt,
	ULONG MessageID
)
{
	UNREFERENCED_PARAMETER(MessageID);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtInterruptIsr Entry");

	WdfInterruptQueueWorkItemForIsr(Interrupt);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "EvtInterruptIsr Exit");

	return TRUE;
}

NTSTATUS Uc120InterruptEnable(
	WDFINTERRUPT Interrupt,
	WDFDEVICE AssociatedDevice
)
{
	NTSTATUS status = STATUS_SUCCESS;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Uc120InterruptEnable Entry");

	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedDevice);
	UNREFERENCED_PARAMETER(Interrupt);

	status = UC120_InterruptsEnable(ctx);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Uc120InterruptEnable Exit");

	return status;
}

NTSTATUS Uc120InterruptDisable(
	WDFINTERRUPT Interrupt,
	WDFDEVICE AssociatedDevice
)
{
	NTSTATUS status = STATUS_SUCCESS;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Uc120InterruptDisable Entry");

	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedDevice);
	UNREFERENCED_PARAMETER(Interrupt);

	status = UC120_InterruptsDisable(ctx);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Uc120InterruptDisable Exit");

	return status;
}

NTSTATUS OpenIOTarget(PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use, WDFIOTARGET* target)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDF_IO_TARGET_OPEN_PARAMS OpenParams;
	UNICODE_STRING ReadString;
	WCHAR ReadStringBuffer[260];

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget Entry");

	RtlInitEmptyUnicodeString(&ReadString,
		ReadStringBuffer,
		sizeof(ReadStringBuffer));

	status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString,
		res.LowPart,
		res.HighPart);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "RESOURCE_HUB_CREATE_PATH_FROM_ID failed 0x%x", status);
		goto Exit;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = ctx->Device;

	status = WdfIoTargetCreate(ctx->Device, &ObjectAttributes, target);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfIoTargetCreate failed 0x%x", status);
		goto Exit;
	}

	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams, &ReadString, use);
	status = WdfIoTargetOpen(*target, &OpenParams);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfIoTargetOpen failed 0x%x", status);
		goto Exit;
	}

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget Exit");
	return status;
}

NTSTATUS
LumiaUSBCProbeResources(
	PDEVICE_CONTEXT DeviceContext,
	WDFCMRESLIST ResourcesTranslated,
	WDFCMRESLIST ResourcesRaw
)
{
	PAGED_CODE();

	NTSTATUS status = STATUS_SUCCESS;
	WDF_INTERRUPT_CONFIG interruptConfig;
	WDF_INTERRUPT_CONFIG interruptConfig2;
	WDF_INTERRUPT_CONFIG interruptConfig3;
	WDF_INTERRUPT_CONFIG interruptConfig4;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor = NULL;

	BOOLEAN spiFound = FALSE;
	ULONG gpioFound = 0;
	ULONG interruptFound = 0;

	ULONG PlugDetInterrupt = 0;
	ULONG UC120Interrupt = 0;
	ULONG MysteryInterrupt1 = 0;
	ULONG MysteryInterrupt2 = 0;

	ULONG resourceCount;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "-> LumiaUSBCProbeResources");

	resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);

	for (ULONG i = 0; i < resourceCount; i++)
	{
		descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

		switch (descriptor->Type)
		{
		case CmResourceTypeConnection:
			// Check for GPIO resource
			if (descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_GPIO &&
				descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_GPIO_IO)
			{
				switch (gpioFound) {
				case 0:
					DeviceContext->VbusGpioId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->VbusGpioId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				case 1:
					DeviceContext->PolGpioId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->PolGpioId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				case 2:
					DeviceContext->AmselGpioId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->AmselGpioId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				case 3:
					DeviceContext->EnGpioId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->EnGpioId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				default:
					break;
				}

				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Found GPIO resource id=%lu index=%lu", gpioFound, i);

				gpioFound++;
			}
			// Check for SPI resource
			else if (descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
				descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI)
			{
				DeviceContext->SpiId.LowPart = descriptor->u.Connection.IdLowPart;
				DeviceContext->SpiId.HighPart = descriptor->u.Connection.IdHighPart;

				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Found SPI resource index=%lu", i);

				spiFound = TRUE;
			}
			break;

		case CmResourceTypeInterrupt:
			// We've found an interrupt resource.
			
			switch (interruptFound)
			{
			case 0:
				PlugDetInterrupt = i;
				break;
			case 1:
				UC120Interrupt = i;
				break;
			case 2:
				MysteryInterrupt1 = i;
				break;
			case 3:
				MysteryInterrupt2 = i;
				break;
			default:
				break;
			}

			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Found Interrupt resource id=%lu index=%lu", interruptFound, i);

			interruptFound++;
			break;

		default:
			// We don't care about other descriptors.
			break;
		}
	}

	if (!spiFound || gpioFound < 4 || interruptFound < 4)
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Not all resources were found");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	WDF_INTERRUPT_CONFIG_INIT(&interruptConfig, EvtInterruptIsr, NULL);

	interruptConfig.PassiveHandling = TRUE;
	interruptConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(ResourcesTranslated, UC120Interrupt);
	interruptConfig.InterruptRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, UC120Interrupt);
	interruptConfig.EvtInterruptWorkItem = Uc120InterruptWorkItem;

	interruptConfig.EvtInterruptEnable = Uc120InterruptEnable;
	interruptConfig.EvtInterruptDisable = Uc120InterruptDisable;

	status = WdfInterruptCreate(
		DeviceContext->Device,
		&interruptConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DeviceContext->Uc120Interrupt
	);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfInterruptCreate failed for UC120 interrupt, 0x%x", status);
		goto Exit;
	}

	WDF_INTERRUPT_CONFIG_INIT(&interruptConfig2, EvtInterruptIsr, NULL);

	interruptConfig2.PassiveHandling = TRUE;
	interruptConfig2.InterruptTranslated = WdfCmResourceListGetDescriptor(ResourcesTranslated, PlugDetInterrupt);
	interruptConfig2.InterruptRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, PlugDetInterrupt);
	interruptConfig2.EvtInterruptWorkItem = PlugDetInterruptWorkItem;

	status = WdfInterruptCreate(
		DeviceContext->Device,
		&interruptConfig2,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DeviceContext->PlugDetectInterrupt
	);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfInterruptCreate failed for plug detection, 0x%x", status);
		goto Exit;
	}

	WDF_INTERRUPT_CONFIG_INIT(&interruptConfig3, EvtInterruptIsr, NULL);

	interruptConfig3.PassiveHandling = TRUE;
	interruptConfig3.InterruptTranslated = WdfCmResourceListGetDescriptor(ResourcesTranslated, MysteryInterrupt1);
	interruptConfig3.InterruptRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, MysteryInterrupt1);
	interruptConfig3.EvtInterruptWorkItem = Mystery1InterruptWorkItem;

	status = WdfInterruptCreate(
		DeviceContext->Device,
		&interruptConfig3,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DeviceContext->MysteryInterrupt1
	);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfInterruptCreate failed for mystery 1 (VBUS), 0x%x", status);
		goto Exit;
	}

	WDF_INTERRUPT_CONFIG_INIT(&interruptConfig4, EvtInterruptIsr, NULL);

	interruptConfig4.PassiveHandling = TRUE;
	interruptConfig4.InterruptTranslated = WdfCmResourceListGetDescriptor(ResourcesTranslated, MysteryInterrupt2);
	interruptConfig4.InterruptRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, MysteryInterrupt2);
	interruptConfig4.EvtInterruptWorkItem = Mystery2InterruptWorkItem;

	status = WdfInterruptCreate(
		DeviceContext->Device,
		&interruptConfig4,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DeviceContext->MysteryInterrupt2
	);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfInterruptCreate failed for mystery 2, 0x%x", status);
		goto Exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<- LumiaUSBCProbeResources");

Exit:
	return status;
}

NTSTATUS
LumiaUSBCOpenResources(
	PDEVICE_CONTEXT ctx
)
{
	NTSTATUS status = STATUS_SUCCESS;
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources Entry");

	status = OpenIOTarget(ctx, ctx->SpiId, GENERIC_READ | GENERIC_WRITE, &ctx->Spi);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget failed for SPI 0x%x Falling back to fake SPI.", status);
		goto Exit;
	}

	status = OpenIOTarget(ctx, ctx->VbusGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->VbusGpio);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget failed for VBUS GPIO 0x%x", status);
		goto Exit;
	}

	status = OpenIOTarget(ctx, ctx->PolGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->PolGpio);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget failed for polarity GPIO 0x%x", status);
		goto Exit;
	}

	status = OpenIOTarget(ctx, ctx->AmselGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->AmselGpio);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget failed for alternate mode selection GPIO 0x%x", status);
		goto Exit;
	}

	status = OpenIOTarget(ctx, ctx->EnGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->EnGpio);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "OpenIOTarget failed for mux enable GPIO 0x%x", status);
		goto Exit;
	}

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources Exit");
	return status;
}

void
LumiaUSBCCloseResources(
	PDEVICE_CONTEXT ctx
)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCCloseResources Entry");

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

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCCloseResources Exit");
}

NTSTATUS LumiaUSBCDeviceD0Exit(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE TargetState
)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Exit Entry");

	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);

	switch (TargetState)
	{
	case WdfPowerDeviceInvalid:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceInvalid");
		break;
	}
	case WdfPowerDeviceD0:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD0");
		break;
	}
	case WdfPowerDeviceD1:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD1");
		break;
	}
	case WdfPowerDeviceD2:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD2");
		break;
	}
	case WdfPowerDeviceD3:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD3");
		break;
	}
	case WdfPowerDeviceD3Final:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceD3Final");
		break;
	}
	case WdfPowerDevicePrepareForHibernation:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDevicePrepareForHibernation");
		break;
	}
	case WdfPowerDeviceMaximum:
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "WdfPowerDeviceMaximum");
		break;
	}
	}

	LumiaUSBCCloseResources(devCtx);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Exit Exit");
	return STATUS_SUCCESS;
}

NTSTATUS LumiaUSBCDeviceReleaseHardware(
	WDFDEVICE Device,
	WDFCMRESLIST ResourcesTranslated
)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceReleaseHardware Entry");

	UNREFERENCED_PARAMETER((Device, ResourcesTranslated));

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceReleaseHardware Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
LumiaUSBCDevicePrepareHardware(
	WDFDEVICE Device,
	WDFCMRESLIST ResourcesRaw,
	WDFCMRESLIST ResourcesTranslated
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devCtx;
	UCM_MANAGER_CONFIG ucmCfg;
	UCM_CONNECTOR_CONFIG connCfg;
	UCM_CONNECTOR_TYPEC_CONFIG typeCConfig;
	UCM_CONNECTOR_PD_CONFIG pdConfig;
	WDF_OBJECT_ATTRIBUTES attr;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDevicePrepareHardware Entry");

	devCtx = DeviceGetContext(Device);

	status = LumiaUSBCProbeResources(devCtx, ResourcesTranslated, ResourcesRaw);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCProbeResources failed 0x%x\n", status);
		goto Exit;
	}

	if (devCtx->Connector)
	{
		goto Exit;
	}

	//
	// Initialize UCM Manager
	//
	UCM_MANAGER_CONFIG_INIT(&ucmCfg);

	status = UcmInitializeDevice(Device, &ucmCfg);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UcmInitializeDevice failed 0x%x\n", status);
		goto Exit;
	}

	//
	// Assemble the Type-C and PD configuration for UCM.
	//

	UCM_CONNECTOR_CONFIG_INIT(&connCfg, 0);

	UCM_CONNECTOR_TYPEC_CONFIG_INIT(
		&typeCConfig,
		UcmTypeCOperatingModeDrp | UcmTypeCOperatingModeUfp | UcmTypeCOperatingModeDfp,
		UcmTypeCCurrent3000mA | UcmTypeCCurrent1500mA | UcmTypeCCurrentDefaultUsb
	);

	typeCConfig.EvtSetDataRole = LumiaUSBCSetDataRole;
	typeCConfig.AudioAccessoryCapable = FALSE;

	UCM_CONNECTOR_PD_CONFIG_INIT(&pdConfig, UcmPowerRoleSink | UcmPowerRoleSource);

	pdConfig.EvtSetPowerRole = LumiaUSBCSetPowerRole;

	connCfg.TypeCConfig = &typeCConfig;
	connCfg.PdConfig = &pdConfig;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, CONNECTOR_CONTEXT);

	//
	// Create the UCM connector object.
	//

	status = UcmConnectorCreate(Device, &connCfg, &attr, &devCtx->Connector);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UcmConnectorCreate failed 0x%x\n", status);
		goto Exit;
	}

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDevicePrepareHardware Exit");
	return status;
}

NTSTATUS LumiaUSBCDeviceD0Entry(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE PreviousState
)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(PreviousState);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Entry Entry");

	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);

	status = LumiaUSBCOpenResources(devCtx);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCOpenResources failed 0x%x\n", status);
		goto Exit;
	}

	unsigned char value = (unsigned char)0;
	SetGPIO(devCtx, devCtx->PolGpio, &value);

	value = (unsigned char)0;
	SetGPIO(devCtx, devCtx->AmselGpio, &value); // high = HDMI only, medium (unsupported) = USB only, low = both

	value = (unsigned char)1;
	SetGPIO(devCtx, devCtx->EnGpio, &value);

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCDeviceD0Entry Exit");
	return status;
}

NTSTATUS LumiaUSBCSelfManagedIoInit(
	WDFDEVICE Device
)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG input[8], output[6];
	LARGE_INTEGER delay;

	PO_FX_DEVICE poFxDevice;
	PO_FX_COMPONENT_IDLE_STATE idleState;

	UNICODE_STRING		CalibrationFileString;
	OBJECT_ATTRIBUTES	CalibrationFileObjectAttribute;
	HANDLE				hCalibrationFile;
	IO_STATUS_BLOCK		CalibrationIoStatusBlock;
	UCHAR				CalibrationBlob[UC120_CALIBRATIONFILE_SIZE + 2];
	LARGE_INTEGER		CalibrationFileByteOffset;
	FILE_STANDARD_INFORMATION CalibrationFileInfo;
	// { 0x0C, 0x7C, 0x31, 0x5E, 0x9D, 0x0D, 0x7D, 0x32, 0x5F, 0x9E };
	UCHAR				DefaultCalibrationBlob[] = { 0x0C, 0x7C, 0x31, 0x5E, 0x9D, 0x0A, 0x7A, 0x2F, 0x5C, 0x9B };

	LONGLONG			CalibrationFileSize = 0;

	BOOLEAN             SkipCalibration = FALSE;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSelfManagedIoInit Entry");

	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);

	memset(&poFxDevice, 0, sizeof(poFxDevice));
	memset(&idleState, 0, sizeof(idleState));
	poFxDevice.Version = PO_FX_VERSION_V1;
	poFxDevice.ComponentCount = 1;
	poFxDevice.Components[0].IdleStateCount = 1;
	poFxDevice.Components[0].IdleStates = &idleState;
	poFxDevice.DeviceContext = devCtx;
	idleState.NominalPower = PO_FX_UNKNOWN_POWER;

	status = PoFxRegisterDevice(WdfDeviceWdmGetPhysicalDevice(Device), &poFxDevice, &devCtx->PoHandle);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "PoFxRegisterDevice failed 0x%x", status);
		goto Exit;
	}

	PoFxActivateComponent(devCtx->PoHandle, 0, PO_FX_FLAG_BLOCKING);

	PoFxStartDevicePowerManagement(devCtx->PoHandle);

	// Tell PEP to turn on the clock
	memset(input, 0, sizeof(input));
	input[0] = 2;
	input[7] = 2;
	status = PoFxPowerControl(devCtx->PoHandle, &PowerControlGuid, &input, sizeof(input), &output, sizeof(output), NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "PoFxPowerControl failed 0x%x", status);
		goto Exit;
	}

	// Read calibration file
	RtlInitUnicodeString(&CalibrationFileString, L"\\DosDevices\\C:\\DPP\\MMO\\ice5lp_2k_cal.bin");
	InitializeObjectAttributes(&CalibrationFileObjectAttribute, &CalibrationFileString,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL
	);

	// Should not happen
	if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
		status = STATUS_INVALID_DEVICE_STATE;
		goto Exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "acquire calibration file handle");
	status = ZwCreateFile(&hCalibrationFile,
		GENERIC_READ,
		&CalibrationFileObjectAttribute, 
		&CalibrationIoStatusBlock, NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "failed to open calibration file 0x%x, skipping calibration. Is this a FUSBC device?", status);
		status = STATUS_SUCCESS;
		SkipCalibration = TRUE;
	}

	if (!SkipCalibration)
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "stat calibration file");
		status = ZwQueryInformationFile(
			hCalibrationFile,
			&CalibrationIoStatusBlock,
			&CalibrationFileInfo,
			sizeof(CalibrationFileInfo),
			FileStandardInformation
		);

		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "failed to stat calibration file 0x%x", status);
			ZwClose(hCalibrationFile);
			goto Exit;
		}

		CalibrationFileSize = CalibrationFileInfo.EndOfFile.QuadPart;
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "calibration file size %lld", CalibrationFileSize);

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "read calibration file");
		RtlZeroMemory(CalibrationBlob, sizeof(CalibrationBlob));
		CalibrationFileByteOffset.LowPart = 0;
		CalibrationFileByteOffset.HighPart = 0;
		status = ZwReadFile(hCalibrationFile,
			NULL, NULL, NULL, &CalibrationIoStatusBlock,
			CalibrationBlob, UC120_CALIBRATIONFILE_SIZE, &CalibrationFileByteOffset, NULL
		);

		ZwClose(hCalibrationFile);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "failed to read calibration file 0x%x", status);
			goto Exit;
		}
	}

	status = UC120_D0Entry(devCtx);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_D0Entry failed 0x%x", status);
		goto Exit;
	}

	if (!SkipCalibration)
	{
		// Initialize the UC120 accordingly
		if (CalibrationFileSize == 11) {
			// Skip the first byte
			status = UC120_UploadCalibrationData(devCtx, &CalibrationBlob[1], 10);
		}
		else if (CalibrationFileSize == 8) {
			// No skip
			status = UC120_UploadCalibrationData(devCtx, CalibrationBlob, 8);
		}
		else {
			// Not recognized, use default
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Unknown calibration data, fallback to the default");
			status = UC120_UploadCalibrationData(devCtx, DefaultCalibrationBlob, sizeof(DefaultCalibrationBlob));
		}

		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_UploadCalibrationData failed 0x%x", status);
			goto Exit;
		}
	}

	delay.QuadPart = -2000000;
	KeDelayExecutionThread(UserMode, TRUE, &delay);

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSelfManagedIoInit Exit");
	return status;
}

NTSTATUS
LumiaUSBCKmCreateDevice(
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
/*++

Routine Description:

	Worker routine called to create a device and its software resources.

Arguments:

	DeviceInit - Pointer to an opaque init structure. Memory for this
					structure will be freed by the framework when the WdfDeviceCreate
					succeeds. So don't access the structure after that point.

Return Value:

	NTSTATUS

--*/
{
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	PDEVICE_CONTEXT deviceContext;
	WDFDEVICE device;
	UCM_MANAGER_CONFIG ucmConfig;
	NTSTATUS status;

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCKmCreateDevice Entry");

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
	pnpPowerCallbacks.EvtDevicePrepareHardware = LumiaUSBCDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = LumiaUSBCDeviceReleaseHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = LumiaUSBCDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = LumiaUSBCDeviceD0Exit;
	pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = LumiaUSBCSelfManagedIoInit;
	//pnpPowerCallbacks.EvtDeviceSelfManagedIoRestart = Fdo_EvtDeviceSelfManagedIoRestart;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

	if (NT_SUCCESS(status)) {
		//
		// Get a pointer to the device context structure that we just associated
		// with the device object. We define this structure in the device.h
		// header file. DeviceGetContext is an inline function generated by
		// using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
		// This function will do the type checking and return the device context.
		// If you pass a wrong object handle it will return NULL and assert if
		// run under framework verifier mode.
		//
		deviceContext = DeviceGetContext(device);

		//
		// Initialize the context.
		//
		deviceContext->Device = device;
		deviceContext->Connector = NULL;

		UCM_MANAGER_CONFIG_INIT(&ucmConfig);
		status = UcmInitializeDevice(device, &ucmConfig);
		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCKmCreateDevice Exit: 0x%x\n", status);
			goto Exit;
		}

		/*WDF_POWER_FRAMEWORK_SETTINGS poFxSettings;

		WDF_POWER_FRAMEWORK_SETTINGS_INIT(&poFxSettings);

		poFxSettings.EvtDeviceWdmPostPoFxRegisterDevice = LumiaUSBCPostPoFxRegisterDevice;
		poFxSettings.ComponentActiveConditionCallback = LumiaUSBCActiveConditionCallback;

		poFxSettings.Component = NULL;// &component;
		poFxSettings.PoFxDeviceContext = (PVOID)device;

		status = WdfDeviceWdmAssignPowerFrameworkSettings(device, &poFxSettings);
		if (!NT_SUCCESS(status)) {
			return status;
		}*/

		WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS  idleSettings;

		WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
			&idleSettings,
			IdleCannotWakeFromS0
		);
		idleSettings.IdleTimeoutType = DriverManagedIdleTimeout;
		idleSettings.Enabled = WdfFalse;

		status = WdfDeviceAssignS0IdleSettings(
			device,
			&idleSettings
		);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCKmCreateDevice Exit: 0x%x\n", status);
			goto Exit;
		}

		//
		// Create a device interface so that applications can find and talk
		// to us.
		//
		/*status = WdfDeviceCreateDeviceInterface(
			device,
			&GUID_DEVINTERFACE_LumiaUSBCKm,
			NULL // ReferenceString
			);

		if (NT_SUCCESS(status)) {
			//
			// Initialize the I/O Package and any Queues
			//
			status = LumiaUSBCKmQueueInitialize(device);
		}*/
	}

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCKmCreateDevice Exit: 0x%x\n", status);
	return status;
}
