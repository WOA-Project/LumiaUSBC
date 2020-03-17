/*++

Module Name:

	UC120.c - Communication routines the UC120 driver.

Abstract:

   This file contains the communication routines for the UC120.

Environment:

	Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "SPI.h"
#include "Registry.h"
#include "USBRole.h"
#include <wchar.h>
#include "UC120.h"

#ifndef DBG_PRINT_EX_LOGGING
#include "UC120.tmh"
#endif

#include "UC120Registers.h"

NTSTATUS UC120_ReadRegisters(PDEVICE_CONTEXT deviceContext, PUC120_REGISTERS registers)
{
	NTSTATUS status = STATUS_SUCCESS;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_ReadRegisters Entry");

	UC120_REGISTERS registersdata;
	RtlZeroMemory(&registersdata, sizeof(UC120_REGISTERS));

	status = ReadRegister(deviceContext, 0, &registersdata.Register0, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 1, &registersdata.Register1, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 2, &registersdata.Register2, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 3, &registersdata.Register3, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 4, &registersdata.Register4, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 5, &registersdata.Register5, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 6, &registersdata.Register6, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 7, &registersdata.Register7, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 8, &registersdata.Register8, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 9, &registersdata.Register9, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 10, &registersdata.Register10, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 11, &registersdata.Register11, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 12, &registersdata.Register12, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 13, &registersdata.Register13, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	*registers = registersdata;

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_ReadRegisters Exit");
	return status;
}

unsigned long counter = 0;

NTSTATUS UC120_PrintRegisters(UC120_REGISTERS registers, UC120_CONTEXT_STATES context)
{
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
	else if (context == 3)
	{
		contextStr = L"MYS1";
	}
	else if (context == 4)
	{
		contextStr = L"MYS2";
	}

	wchar_t buf[260];
	swprintf(buf, L"%lu_%ls", counter, contextStr);

	NTSTATUS status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		(PCWSTR)buf,
		REG_BINARY,
		&registers,
		sizeof(UC120_REGISTERS)
	);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	counter++;
Exit:
	return status;
}

NTSTATUS UC120_GetCurrentRegisters(PDEVICE_CONTEXT deviceContext, UC120_CONTEXT_STATES context)
{
	NTSTATUS status = STATUS_SUCCESS;
	UC120_REGISTERS_PARSED registers;

	UNREFERENCED_PARAMETER(context);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_GetCurrentRegisters Entry");
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "-> UC120_GetCurrentRegisters");

	RtlZeroMemory(&registers, sizeof(UC120_REGISTERS_PARSED));

	// Read register 2
	status = ReadRegister(deviceContext, 2, &registers.Register2, 1);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReadRegister failed with %!STATUS!", status);
		goto Exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BEHAVIOR, "Reg2: 0x%x, ChargerBit1: %d, DongleBit: %d, ChargerBit2: %d",
		registers.Register2.RegisterData,
		registers.Register2.RegisterContent.Charger1,
		registers.Register2.RegisterContent.Dongle,
		registers.Register2.RegisterContent.Charger2
	);

	// Read register 7
	status = ReadRegister(deviceContext, 7, &registers.Register7, 1);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReadRegister failed with %!STATUS!", status);
		goto Exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BEHAVIOR, "Reg7: 0x%x, SkipPD: %d, CableType: %d, Polarity: %d",
		registers.Register7.RegisterData,
		registers.Register7.RegisterContent.SkipPDNegotiation,
		registers.Register7.RegisterContent.CableType,
		registers.Register7.RegisterContent.Polarity
	);

	// Read register 5
	status = ReadRegister(deviceContext, 5, &registers.Register5, 1);

	
Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_GetCurrentRegisters Exit");
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<- UC120_GetCurrentRegisters");
	return status;
}

NTSTATUS UC120_InterruptHandled(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptHandled Entry");

	data = 0xFF; // Clear to 0xFF
	status = WriteRegister(deviceContext, 2, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptHandled Exit");
	return status;
}

NTSTATUS UC120_InterruptsEnable(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptsEnable Entry");

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
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptsEnable Exit");
	return status;
}

NTSTATUS UC120_InterruptsDisable(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptsDisable Entry");

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
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_InterruptsDisable Exit");
	return status;
}

NTSTATUS UC120_D0Entry(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR Val = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_D0Entry Entry");

	// Write as is
	Val = 0x6;
	status = WriteRegister(deviceContext, 4, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	Val = 0x88;
	status = WriteRegister(deviceContext, 5, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	Val = 0x2;
	status = WriteRegister(deviceContext, 13, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	Val = 0xff;
	status = WriteRegister(deviceContext, 2, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	Val = 0xff;
	status = WriteRegister(deviceContext, 3, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	Val = 0x7;
	status = WriteRegister(deviceContext, 4, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	Val = 0x8;
	status = WriteRegister(deviceContext, 5, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WriteRegister failed with %!STATUS!", status);
		goto exit;
	}

	status = ReadRegister(deviceContext, 2, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "ReadRegister failed with %!STATUS!", status);
		goto exit;
	}

	status = ReadRegister(deviceContext, 7, &Val, 1);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "ReadRegister failed with %!STATUS!", status);
		goto exit;
	}

exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_D0Entry Exit");
	return status;
}

NTSTATUS UC120_UploadCalibrationData(PDEVICE_CONTEXT deviceContext, unsigned char* calibrationFile, unsigned int length)
{
	NTSTATUS status = STATUS_SUCCESS;
#if 1
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_UploadCalibrationData Entry");

	switch (length)
	{
	case 10:
	{
		status = WriteRegister(deviceContext, 18, calibrationFile + 0, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 19, calibrationFile + 1, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 20, calibrationFile + 2, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 21, calibrationFile + 3, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 26, calibrationFile + 4, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 22, calibrationFile + 5, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 23, calibrationFile + 6, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 24, calibrationFile + 7, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 25, calibrationFile + 8, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 27, calibrationFile + 9, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		break;
	}
	case 8:
	{
		status = WriteRegister(deviceContext, 18, calibrationFile + 0, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 19, calibrationFile + 1, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 20, calibrationFile + 2, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 21, calibrationFile + 3, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 22, calibrationFile + 4, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 23, calibrationFile + 5, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 24, calibrationFile + 6, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		status = WriteRegister(deviceContext, 25, calibrationFile + 7, 1);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
		break;
	}
	default:
	{
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

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UC120_UploadCalibrationData Exit");
	return status;
}
