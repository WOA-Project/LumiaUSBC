/*++

Module Name:

	device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include <gpio.h>
#include <wdf.h>


EVT_WDF_DEVICE_PREPARE_HARDWARE LumiaUSBCDevicePrepareHardware;
EVT_UCM_CONNECTOR_SET_DATA_ROLE     LumiaUSBCSetDataRole;
EVT_WDF_DEVICE_D0_ENTRY LumiaUSBCDeviceD0Entry;

NTSTATUS ReadRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length);
NTSTATUS WriteRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length);
NTSTATUS GetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char* value);
NTSTATUS SetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char* value);

BOOLEAN EvtInterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID);
void Uc120InterruptWorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);
void PlugDetInterruptWorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);
NTSTATUS LumiaUSBCSelfManagedIoInit(WDFDEVICE Device);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LumiaUSBCKmCreateDevice)
#pragma alloc_text (PAGE, LumiaUSBCDevicePrepareHardware)
#pragma alloc_text (PAGE, LumiaUSBCSetDataRole)

#pragma alloc_text (PAGE, EvtInterruptIsr)
#pragma alloc_text (PAGE, Uc120InterruptWorkItem)
#pragma alloc_text (PAGE, PlugDetInterruptWorkItem)
#endif

#pragma region UC120 Communication

NTSTATUS UC120_GetCurrentRegisters(PDEVICE_CONTEXT deviceContext, DWORD context)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char value = 0;
	wchar_t buf[260];

	PCWSTR contextStr = L"REG";
	if (context == 0)
	{
		contextStr = L"INIT";
	}
	else if (context == 1)
	{
		contextStr = L"PLUGDET";
	}
	else if (context == 2)
	{
		contextStr = L"UC120";
	}

	unsigned char registers[8];

	memset(registers, 0, sizeof(registers));

	status = ReadRegister(deviceContext, 0, registers + 0, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 1, registers + 1, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 2, registers + 2, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 5, registers + 3, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 7, registers + 4, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 9, registers + 5, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 10, registers + 6, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 11, registers + 7, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	swprintf(buf, L"%ls_%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", contextStr, registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);
	DbgPrint("LumiaUSBC: %ls\n", buf);

	value = 0;
	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&value,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	return status;
}

NTSTATUS UC120_GetCurrentState(PDEVICE_CONTEXT deviceContext, DWORD context)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;
	wchar_t buf[260];
	ULONG outgoingMessageSize, incomingMessageSize, isCableConnected, newPowerRole, newDataRole, vconnRoleSwitch = 0;

	PCWSTR contextStr = L"REG";
	if (context == 0)
	{
		contextStr = L"INIT";
	}
	else if (context == 1)
	{
		contextStr = L"PLUGDET";
	}
	else if (context == 2)
	{
		contextStr = L"UC120";
	}

	unsigned char registers[8];

	memset(registers, 0, sizeof(registers));

	status = ReadRegister(deviceContext, 0, registers + 0, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 1, registers + 1, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 2, registers + 2, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 5, registers + 3, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 7, registers + 4, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 9, registers + 5, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 10, registers + 6, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 11, registers + 7, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	swprintf(buf, L"%ls_%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", contextStr, registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);
	DbgPrint("LumiaUSBC: %ls\n", buf);

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	/*status = ReadRegister(deviceContext, 0, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}*/

	data = registers[0];
	outgoingMessageSize = data & 31u; // 0-4

	/*status = ReadRegister(deviceContext, 1, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}*/

	data = registers[1];
	incomingMessageSize = data & 31u; // 0-4

	/*status = ReadRegister(deviceContext, 2, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}*/

	data = registers[2];
	isCableConnected = (data & 60u) >> 2u; // 2-5

	/*status = ReadRegister(deviceContext, 5, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}*/

	data = registers[3];
	newPowerRole = (unsigned int)data & 1u; // 0
	newDataRole = (((unsigned int)data >> 2) & 1u); // 2
	vconnRoleSwitch = (((unsigned int)data >> 5) & 1u); // 5

	swprintf(buf, L"%ls_%05x-%05x-%04x-%01x-%01x-%01x", contextStr, outgoingMessageSize, incomingMessageSize, isCableConnected, newPowerRole, newDataRole, vconnRoleSwitch);
	DbgPrint("LumiaUSBC: %ls\n", buf);

	data = 0;
	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"NewPowerRole",
		REG_DWORD,
		&newPowerRole,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"NewDataRole",
		REG_DWORD,
		&newDataRole,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"VconnRoleSwitch",
		REG_DWORD,
		&vconnRoleSwitch,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	return status;
}

NTSTATUS UC120_InterruptHandled(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	data = 0xFF; // Clear to 0xFF
	status = WriteRegister(deviceContext, 2, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	return status;
}

NTSTATUS UC120_InterruptsEnable(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	data = 0xFF; // Clear to 0xFF
	status = WriteRegister(deviceContext, 2, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = WriteRegister(deviceContext, 3, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 4, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	data |= 1; // Turn on bit 1
	status = WriteRegister(deviceContext, 4, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 5, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	data &= ~0x80; // Clear bit 7
	status = WriteRegister(deviceContext, 5, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	return status;
}

NTSTATUS UC120_InterruptsDisable(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	status = ReadRegister(deviceContext, 4, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	data &= ~1; // Turn off bit 1
	status = WriteRegister(deviceContext, 4, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	return status;
}

NTSTATUS UC120_D0Entry(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	status = ReadRegister(deviceContext, 4, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	data |= 6; // Turn on bit 1 and 2
	status = WriteRegister(deviceContext, 4, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 5, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	data |= 88; // Turn on bit 3 and 7
	status = WriteRegister(deviceContext, 5, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 13, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	data |= 2; // Turn on bit 1
	data &= ~1; // Turn off bit 0
	status = WriteRegister(deviceContext, 13, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	return status;
}

NTSTATUS UC120_UploadCalibrationData(PDEVICE_CONTEXT deviceContext, unsigned char* calibrationFile, DWORD length)
{
	NTSTATUS status = STATUS_SUCCESS;
	switch (length)
	{
	case 10:
	{
		status = WriteRegister(deviceContext, 18, calibrationFile + 0, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 19, calibrationFile + 1, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 20, calibrationFile + 2, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 21, calibrationFile + 3, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 26, calibrationFile + 4, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 22, calibrationFile + 5, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 23, calibrationFile + 6, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 24, calibrationFile + 7, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 25, calibrationFile + 8, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 27, calibrationFile + 9, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		break;
	}
	case 8:
	{
		status = WriteRegister(deviceContext, 18, calibrationFile + 0, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 19, calibrationFile + 1, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 20, calibrationFile + 2, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 21, calibrationFile + 3, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 22, calibrationFile + 4, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 23, calibrationFile + 5, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 24, calibrationFile + 6, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		status = WriteRegister(deviceContext, 25, calibrationFile + 7, 1);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
		break;
	}
	default:
	{
		status = STATUS_BAD_DATA;
		goto Exit;
	}
	}

Exit:
	return status;
}

#pragma endregion

NTSTATUS
LumiaUSBCSetDataRole(
	UCMCONNECTOR  Connector,
	UCM_DATA_ROLE  DataRole
)
{
	PCONNECTOR_CONTEXT connCtx;

	UNREFERENCED_PARAMETER(DataRole);

	connCtx = ConnectorGetContext(Connector);

	return STATUS_SUCCESS;
}

BOOLEAN EvtInterruptIsr(
	WDFINTERRUPT Interrupt,
	ULONG MessageID
)
{
	UNREFERENCED_PARAMETER(MessageID);

	WdfInterruptQueueWorkItemForIsr(Interrupt);

	return TRUE;
}

void Uc120InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	DbgPrint("LumiaUSBC: Got an interrupt from the UC120!\n");

	//UC120_GetCurrentRegisters(ctx, 2);
	UC120_GetCurrentState(ctx, 2);
	UC120_InterruptHandled(ctx);
}

void PlugDetInterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	DbgPrint("LumiaUSBC: Got an interrupt from PLUGDET!\n");

	//UC120_GetCurrentRegisters(ctx, 1);
	UC120_GetCurrentState(ctx, 1);
	UC120_InterruptHandled(ctx);
}

NTSTATUS Uc120InterruptEnable(
	WDFINTERRUPT Interrupt,
	WDFDEVICE AssociatedDevice
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedDevice);
	UNREFERENCED_PARAMETER(Interrupt);

	status = UC120_InterruptsEnable(ctx);

	return status;
}

NTSTATUS Uc120InterruptDisable(
	WDFINTERRUPT Interrupt,
	WDFDEVICE AssociatedDevice
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedDevice);
	UNREFERENCED_PARAMETER(Interrupt);

	status = UC120_InterruptsDisable(ctx);

	return status;
}

NTSTATUS OpenIOTarget(PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use, WDFIOTARGET* target)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDF_IO_TARGET_OPEN_PARAMS OpenParams;
	UNICODE_STRING ReadString;
	WCHAR ReadStringBuffer[260];

	DbgPrint("LumiaUSBC: OpenIOTarget Entry\n");

	RtlInitEmptyUnicodeString(&ReadString,
		ReadStringBuffer,
		sizeof(ReadStringBuffer));

	status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString,
		res.LowPart,
		res.HighPart);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: RESOURCE_HUB_CREATE_PATH_FROM_ID failed %x\n", status);
		return status;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = ctx->Device;

	status = WdfIoTargetCreate(ctx->Device, &ObjectAttributes, target);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: WdfIoTargetCreate failed %x\n", status);
		return status;
	}

	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams, &ReadString, use);
	status = WdfIoTargetOpen(*target, &OpenParams);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: WdfIoTargetOpen failed %x\n", status);
	}

	DbgPrint("LumiaUSBC: OpenIOTarget Exit\n");
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

	DeviceContext->HaveResetGpio = FALSE;
	DeviceContext->UseFakeSpi = FALSE;

	NTSTATUS status = STATUS_SUCCESS;
	WDF_INTERRUPT_CONFIG interruptConfig;
	WDF_INTERRUPT_CONFIG interruptConfig2;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor = NULL;

	BOOLEAN spiFound = FALSE;
	ULONG gpioFound = 0;
	ULONG interruptFound = 0;

	ULONG UC120Interrupt = 0;
	ULONG PlugDetInterrupt = 0;

	ULONG resourceCount;

	DbgPrint("LumiaUSBC: LumiaUSBCProbeResources Entry\n");

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
				case 4:
					DeviceContext->ResetGpioId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->ResetGpioId.HighPart = descriptor->u.Connection.IdHighPart;
					DeviceContext->HaveResetGpio = TRUE;
					break;
				case 5:
					DeviceContext->FakeSpiMosiId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->FakeSpiMosiId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				case 6:
					DeviceContext->FakeSpiMisoId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->FakeSpiMisoId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				case 7:
					DeviceContext->FakeSpiCsId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->FakeSpiCsId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				case 8:
					DeviceContext->FakeSpiClkId.LowPart = descriptor->u.Connection.IdLowPart;
					DeviceContext->FakeSpiClkId.HighPart = descriptor->u.Connection.IdHighPart;
					break;
				default:
					break;
				}

				DbgPrint("LumiaUSBC: Found GPIO resource id=%lu index=%lu\n", gpioFound, i);

				gpioFound++;
			}
			// Check for SPI resource
			else if (descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
				descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI)
			{
				DeviceContext->SpiId.LowPart = descriptor->u.Connection.IdLowPart;
				DeviceContext->SpiId.HighPart = descriptor->u.Connection.IdHighPart;

				DbgPrint("LumiaUSBC: Found SPI resource index=%lu\n", i);

				spiFound = TRUE;
			}
			break;

		case CmResourceTypeInterrupt:
			// We've found an interrupt resource.
			
			switch (interruptFound)
			{
			case 2:
				PlugDetInterrupt = i;
				break;
			case 3:
				UC120Interrupt = i;
				break;
			default:
				break;
			}

			DbgPrint("LumiaUSBC: Found Interrupt resource id=%lu index=%lu\n", interruptFound, i);

			interruptFound++;
			break;

		default:
			// We don't care about other descriptors.
			break;
		}
	}

	if (!spiFound && gpioFound >= 8)
	{
		spiFound = TRUE;
		DeviceContext->UseFakeSpi = TRUE;
	}

	if (!spiFound || gpioFound < 4)
	{
		DbgPrint("LumiaUSBC: Not all resources were found, SPI = %d, GPIO = %d, Interrupts = %d\n", spiFound, gpioFound, interruptFound);
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	if (interruptFound < 4)
	{
		DbgPrint("LumiaUSBC: Not all resources were found, SPI = %d, GPIO = %d, Interrupts = %d\n", spiFound, gpioFound, interruptFound);
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	WDF_INTERRUPT_CONFIG_INIT(&interruptConfig, EvtInterruptIsr, NULL);

	interruptConfig.PassiveHandling = TRUE;
	interruptConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(ResourcesTranslated, UC120Interrupt);
	interruptConfig.InterruptRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, UC120Interrupt);

	interruptConfig.EvtInterruptWorkItem = Uc120InterruptWorkItem;
	//interruptConfig.EvtInterruptEnable = Uc120InterruptEnable;
	interruptConfig.EvtInterruptDisable = Uc120InterruptDisable;

	status = WdfInterruptCreate(
		DeviceContext->Device,
		&interruptConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DeviceContext->Uc120Interrupt);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("LumiaUSBC: WdfInterruptCreate failed for UC120 interrupt %x\n", status);
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
		&DeviceContext->PlugDetectInterrupt);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("LumiaUSBC: WdfInterruptCreate failed for plug detection %x\n", status);
		goto Exit;
	}

	DbgPrint("LumiaUSBC: LumiaUSBCProbeResources Exit: %x\n", status);

Exit:
	return status;
}

NTSTATUS
LumiaUSBCOpenResources(
	PDEVICE_CONTEXT ctx
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("LumiaUSBC: LumiaUSBCOpenResources Entry\n");

	if (!(ctx->UseFakeSpi)) {
		status = OpenIOTarget(ctx, ctx->SpiId, GENERIC_READ | GENERIC_WRITE, &ctx->Spi);
		if (!NT_SUCCESS(status)) {
			DbgPrint("LumiaUSBC: OpenIOTarget failed for SPI %x Falling back to fake SPI.\n", status);
			ctx->UseFakeSpi = TRUE;
			status = STATUS_SUCCESS; // return status;
		}
	}

	status = OpenIOTarget(ctx, ctx->VbusGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->VbusGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: OpenIOTarget failed for VBUS GPIO %x\n", status);
		return status;
	}

	status = OpenIOTarget(ctx, ctx->PolGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->PolGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: OpenIOTarget failed for polarity GPIO %x\n", status);
		return status;
	}

	status = OpenIOTarget(ctx, ctx->AmselGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->AmselGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: OpenIOTarget failed for alternate mode selection GPIO %x\n", status);
		return status;
	}

	status = OpenIOTarget(ctx, ctx->EnGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->EnGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: OpenIOTarget failed for mux enable GPIO %x\n", status);
		return status;
	}

	if (ctx->HaveResetGpio) {
		status = OpenIOTarget(ctx, ctx->ResetGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->ResetGpio);
		if (!(NT_SUCCESS(status))) {
			DbgPrint("LumiaUSBC: OpenIOTarget failed for chip reset GPIO %x\n", status);
			ctx->HaveResetGpio = FALSE;
		}
		status = STATUS_SUCCESS; // this GPIO is optional - make sure we never fail on it missing
	}

	if (ctx->UseFakeSpi) {
		status = OpenIOTarget(ctx, ctx->FakeSpiMosiId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiMosi);
		if (!NT_SUCCESS(status)) {
			DbgPrint("LumiaUSBC: OpenIOTarget failed for fake SPI MOSI line %x\n", status);
			return status;
		}
		status = OpenIOTarget(ctx, ctx->FakeSpiMisoId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiMiso);
		if (!NT_SUCCESS(status)) {
			DbgPrint("LumiaUSBC: OpenIOTarget failed for fake SPI MISO line %x\n", status);
			return status;
		}
		status = OpenIOTarget(ctx, ctx->FakeSpiCsId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiCs);
		if (!NT_SUCCESS(status)) {
			DbgPrint("LumiaUSBC: OpenIOTarget failed for fake SPI CS# line %x\n", status);
			return status;
		}
		status = OpenIOTarget(ctx, ctx->FakeSpiClkId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiClk);
		if (!NT_SUCCESS(status)) {
			DbgPrint("LumiaUSBC: OpenIOTarget failed for fake SPI clock line %x\n", status);
			return status;
		}
	}

	DbgPrint("LumiaUSBC: LumiaUSBCOpenResources Exit\n");
	return status;
}

void
LumiaUSBCCloseResources(
	PDEVICE_CONTEXT ctx
)
{
	DbgPrint("LumiaUSBC: LumiaUSBCCloseResources Entry\n");

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

	if (ctx->ResetGpio) {
		WdfIoTargetClose(ctx->ResetGpio);
	}

	if (ctx->FakeSpiClk) {
		WdfIoTargetClose(ctx->FakeSpiClk);
	}

	if (ctx->FakeSpiCs) {
		WdfIoTargetClose(ctx->FakeSpiCs);
	}

	if (ctx->FakeSpiMiso) {
		WdfIoTargetClose(ctx->FakeSpiMiso);
	}

	if (ctx->FakeSpiMosi) {
		WdfIoTargetClose(ctx->FakeSpiMosi);
	}

	DbgPrint("LumiaUSBC: LumiaUSBCCloseResources Exit\n");
}

NTSTATUS LumiaUSBCDeviceD0Exit(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE TargetState
)
{
	DbgPrint("LumiaUSBC: LumiaUSBCDeviceD0Exit Entry\n");
	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);

	switch (TargetState)
	{
	case WdfPowerDeviceInvalid:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceInvalid\n");
		break;
	}
	case WdfPowerDeviceD0:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceD0\n");
		break;
	}
	case WdfPowerDeviceD1:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceD1\n");
		break;
	}
	case WdfPowerDeviceD2:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceD2\n");
		break;
	}
	case WdfPowerDeviceD3:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceD3\n");
		break;
	}
	case WdfPowerDeviceD3Final:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceD3Final\n");
		break;
	}
	case WdfPowerDevicePrepareForHibernation:
	{
		DbgPrint("LumiaUSBC: WdfPowerDevicePrepareForHibernation\n");
		break;
	}
	case WdfPowerDeviceMaximum:
	{
		DbgPrint("LumiaUSBC: WdfPowerDeviceMaximum\n");
		break;
	}
	}

	LumiaUSBCCloseResources(devCtx);

	DbgPrint("LumiaUSBC: LumiaUSBCDeviceD0Exit Exit\n");
	return STATUS_SUCCESS;
}

NTSTATUS LumiaUSBCDeviceReleaseHardware(
	WDFDEVICE Device,
	WDFCMRESLIST ResourcesTranslated
)
{
	UNREFERENCED_PARAMETER((Device, ResourcesTranslated));

	return STATUS_SUCCESS;
}

#define IOCTL_QUP_SPI_CS_MANIPULATION 0x610

#define IOCTL_QUP_SPI_AUTO_CS     CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUP_SPI_ASSERT_CS   CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUP_SPI_DEASSERT_CS CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x0, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS ReadRegisterReal(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_MEMORY_DESCRIPTOR regDescriptor, outputDescriptor;
	unsigned char command = (unsigned char)(reg << 3);

	WdfObjectAcquireLock(ctx->Device);

	status = WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_ASSERT_CS, NULL, NULL, NULL, NULL);
	if (!NT_SUCCESS(status))
	{
		WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);
		WdfObjectReleaseLock(ctx->Device);
		return status;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&regDescriptor, &command, 1);
	status = WdfIoTargetSendWriteSynchronously(ctx->Spi, NULL, &regDescriptor, NULL, NULL, NULL);
	if (!NT_SUCCESS(status))
	{
		WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);
		WdfObjectReleaseLock(ctx->Device);
		return status;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, value, length);

	// need to read 3 times to get the correct value, for some reason
	// NOTE: original driver does a full-duplex transfer here, where the 1st byte starts reading *before*
	// the register ID is written, so possibly we will need 2 here instead!
	for (int i = 0; i < 3; i++) {
		status = WdfIoTargetSendReadSynchronously(ctx->Spi, NULL, &outputDescriptor, NULL, NULL, NULL);

		if (!NT_SUCCESS(status))
		{
			WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);
			WdfObjectReleaseLock(ctx->Device);
			return status;
		}
	}

	status = WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);

	WdfObjectReleaseLock(ctx->Device);

	return status;
}

NTSTATUS WriteRegisterReal(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_MEMORY_DESCRIPTOR regDescriptor, inputDescriptor;
	unsigned char command = (unsigned char)((reg << 3) | 1);

	WdfObjectAcquireLock(ctx->Device);

	status = WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_ASSERT_CS, NULL, NULL, NULL, NULL);
	if (!NT_SUCCESS(status))
	{
		WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);
		WdfObjectReleaseLock(ctx->Device);
		return status;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&regDescriptor, &command, 1);
	status = WdfIoTargetSendWriteSynchronously(ctx->Spi, NULL, &regDescriptor, NULL, NULL, NULL);
	if (!NT_SUCCESS(status))
	{
		WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);
		WdfObjectReleaseLock(ctx->Device);
		return status;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, value, length);
	status = WdfIoTargetSendWriteSynchronously(ctx->Spi, NULL, &inputDescriptor, NULL, NULL, NULL);
	if (!NT_SUCCESS(status))
	{
		WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);
		WdfObjectReleaseLock(ctx->Device);
		return status;
	}

	status = WdfIoTargetSendIoctlSynchronously(ctx->Spi, NULL, IOCTL_QUP_SPI_AUTO_CS, NULL, NULL, NULL, NULL);

	WdfObjectReleaseLock(ctx->Device);

	return status;
}


NTSTATUS GetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char* value)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_MEMORY_DESCRIPTOR outputDescriptor;

	UNREFERENCED_PARAMETER(ctx);

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, value, 1);

	status = WdfIoTargetSendIoctlSynchronously(gpio, NULL, IOCTL_GPIO_READ_PINS, NULL, &outputDescriptor, NULL, NULL);

	return status;
}

NTSTATUS SetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char* value)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_MEMORY_DESCRIPTOR inputDescriptor, outputDescriptor;

	UNREFERENCED_PARAMETER(ctx);

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, value, 1);
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, value, 1);

	status = WdfIoTargetSendIoctlSynchronously(gpio, NULL, IOCTL_GPIO_WRITE_PINS, &inputDescriptor, &outputDescriptor, NULL, NULL);

	return status;
}

NTSTATUS ReadRegisterFake(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
{
	NTSTATUS status;
	unsigned char data;
	unsigned char command = (unsigned char)(reg << 3);
	LARGE_INTEGER delay;
	int i;
	unsigned int j;

	delay.QuadPart = -10; // 1ms

	// Select the chip
	data = 0;
	status = SetGPIO(ctx, ctx->FakeSpiCs, &data);
	if (!NT_SUCCESS(status))
		return status;

	// Send the read command bit by bit, MSB first
	for (i = 7; i >= 0; i--) {
		KeDelayExecutionThread(KernelMode, TRUE, &delay);

		data = (command >> i) & 1;
		status = SetGPIO(ctx, ctx->FakeSpiMosi, &data);
		if (!NT_SUCCESS(status))
			return status;

		data = 0;
		status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
		if (!NT_SUCCESS(status))
			return status;

		KeDelayExecutionThread(KernelMode, TRUE, &delay);

		data = 1;
		status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
		if (!NT_SUCCESS(status))
			return status;
	}

	// Receive the result, MSB first
	for (j = 0; j < length; j++) {
		value[j] = 0;
		for (i = 7; i >= 0; i--) {
			KeDelayExecutionThread(KernelMode, TRUE, &delay);

			data = 0;
			status = GetGPIO(ctx, ctx->FakeSpiMiso, &data);
			if (!NT_SUCCESS(status))
				return status;

			value[j] |= (data & 1) << i;

			data = 0;
			status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
			if (!NT_SUCCESS(status))
				return status;

			KeDelayExecutionThread(KernelMode, TRUE, &delay);

			data = 1;
			status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
			if (!NT_SUCCESS(status))
				return status;
		}
	}

	// Deselect the chip
	data = 1;
	status = SetGPIO(ctx, ctx->FakeSpiCs, &data);
	if (!NT_SUCCESS(status))
		return status;

	return status;
}

NTSTATUS WriteRegisterFake(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
{
	NTSTATUS status;
	unsigned char data;
	unsigned char command = (unsigned char)((reg << 3) | 1);
	LARGE_INTEGER delay;
	int i;
	unsigned int j;

	delay.QuadPart = -10; // 1ms

	// Select the chip
	data = 0;
	status = SetGPIO(ctx, ctx->FakeSpiCs, &data);
	if (!NT_SUCCESS(status))
		return status;

	// Send the write command bit by bit, MSB first
	for (i = 7; i >= 0; i--) {
		KeDelayExecutionThread(KernelMode, TRUE, &delay);

		data = (command >> i) & 1;
		status = SetGPIO(ctx, ctx->FakeSpiMosi, &data);
		if (!NT_SUCCESS(status))
			return status;

		data = 0;
		status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
		if (!NT_SUCCESS(status))
			return status;

		KeDelayExecutionThread(KernelMode, TRUE, &delay);

		data = 1;
		status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
		if (!NT_SUCCESS(status))
			return status;
	}

	// Send the data, MSB 
	for (j = 0; j < length; j++) {
		for (i = 7; i >= 0; i--) {
			KeDelayExecutionThread(KernelMode, TRUE, &delay);

			data = (value[j] >> i) & 1;
			status = SetGPIO(ctx, ctx->FakeSpiMosi, &data);
			if (!NT_SUCCESS(status))
				return status;

			data = 0;
			status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
			if (!NT_SUCCESS(status))
				return status;

			KeDelayExecutionThread(KernelMode, TRUE, &delay);

			data = 1;
			status = SetGPIO(ctx, ctx->FakeSpiClk, &data);
			if (!NT_SUCCESS(status))
				return status;
		}
	}

	// Deselect the chip
	data = 1;
	status = SetGPIO(ctx, ctx->FakeSpiCs, &data);
	if (!NT_SUCCESS(status))
		return status;

	return status;
}

NTSTATUS ReadRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
{
	if (ctx->UseFakeSpi)
		return ReadRegisterFake(ctx, reg, value, length);
	else
		return ReadRegisterReal(ctx, reg, value, length);
}

NTSTATUS WriteRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
{
	if (ctx->UseFakeSpi)
		return WriteRegisterFake(ctx, reg, value, length);
	else
		return WriteRegisterReal(ctx, reg, value, length);
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

	DbgPrint("LumiaUSBC: LumiaUSBCDevicePrepareHardware Entry\n");

	devCtx = DeviceGetContext(Device);

	status = LumiaUSBCProbeResources(devCtx, ResourcesTranslated, ResourcesRaw);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: LumiaUSBCProbeResources failed %x\n", status);
		return status;
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
		DbgPrint("LumiaUSBC: UcmInitializeDevice failed %x\n", status);
		goto Exit;
	}

	//
	// Create a USB Type-C connector #0 with PD
	//
	UCM_CONNECTOR_CONFIG_INIT(&connCfg, 0);

	UCM_CONNECTOR_TYPEC_CONFIG_INIT(
		&typeCConfig,
		UcmTypeCOperatingModeDrp,
		UcmTypeCCurrentDefaultUsb);

	typeCConfig.EvtSetDataRole = LumiaUSBCSetDataRole;

	UCM_CONNECTOR_PD_CONFIG_INIT(&pdConfig, UcmPowerRoleSink | UcmPowerRoleSource);

	connCfg.TypeCConfig = &typeCConfig;
	connCfg.PdConfig = &pdConfig;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, CONNECTOR_CONTEXT);

	status = UcmConnectorCreate(Device, &connCfg, &attr, &devCtx->Connector);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("LumiaUSBC: UcmConnectorCreate failed %x\n", status);
		goto Exit;
	}

	//UcmEventInitialize(&connCtx->EventSetDataRole);
Exit:
	DbgPrint("LumiaUSBC: LumiaUSBCDevicePrepareHardware Exit\n");
	return status;
}

// I can't believe RtlWriteRegistryValue exists, but not RtlReadRegistryValue...
NTSTATUS MyReadRegistryValue(PCWSTR registry_path, PCWSTR value_name, ULONG type,
	PVOID data, ULONG length)
{
	UNICODE_STRING valname;
	UNICODE_STRING keyname;
	OBJECT_ATTRIBUTES attribs;
	PKEY_VALUE_PARTIAL_INFORMATION pinfo;
	HANDLE handle;
	NTSTATUS rc;
	ULONG len, reslen;

	RtlInitUnicodeString(&keyname, registry_path);
	RtlInitUnicodeString(&valname, value_name);

	InitializeObjectAttributes(&attribs, &keyname, OBJ_CASE_INSENSITIVE,
		NULL, NULL);
	rc = ZwOpenKey(&handle, KEY_QUERY_VALUE, &attribs);
	if (!NT_SUCCESS(rc))
		return 0;

	len = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + length;
	pinfo = ExAllocatePool(NonPagedPool, len);
	rc = ZwQueryValueKey(handle, &valname, KeyValuePartialInformation,
		pinfo, len, &reslen);
	if ((NT_SUCCESS(rc) || rc == STATUS_BUFFER_OVERFLOW) &&
		reslen >= (sizeof(KEY_VALUE_PARTIAL_INFORMATION) - 1) &&
		(!type || pinfo->Type == type))
	{
		reslen = pinfo->DataLength;
		memcpy(data, pinfo->Data, min(length, reslen));
	}
	else
		reslen = 0;
	ExFreePool(pinfo);

	ZwClose(handle);
	return rc;
}

NTSTATUS LumiaUSBCDeviceD0Entry(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE PreviousState
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);
	//PCONNECTOR_CONTEXT connCtx = ConnectorGetContext(devCtx->Connector);
	ULONG data = 0;
	UNREFERENCED_PARAMETER(PreviousState);

	DbgPrint("LumiaUSBC: LumiaUSBCDeviceD0Entry Entry\n");

	status = LumiaUSBCOpenResources(devCtx);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: LumiaUSBCOpenResources failed %x\n", status);
		return status;
	}

	UCM_CONNECTOR_TYPEC_ATTACH_PARAMS Params;
	UCM_CONNECTOR_TYPEC_ATTACH_PARAMS_INIT(&Params, UcmTypeCPartnerUfp);
	Params.CurrentAdvertisement = UcmTypeCCurrentDefaultUsb;
	UcmConnectorTypeCAttach(devCtx->Connector, &Params);

	unsigned char value = (unsigned char)0;

	SetGPIO(devCtx, devCtx->PolGpio, &value);
	SetGPIO(devCtx, devCtx->AmselGpio, &value); // high = HDMI only, medium (unsupported) = USB only, low = both
	value = (unsigned char)1;
	SetGPIO(devCtx, devCtx->EnGpio, &value);
	/*
	SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
	if (devCtx->HaveResetGpio) {
		delay.QuadPart = -2000; // 200us
		value = (unsigned char)0;
		SetGPIO(devCtx, devCtx->ResetGpio, &value);
		SetGPIO(devCtx, devCtx->FakeSpiCs, &value);
		KeDelayExecutionThread(KernelMode, TRUE, &delay);
		value = (unsigned char)1;
		SetGPIO(devCtx, devCtx->ResetGpio, &value);

		delay.QuadPart = -12000; // 1200us
		KeDelayExecutionThread(KernelMode, TRUE, &delay);

		delay.QuadPart = -10; // 1us
		value = (unsigned char)1;
		SetGPIO(devCtx, devCtx->FakeSpiCs, &value);

		for (i = 0; i < 8; i++) {
			value = (unsigned char)0;
			SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
			KeDelayExecutionThread(KernelMode, TRUE, &delay);
			value = (unsigned char)1;
			SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
			KeDelayExecutionThread(KernelMode, TRUE, &delay);
		}

		value = (unsigned char)0;
		SetGPIO(devCtx, devCtx->FakeSpiCs, &value);

		for (i = 0; i < 71339; i++) {
			for (j = 7; j >= 0; j--) {
				//KeDelayExecutionThread(KernelMode, TRUE, &delay);

				value = (unsigned char)((bitstream[i] >> j) & 1);
				status = SetGPIO(devCtx, devCtx->FakeSpiMosi, &value);
				if (!NT_SUCCESS(status))
					return status;

				value = (unsigned char)0;
				status = SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
				if (!NT_SUCCESS(status))
					return status;

				//KeDelayExecutionThread(KernelMode, TRUE, &delay);

				value = (unsigned char)1;
				status = SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
				if (!NT_SUCCESS(status))
					return status;
			}
		}

		for (i = 0; i < 150; i++) {
			value = (unsigned char)0;
			SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
			//KeDelayExecutionThread(KernelMode, TRUE, &delay);
			value = (unsigned char)1;
			SetGPIO(devCtx, devCtx->FakeSpiClk, &value);
			//KeDelayExecutionThread(KernelMode, TRUE, &delay);
		}
	}
	*/
	if (!NT_SUCCESS(MyReadRegistryValue(
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)L"VbusEnable",
		REG_DWORD,
		&data,
		sizeof(ULONG))))
	{
		data = 0;
	}
	value = !!data;
	SetGPIO(devCtx, devCtx->VbusGpio, &value);
	if (data) {
		UCM_PD_POWER_DATA_OBJECT Pdos[1];
		UCM_PD_POWER_DATA_OBJECT_INIT_FIXED(&Pdos[0]);

		Pdos[0].FixedSupplyPdo.VoltageIn50mV = 100;         // 5V
		Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = 50;  // 500 mA
		UcmConnectorPdSourceCaps(devCtx->Connector, Pdos, 1);
		UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS PdParams;
		UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&PdParams, UcmPdConnStateNotSupported);
		PdParams.ChargingState = UcmChargingStateNotCharging;
		UcmConnectorPdConnectionStateChanged(devCtx->Connector, &PdParams);
		UcmConnectorPowerDirectionChanged(devCtx->Connector, TRUE, UcmPowerRoleSource);
		UcmConnectorChargingStateChanged(devCtx->Connector, PdParams.ChargingState);
	}
	else
	{
		UCM_PD_POWER_DATA_OBJECT Pdos[1];
		UCM_PD_POWER_DATA_OBJECT_INIT_FIXED(&Pdos[0]);

		Pdos[0].FixedSupplyPdo.VoltageIn50mV = 100;         // 5V
		Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = 50;  // 500 mA - can be overridden in Registry
		if (NT_SUCCESS(MyReadRegistryValue(
			(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
			(PCWSTR)L"ChargeCurrent",
			REG_DWORD,
			&data,
			sizeof(ULONG))))
		{
			Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = data / 10;
		}
		UcmConnectorPdPartnerSourceCaps(devCtx->Connector, Pdos, 1);
		UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS PdParams;
		UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&PdParams, UcmPdConnStateNotSupported);
		PdParams.ChargingState = UcmChargingStateNominalCharging;
		UcmConnectorPdConnectionStateChanged(devCtx->Connector, &PdParams);
		UcmConnectorPowerDirectionChanged(devCtx->Connector, TRUE, UcmPowerRoleSink);
		UcmConnectorTypeCCurrentAdChanged(devCtx->Connector, UcmTypeCCurrent3000mA);
		UcmConnectorChargingStateChanged(devCtx->Connector, PdParams.ChargingState);
	}

	DbgPrint("LumiaUSBC: LumiaUSBCDeviceD0Entry Exit\n");

	return status;
}

NTSTATUS LumiaUSBCSelfManagedIoInit(
	WDFDEVICE Device
)
{
	DbgPrint("LumiaUSBC: LumiaUSBCSelfManagedIoInit Entry\n");

	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);
	unsigned char value = (unsigned char)0;
	ULONG input[8], output[6];
	LARGE_INTEGER delay;
	LONG i = 0;// , j = 0;
	PO_FX_DEVICE poFxDevice;
	PO_FX_COMPONENT_IDLE_STATE idleState;

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
		DbgPrint("LumiaUSBC: PoFxRegisterDevice failed %x\n", status);
		return status;
	}

	PoFxActivateComponent(devCtx->PoHandle, 0, PO_FX_FLAG_BLOCKING);

	PoFxStartDevicePowerManagement(devCtx->PoHandle);

	// Initialize the UC120
	unsigned char initvals[] = { 0x0C, 0x7C, 0x31, 0x5E, 0x9D, 0x0D, 0x7D, 0x32, 0x5F, 0x9E };

	UC120_D0Entry(devCtx);
	UC120_UploadCalibrationData(devCtx, initvals, sizeof(initvals));

	value = 0;

	delay.QuadPart = -1000000;
	KeDelayExecutionThread(UserMode, TRUE, &delay);
	delay.QuadPart = -100000;
	do {
		if (!NT_SUCCESS(ReadRegister(devCtx, 5, &value, 1)))
			DbgPrint("LumiaUSBC: Failed to read register #5 in the init waiting loop!\n");
		KeDelayExecutionThread(UserMode, TRUE, &delay);
		i++;
		if (i > 500) {
			break;
		}
	} while (value == 0);

	DbgPrint("LumiaUSBC: UC120 initialized after %d iterations with value of %x\n", i, value);

	i |= value << 16;

	RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)L"startdelay",
		REG_DWORD,
		&i,
		sizeof(ULONG));

	//UC120_GetCurrentRegisters(devCtx, 0);
	UC120_GetCurrentState(devCtx, 0);

	UC120_InterruptsEnable(devCtx);

	// Tell PEP to turn on the clock
	memset(input, 0, sizeof(input));
	input[0] = 2;
	input[7] = 2;
	status = PoFxPowerControl(devCtx->PoHandle, &PowerControlGuid, &input, sizeof(input), &output, sizeof(output), NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBC: PoFxPowerControl failed %x\n", status);
		return status;
	}

	DbgPrint("LumiaUSBC: LumiaUSBCSelfManagedIoInit Exit\n");
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

	DbgPrint("LumiaUSBC: LumiaUSBCKmCreateDevice Entry\n");

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
			DbgPrint("LumiaUSBC: LumiaUSBCKmCreateDevice Exit: %x\n", status);
			return status;
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
			DbgPrint("LumiaUSBC: LumiaUSBCKmCreateDevice Exit: %x\n", status);
			return status;
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

	DbgPrint("LumiaUSBC: LumiaUSBCKmCreateDevice Exit: %x\n", status);
	return status;
}
