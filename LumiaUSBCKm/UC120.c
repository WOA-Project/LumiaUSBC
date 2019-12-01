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
#include "UC120.tmh"

NTSTATUS UC120_GetCurrentRegisters(PDEVICE_CONTEXT deviceContext, unsigned int context)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char value = 0;
	wchar_t buf[260];

	DbgPrint("LumiaUSBC: UC120_GetCurrentRegisters Entry\n");
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
	DbgPrint("LumiaUSBC: UC120_GetCurrentRegisters Exit\n");
	return status;
}

NTSTATUS UC120_GetCurrentState(PDEVICE_CONTEXT deviceContext, unsigned int context)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;
	wchar_t buf[260];
	ULONG outgoingMessageSize, incomingMessageSize, isCableConnected, newPowerRole, newDataRole, vconnRoleSwitch = 0;

	DbgPrint("LumiaUSBC: UC120_GetCurrentState Entry\n");
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

	data = registers[0];
	outgoingMessageSize = data & 31u; // 0-4

	data = registers[1];
	incomingMessageSize = data & 31u; // 0-4

	data = registers[2];
	isCableConnected = (data & 60u) >> 2u; // 2-5

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

	unsigned int side = 1;
	UCM_TYPEC_PARTNER mode = UcmTypeCPartnerInvalid;

	// Detect the side by measuring which CC value is the highest
	side = registers[5] > registers[6];

	// Nothing is connected
	if ((unsigned int)registers[5] == 0u && (unsigned int)registers[6] == 0u)
	{
		DbgPrint("LumiaUSBC: Connector is empty!\n");

		status = USBC_ChangeRole(deviceContext, mode, side);
		goto Exit;
	}

	DbgPrint("LumiaUSBC: DetectedSide=%x\n", side);

	// We connected a dongle, turn on VBUS
	if (newPowerRole)
	{
		// HOST + VBUS
		mode = UcmTypeCPartnerUfp;
		DbgPrint("LumiaUSBC: VBus will be turned on\n");
	}
	else
	{
		// VBUS off
		mode = UcmTypeCPartnerPoweredCableWithUfp;

		/*if (context == 2)
		{
			if (registers[2] == 0x40)
			{
				// DEVICE
				mode = UcmTypeCPartnerDfp;
			}
			else if (registers[2] == 0x80)
			{
				// HOST
				mode = UcmTypeCPartnerPoweredCableWithUfp;
			}
			else if (registers[2] == 0xc0)
			{
				// CHARGER
				mode = UcmTypeCPartnerPoweredCableNoUfp;
			}
			else
			{
				// No idea - report as UcmTypeCPartnerPoweredCableNoUfp
				mode = UcmTypeCPartnerPoweredCableNoUfp;
			}
		}
		else
		{
			// No idea - report as UcmTypeCPartnerPoweredCableNoUfp
			mode = UcmTypeCPartnerPoweredCableNoUfp;
		}*/
	}

	DbgPrint("LumiaUSBC: CurrentState=%x\n", mode);

	status = USBC_ChangeRole(deviceContext, mode, side);

Exit:
	DbgPrint("LumiaUSBC: UC120_GetCurrentState Exit\n");
	return status;
}

NTSTATUS UC120_InterruptHandled(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	DbgPrint("LumiaUSBC: UC120_InterruptHandled Entry\n");

	data = 0xFF; // Clear to 0xFF
	status = WriteRegister(deviceContext, 2, &data, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	DbgPrint("LumiaUSBC: UC120_InterruptHandled Exit\n");
	return status;
}

NTSTATUS UC120_InterruptsEnable(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	DbgPrint("LumiaUSBC: UC120_InterruptsEnable Entry\n");

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
	DbgPrint("LumiaUSBC: UC120_InterruptsEnable Exit\n");
	return status;
}

NTSTATUS UC120_InterruptsDisable(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	DbgPrint("LumiaUSBC: UC120_InterruptsDisable Entry\n");

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
	DbgPrint("LumiaUSBC: UC120_InterruptsDisable Exit\n");
	return status;
}

NTSTATUS UC120_D0Entry(PDEVICE_CONTEXT deviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;

	DbgPrint("LumiaUSBC: UC120_D0Entry Entry\n");

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
	DbgPrint("LumiaUSBC: UC120_D0Entry Exit\n");
	return status;
}

NTSTATUS UC120_UploadCalibrationData(PDEVICE_CONTEXT deviceContext, unsigned char* calibrationFile, unsigned int length)
{
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("LumiaUSBC: UC120_UploadCalibrationData Entry\n");

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
	DbgPrint("LumiaUSBC: UC120_UploadCalibrationData Exit\n");
	return status;
}
