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
#include "UC120Registers.h"

NTSTATUS UC120_ReadRegisters(PDEVICE_CONTEXT deviceContext, PUC120_REGISTERS registers)
{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("LumiaUSBC: UC120_ReadRegisters Entry\n");

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
	DbgPrint("LumiaUSBC: UC120_ReadRegisters Exit\n");
	return status;
}

unsigned long counter = 0;

NTSTATUS UC120_PrintRegisters(UC120_REGISTERS registers, ULONG context)
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
		sizeof(UC120_REGISTERS));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	counter++;
Exit:
	return status;
}

NTSTATUS UC120_GetCurrentRegisters(PDEVICE_CONTEXT deviceContext, unsigned int context)
{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("LumiaUSBC: UC120_GetCurrentRegisters Entry\n");

	UC120_REGISTERS registers;
	RtlZeroMemory(&registers, sizeof(UC120_REGISTERS));

	status = UC120_ReadRegisters(deviceContext, &registers);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = UC120_PrintRegisters(registers, context);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	unsigned int side = 1;
	UCM_TYPEC_PARTNER mode = UcmTypeCPartnerInvalid;

	// Detect the side by measuring which CC value is the highest
	side = registers.Register9 > registers.Register10;

	// Nothing is connected
	if ((unsigned int)registers.Register9 == 0u && (unsigned int)registers.Register10 == 0u)
	{
		DbgPrint("LumiaUSBC: Connector is empty!\n");

		status = USBC_ChangeRole(deviceContext, mode, side);
		goto Exit;
	}

	DbgPrint("LumiaUSBC: DetectedSide=%x\n", side);

	UC120_REG5 reg5 = { 0 };
	reg5.RegisterData = registers.Register5;

	// We connected a dongle, turn on VBUS
	if (reg5.RegisterContent.PowerRole)
	{
		// HOST + VBUS
		mode = UcmTypeCPartnerUfp;
		DbgPrint("LumiaUSBC: VBus will be turned on\n");
	}
	else
	{
		// VBUS off
		mode = UcmTypeCPartnerPoweredCableWithUfp;
	}

	DbgPrint("LumiaUSBC: CurrentState=%x\n", mode);

	status = USBC_ChangeRole(deviceContext, mode, side);

	unsigned int newPowerRole = reg5.RegisterContent.PowerRole;
	unsigned int newDataRole = reg5.RegisterContent.DataRole;
	unsigned int vconnRoleSwitch = reg5.RegisterContent.VconnRole;

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

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"Side",
		REG_DWORD,
		&side,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
		(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
		L"RoleMode",
		REG_DWORD,
		&mode,
		sizeof(ULONG));
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

Exit:
	DbgPrint("LumiaUSBC: UC120_GetCurrentRegisters Exit\n");
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

	UC120_REG4 register4;
	UC120_REG5 register5;
	UC120_REG13 register13;

	DbgPrint("LumiaUSBC: UC120_D0Entry Entry\n");

	RtlZeroMemory(&register4, sizeof(UC120_REG4));
	RtlZeroMemory(&register5, sizeof(UC120_REG5));
	RtlZeroMemory(&register13, sizeof(UC120_REG13));

	status = ReadRegister(deviceContext, 4, &register4, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	register4.RegisterContent.EnableInterrupts = 1;
	register4.RegisterContent.Reserved0 |= 1; // turn on bit 1
	status = WriteRegister(deviceContext, 4, &register4, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 5, &register5, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	register5.RegisterContent.D0Enterred = 1;
	register5.RegisterContent.D0EnterredAndInterruptsYetToBeEnabled = 1;
	status = WriteRegister(deviceContext, 5, &register5, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = ReadRegister(deviceContext, 13, &register13, 1);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	register13.RegisterContent.D0Enterred = 1;
	register13.RegisterContent.D0NotEnterred = 0;
	status = WriteRegister(deviceContext, 13, &register13, 1);
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
