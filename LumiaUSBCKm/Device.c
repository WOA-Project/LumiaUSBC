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
//EVT_WDF_DEVICE_D0_ENTRY LumiaUSBCDeviceD0Entry;

NTSTATUS ReadRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length);
NTSTATUS WriteRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length);
NTSTATUS GetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char *value);
NTSTATUS SetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char *value);

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

	UNREFERENCED_PARAMETER(DataRole);

	connCtx = ConnectorGetContext(Connector);

	return STATUS_SUCCESS;
}

BOOLEAN EvtInterruptIsr(
	WDFINTERRUPT Interrupt,
	ULONG MessageID
)
{
	UNREFERENCED_PARAMETER((Interrupt, MessageID));

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
	unsigned char registers[8];
	NTSTATUS statuses[8];
	unsigned char dismiss = 0xFF;
	wchar_t buf[260];
	//ULONG data = 0;

	DbgPrint("Got an interrupt from the UC120!\n");

	memset(registers, 0, sizeof(registers));
	memset(statuses, 0, sizeof(statuses));

	statuses[0] = ReadRegister(ctx, 0, registers + 0, 1);
	statuses[1] = ReadRegister(ctx, 1, registers + 1, 1);
	statuses[2] = ReadRegister(ctx, 2, registers + 2, 1);
	statuses[3] = ReadRegister(ctx, 5, registers + 3, 1);
	statuses[4] = ReadRegister(ctx, 7, registers + 4, 1);
	statuses[5] = ReadRegister(ctx, 9, registers + 5, 1);
	statuses[6] = ReadRegister(ctx, 10, registers + 6, 1);
	statuses[7] = ReadRegister(ctx, 11, registers + 7, 1);

	WriteRegister(ctx, 2, &dismiss, 1);

	swprintf(buf, L"UC120_%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);

	DbgPrint("%ls\n", buf);

	/*RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));*/

	swprintf(buf, L"S_UC120_%08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x", statuses[0], statuses[1], statuses[2], statuses[3], statuses[4], statuses[5], statuses[6], statuses[7]);

	DbgPrint("%ls\n", buf);

	/*RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));*/
}

void PlugDetInterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);
	unsigned char registers[8];
	NTSTATUS statuses[8];
	//	unsigned char dismiss = 0x1;
	wchar_t buf[260];
	ULONG data = 0;

	memset(registers, 0, sizeof(registers));
	memset(statuses, 0, sizeof(statuses));

	statuses[0] = ReadRegister(ctx, 0, registers + 0, 1);
	statuses[1] = ReadRegister(ctx, 1, registers + 1, 1);
	statuses[2] = ReadRegister(ctx, 2, registers + 2, 1);
	statuses[3] = ReadRegister(ctx, 5, registers + 3, 1);
	statuses[4] = ReadRegister(ctx, 7, registers + 4, 1);
	statuses[5] = ReadRegister(ctx, 9, registers + 5, 1);
	statuses[6] = ReadRegister(ctx, 10, registers + 6, 1);
	statuses[7] = ReadRegister(ctx, 11, registers + 7, 1);

	//WriteRegister(ctx, 2, &dismiss, 1);

	swprintf(buf, L"PLUGDET_%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);

	RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));

	swprintf(buf, L"S_PLUGDET_%08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x", statuses[0], statuses[1], statuses[2], statuses[3], statuses[4], statuses[5], statuses[6], statuses[7]);

	RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));
}

NTSTATUS Uc120InterruptEnable(
	WDFINTERRUPT Interrupt,
	WDFDEVICE AssociatedDevice
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedDevice);
	unsigned char value;
	UNREFERENCED_PARAMETER(Interrupt);

	value = 0xFF;
	WriteRegister(ctx, 2, &value, 1);
	WriteRegister(ctx, 3, &value, 1);

	ReadRegister(ctx, 4, &value, 1);
	value |= 1;
	WriteRegister(ctx, 4, &value, 1);

	ReadRegister(ctx, 5, &value, 1);
	value &= ~0x80;
	WriteRegister(ctx, 5, &value, 1);

	return status;
}

NTSTATUS Uc120InterruptDisable(
	WDFINTERRUPT Interrupt,
	WDFDEVICE AssociatedDevice
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedDevice);
	unsigned char value;
	UNREFERENCED_PARAMETER(Interrupt);

	ReadRegister(ctx, 4, &value, 1);
	value &= ~1;
	WriteRegister(ctx, 4, &value, 1);

	return status;
}

NTSTATUS OpenIOTarget(PDEVICE_CONTEXT ctx, LARGE_INTEGER res, ACCESS_MASK use, WDFIOTARGET *target)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDF_IO_TARGET_OPEN_PARAMS OpenParams;
	UNICODE_STRING ReadString;
	WCHAR ReadStringBuffer[260];

	DbgPrint("%!FUNC! Entry");

	RtlInitEmptyUnicodeString(&ReadString,
		ReadStringBuffer,
		sizeof(ReadStringBuffer));

	status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString,
		res.LowPart,
		res.HighPart);
	if (!NT_SUCCESS(status)) {
		DbgPrint("RESOURCE_HUB_CREATE_PATH_FROM_ID failed %!STATUS!\n", status);
		return status;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = ctx->Device;

	status = WdfIoTargetCreate(ctx->Device, &ObjectAttributes, target);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfIoTargetCreate failed %!STATUS!\n", status);
		return status;
	}

	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams, &ReadString, use);
	status = WdfIoTargetOpen(*target, &OpenParams);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfIoTargetOpen failed %!STATUS!\n", status);
	}

	DbgPrint("%!FUNC! Exit\n");
	return status;
}

NTSTATUS
LumiaUSBCProbeResources(
	PDEVICE_CONTEXT ctx,
	WDFCMRESLIST res,
	WDFCMRESLIST rawres
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR  desc;
	WDF_INTERRUPT_CONFIG Config;
	int k = 0, l = 0;
	int spi_found = 0;
	ctx->HaveResetGpio = FALSE;
	ctx->UseFakeSpi = FALSE;
	DbgPrint("%!FUNC! Entry\n");

	for (unsigned int i = 0; i < WdfCmResourceListGetCount(res); i++) {
		desc = WdfCmResourceListGetDescriptor(res, i);
		switch (desc->Type) {

		case CmResourceTypeConnection:
			//
			// Handle memory resources here.
			//
			if (desc->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_GPIO && desc->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_GPIO_IO) {
				switch (k) {
				case 0:
					ctx->VbusGpioId.LowPart = desc->u.Connection.IdLowPart;
					ctx->VbusGpioId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 1:
					ctx->PolGpioId.LowPart = desc->u.Connection.IdLowPart;
					ctx->PolGpioId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 2:
					ctx->AmselGpioId.LowPart = desc->u.Connection.IdLowPart;
					ctx->AmselGpioId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 3:
					ctx->EnGpioId.LowPart = desc->u.Connection.IdLowPart;
					ctx->EnGpioId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 4:
					ctx->ResetGpioId.LowPart = desc->u.Connection.IdLowPart;
					ctx->ResetGpioId.HighPart = desc->u.Connection.IdHighPart;
					ctx->HaveResetGpio = TRUE;
					break;
				case 5:
					ctx->FakeSpiMosiId.LowPart = desc->u.Connection.IdLowPart;
					ctx->FakeSpiMosiId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 6:
					ctx->FakeSpiMisoId.LowPart = desc->u.Connection.IdLowPart;
					ctx->FakeSpiMisoId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 7:
					ctx->FakeSpiCsId.LowPart = desc->u.Connection.IdLowPart;
					ctx->FakeSpiCsId.HighPart = desc->u.Connection.IdHighPart;
					break;
				case 8:
					ctx->FakeSpiClkId.LowPart = desc->u.Connection.IdLowPart;
					ctx->FakeSpiClkId.HighPart = desc->u.Connection.IdHighPart;
					break;
				default:
					break;
				}

				k++;
			}

			if (desc->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL && desc->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI) {
				ctx->SpiId.LowPart = desc->u.Connection.IdLowPart;
				ctx->SpiId.HighPart = desc->u.Connection.IdHighPart;
				spi_found = 1;
			}
			break;

		case CmResourceTypeInterrupt:
			switch (l)
			{
			case 0:
				WDF_INTERRUPT_CONFIG_INIT(&Config, EvtInterruptIsr, NULL);
				//Config.PassiveHandling = TRUE;
				Config.EvtInterruptWorkItem = PlugDetInterruptWorkItem;
				Config.InterruptRaw = WdfCmResourceListGetDescriptor(rawres, i);
				Config.InterruptTranslated = desc;
				//status = WdfInterruptCreate(ctx->Device, &Config, WDF_NO_OBJECT_ATTRIBUTES, &ctx->PlugDetectInterrupt);
				if (!NT_SUCCESS(status)) {
					DbgPrint("WdfInterruptCreate failed for plug detection %!STATUS!\n", status);
					return status;
				}
				break;
			case 1:
				WDF_INTERRUPT_CONFIG_INIT(&Config, EvtInterruptIsr, NULL);
				Config.PassiveHandling = TRUE;
				Config.EvtInterruptWorkItem = Uc120InterruptWorkItem;
				Config.InterruptRaw = WdfCmResourceListGetDescriptor(rawres, i);
				Config.InterruptTranslated = desc;
				Config.EvtInterruptEnable = Uc120InterruptEnable;
				Config.EvtInterruptDisable = Uc120InterruptDisable;
				status = WdfInterruptCreate(ctx->Device, &Config, WDF_NO_OBJECT_ATTRIBUTES, &ctx->Uc120Interrupt);
				if (!NT_SUCCESS(status)) {
					DbgPrint("WdfInterruptCreate failed for UC120 interrupt %!STATUS!\n", status);
					return status;
				}
				break;
			default:
				break;
			}
			l++;
			break;
		default:
			//
			// Ignore all other descriptors.
			//
			break;
		}
	}

	if (!spi_found && k >= 8) {
		spi_found = 1;
		ctx->UseFakeSpi = TRUE;
	}

	if (!spi_found || k < 4) {
		DbgPrint("Not all resources were found, SPI = %d, GPIO = %d\n", spi_found, k);
		status = 0xC0000000 + 8 * spi_found + k;
	}

	DbgPrint("%!FUNC! Exit\n");
	return status;
}

NTSTATUS
LumiaUSBCOpenResources(
	PDEVICE_CONTEXT ctx
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("%!FUNC! Entry\n");

	if (!(ctx->UseFakeSpi)) {
		status = OpenIOTarget(ctx, ctx->SpiId, GENERIC_READ | GENERIC_WRITE, &ctx->Spi);
		if (!NT_SUCCESS(status)) {
			DbgPrint("OpenIOTarget failed for SPI %!STATUS! Falling back to fake SPI.\n", status);
			ctx->UseFakeSpi = TRUE;
			status = STATUS_SUCCESS; // return status;
		}
	}

	status = OpenIOTarget(ctx, ctx->VbusGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->VbusGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("OpenIOTarget failed for VBUS GPIO %!STATUS!\n", status);
		return status;
	}

	status = OpenIOTarget(ctx, ctx->PolGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->PolGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("OpenIOTarget failed for polarity GPIO %!STATUS!\n", status);
		return status;
	}

	status = OpenIOTarget(ctx, ctx->AmselGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->AmselGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("OpenIOTarget failed for alternate mode selection GPIO %!STATUS!\n", status);
		return status;
	}

	status = OpenIOTarget(ctx, ctx->EnGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->EnGpio);
	if (!NT_SUCCESS(status)) {
		DbgPrint("OpenIOTarget failed for mux enable GPIO %!STATUS!\n", status);
		return status;
	}

	if (ctx->HaveResetGpio) {
		status = OpenIOTarget(ctx, ctx->ResetGpioId, GENERIC_READ | GENERIC_WRITE, &ctx->ResetGpio);
		if (!(NT_SUCCESS(status))) {
			DbgPrint("OpenIOTarget failed for chip reset GPIO %!STATUS!\n", status);
			ctx->HaveResetGpio = FALSE;
		}
		status = STATUS_SUCCESS; // this GPIO is optional - make sure we never fail on it missing
	}

	if (ctx->UseFakeSpi) {
		status = OpenIOTarget(ctx, ctx->FakeSpiMosiId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiMosi);
		if (!NT_SUCCESS(status)) {
			DbgPrint("OpenIOTarget failed for fake SPI MOSI line %!STATUS!\n", status);
			return status;
		}
		status = OpenIOTarget(ctx, ctx->FakeSpiMisoId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiMiso);
		if (!NT_SUCCESS(status)) {
			DbgPrint("OpenIOTarget failed for fake SPI MISO line %!STATUS!\n", status);
			return status;
		}
		status = OpenIOTarget(ctx, ctx->FakeSpiCsId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiCs);
		if (!NT_SUCCESS(status)) {
			DbgPrint("OpenIOTarget failed for fake SPI CS# line %!STATUS!\n", status);
			return status;
		}
		status = OpenIOTarget(ctx, ctx->FakeSpiClkId, GENERIC_READ | GENERIC_WRITE, &ctx->FakeSpiClk);
		if (!NT_SUCCESS(status)) {
			DbgPrint("OpenIOTarget failed for fake SPI clock line %!STATUS!\n", status);
			return status;
		}
	}

	DbgPrint("%!FUNC! Exit\n");
	return status;
}

void
LumiaUSBCCloseResources(
	PDEVICE_CONTEXT ctx
)
{
	DbgPrint("%!FUNC! Entry\n");

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

	DbgPrint("%!FUNC! Exit\n");
}

NTSTATUS LumiaUSBCDeviceD0Exit(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE TargetState
)
{
	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);
	UNREFERENCED_PARAMETER(TargetState);

	LumiaUSBCCloseResources(devCtx);

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

NTSTATUS ReadRegisterReal(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length)
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

NTSTATUS WriteRegisterReal(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length)
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


NTSTATUS GetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char *value)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_MEMORY_DESCRIPTOR outputDescriptor;

	UNREFERENCED_PARAMETER(ctx);

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, value, 1);

	status = WdfIoTargetSendIoctlSynchronously(gpio, NULL, IOCTL_GPIO_READ_PINS, NULL, &outputDescriptor, NULL, NULL);

	return status;
}

NTSTATUS SetGPIO(PDEVICE_CONTEXT ctx, WDFIOTARGET gpio, unsigned char *value)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_MEMORY_DESCRIPTOR inputDescriptor, outputDescriptor;

	UNREFERENCED_PARAMETER(ctx);

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, value, 1);
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, value, 1);

	status = WdfIoTargetSendIoctlSynchronously(gpio, NULL, IOCTL_GPIO_WRITE_PINS, &inputDescriptor, &outputDescriptor, NULL, NULL);

	return status;
}

NTSTATUS ReadRegisterFake(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length)
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

NTSTATUS WriteRegisterFake(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length)
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

NTSTATUS ReadRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length)
{
	if (ctx->UseFakeSpi)
		return ReadRegisterFake(ctx, reg, value, length);
	else
		return ReadRegisterReal(ctx, reg, value, length);
}

NTSTATUS WriteRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char *value, ULONG length)
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

	DbgPrint("%!FUNC! Entry\n");

	devCtx = DeviceGetContext(Device);

	status = LumiaUSBCProbeResources(devCtx, ResourcesTranslated, ResourcesRaw);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBCProbeResources failed %!STATUS!\n", status);
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
		DbgPrint("UcmInitializeDevice failed %!STATUS!\n", status);
		goto Exit;
	}

	//
	// Create a USB Type-C connector #0 with PD
	//
	UCM_CONNECTOR_CONFIG_INIT(&connCfg, 0);

	UCM_CONNECTOR_TYPEC_CONFIG_INIT(
		&typeCConfig,
		UcmTypeCOperatingModeDrp,
		UcmTypeCCurrent3000mA | UcmTypeCCurrent1500mA | UcmTypeCCurrentDefaultUsb);

	typeCConfig.EvtSetDataRole = LumiaUSBCSetDataRole;

	UCM_CONNECTOR_PD_CONFIG_INIT(&pdConfig, UcmPowerRoleSink | UcmPowerRoleSource);

	connCfg.TypeCConfig = &typeCConfig;
	connCfg.PdConfig = &pdConfig;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, CONNECTOR_CONTEXT);

	status = UcmConnectorCreate(Device, &connCfg, &attr, &devCtx->Connector);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("UcmConnectorCreate failed %!STATUS!\n", status);
		goto Exit;
	}

	//UcmEventInitialize(&connCtx->EventSetDataRole);
Exit:
	DbgPrint("%!FUNC! Exit\n");
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

	DbgPrint("%!FUNC! Entry\n");

	status = LumiaUSBCOpenResources(devCtx);
	if (!NT_SUCCESS(status)) {
		DbgPrint("LumiaUSBCOpenResources failed %!STATUS!\n", status);
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
		UcmConnectorPowerDirectionChanged(devCtx->Connector, TRUE, UcmPowerRoleSource);
		UcmConnectorTypeCCurrentAdChanged(devCtx->Connector, UcmTypeCCurrentDefaultUsb);
		UcmConnectorChargingStateChanged(devCtx->Connector, UcmChargingStateNotCharging);
	}
	else
	{
		UcmConnectorPowerDirectionChanged(devCtx->Connector, TRUE, UcmPowerRoleSink);
		UcmConnectorTypeCCurrentAdChanged(devCtx->Connector, UcmTypeCCurrent3000mA);
		UcmConnectorChargingStateChanged(devCtx->Connector, UcmChargingStateNominalCharging);
	}

	return status;
}

NTSTATUS LumiaUSBCSelfManagedIoInit(
	WDFDEVICE Device
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devCtx = DeviceGetContext(Device);
	wchar_t buf[260];
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
		DbgPrint("PoFxRegisterDevice failed %!STATUS!\n", status);
		return status;
	}

	PoFxActivateComponent(devCtx->PoHandle, 0, PO_FX_FLAG_BLOCKING);

	PoFxStartDevicePowerManagement(devCtx->PoHandle);

	// Tell PEP to turn on the clock
	memset(input, 0, sizeof(input));
	input[0] = 2;
	input[7] = 2;
	status = PoFxPowerControl(devCtx->PoHandle, &PowerControlGuid, &input, sizeof(input), &output, sizeof(output), NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("PoFxPowerControl failed %!STATUS!\n", status);
		return status;
	}

	// Initialize the UC120
	unsigned char initvals[] = { 0x0C, 0x7C, 0x31, 0x5E, 0x9D, 0x0A, 0x7A, 0x2F, 0x5C, 0x9B };

	//ReadRegister(devCtx, 4, &value, 1);
	value = 6;
	WriteRegister(devCtx, 4, &value, 1);

	//ReadRegister(devCtx, 5, &value, 1);
	value = 0x88;
	WriteRegister(devCtx, 5, &value, 1);

	//ReadRegister(devCtx, 13, &value, 1);
	value = 2;
	WriteRegister(devCtx, 13, &value, 1);

	WriteRegister(devCtx, 18, initvals + 0, 1);
	WriteRegister(devCtx, 19, initvals + 1, 1);
	WriteRegister(devCtx, 20, initvals + 2, 1);
	WriteRegister(devCtx, 21, initvals + 3, 1);
	WriteRegister(devCtx, 26, initvals + 4, 1);
	WriteRegister(devCtx, 22, initvals + 5, 1);
	WriteRegister(devCtx, 23, initvals + 6, 1);
	WriteRegister(devCtx, 24, initvals + 7, 1);
	WriteRegister(devCtx, 25, initvals + 8, 1);
	WriteRegister(devCtx, 27, initvals + 9, 1);

	value = 0;

	delay.QuadPart = -1000000;
	KeDelayExecutionThread(UserMode, TRUE, &delay);
	delay.QuadPart = -100000;
	do {
		if (!NT_SUCCESS(ReadRegister(devCtx, 5, &value, 1)))
			DbgPrint("Failed to read register #5 in the init waiting loop!\n");
		KeDelayExecutionThread(UserMode, TRUE, &delay);
		i++;
		if (i > 500) {
			break;
		}
	} while (value == 0);

	DbgPrint("UC120 initialized after %d iterations with value of %x\n", i, value);

	/*i |= value << 16;

	RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)L"startdelay",
		REG_DWORD,
		&i,
		sizeof(ULONG));*/

	unsigned char registers[8];
	NTSTATUS statuses[8];

	memset(registers, 0, sizeof(registers));
	memset(statuses, 0, sizeof(statuses));

	statuses[0] = ReadRegister(devCtx, 0, registers + 0, 1);
	statuses[1] = ReadRegister(devCtx, 1, registers + 1, 1);
	statuses[2] = ReadRegister(devCtx, 2, registers + 2, 1);
	statuses[3] = ReadRegister(devCtx, 5, registers + 3, 1);
	statuses[4] = ReadRegister(devCtx, 7, registers + 4, 1);
	statuses[5] = ReadRegister(devCtx, 9, registers + 5, 1);
	statuses[6] = ReadRegister(devCtx, 10, registers + 6, 1);
	statuses[7] = ReadRegister(devCtx, 11, registers + 7, 1);
	swprintf(buf, L"INIT_%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);
	DbgPrint("%ls\n", buf);

	/*RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));*/

	swprintf(buf, L"S_INIT_%08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x", statuses[0], statuses[1], statuses[2], statuses[3], statuses[4], statuses[5], statuses[6], statuses[7]);
	DbgPrint("%ls\n", buf);

	/*RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_DWORD,
		&data,
		sizeof(ULONG));*/

	DbgPrint("%!FUNC! Exit\n");
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
			return status;

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

		status = WdfDeviceAssignS0IdleSettings(
			device,
			&idleSettings
		);
		if (!NT_SUCCESS(status)) {
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

    return status;
}
