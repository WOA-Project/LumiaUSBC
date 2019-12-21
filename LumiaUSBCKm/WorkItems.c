/*++

Module Name:

	WorkItems.c - Interrupt work items for the UC120 driver.

Abstract:

   This file contains the work items for the device interrupts.

Environment:

	Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "SPI.h"
#include "UC120.h"
#include "WorkItems.tmh"
#include "UC120Registers.h"

void Uc120InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	UC120_REG2 Reg2;
	UC120_REG7 Reg7;
	UCHAR Buf;
	NTSTATUS Status;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the UC120");
	DbgPrint("LumiaUSBC: Got an interrupt from the UC120!\n");

	// Read register 2
	Status = ReadRegister(ctx, 2, &Reg2, sizeof(Reg2));
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 2: %!STATUS!", Status);
		goto exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg2: 0x%x; Charger1 = %d, Charger2 = %d, Dongle = %d", 
		Reg2.RegisterData, Reg2.RegisterContent.Charger1, Reg2.RegisterContent.Charger2, Reg2.RegisterContent.Dongle);

	// Read register 7
	Status = ReadRegister(ctx, 7, &Reg7, sizeof(Reg7));
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 7: %!STATUS!", Status);
		goto exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg7: 0x%x; CableType = %d, Polarity = %d, SkipPDNegotiation = %d",
		Reg7.RegisterData, Reg7.RegisterContent.CableType, Reg7.RegisterContent.Polarity, Reg7.RegisterContent.SkipPDNegotiation);

	if (Reg2.RegisterContent.Dongle) {
		Status = ReadRegister(ctx, 5, &Buf, sizeof(Buf));
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 5: %!STATUS!", Status);
			goto exit;
		}

		if (Buf == 0x9) {
			Buf = 0x29;
		}

		Status = WriteRegister(ctx, 5, &Buf, sizeof(Buf));
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 5: %!STATUS!", Status);
			goto exit;
		}

		Status = ReadRegister(ctx, 5, &Buf, sizeof(Buf));
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 5: %!STATUS!", Status);
			goto exit;
		}

		Status = WriteRegister(ctx, 5, &Buf, sizeof(Buf));
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 5: %!STATUS!", Status);
			goto exit;
		}
	}

	if (Reg2.RegisterContent.Charger1 || Reg2.RegisterContent.Charger2) {
		Buf = 5;
		Status = WriteRegister(ctx, 4, &Buf, sizeof(Buf));
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 4: %!STATUS!", Status);
			goto exit;
		}
	}

exit:
	UC120_InterruptHandled(ctx);
}

void PlugDetInterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER((Interrupt, AssociatedObject));
	// PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the PLUGDET");
	DbgPrint("LumiaUSBC: Got an interrupt from PLUGDET!\n");

	// UC120_GetCurrentRegisters(ctx, 1);
	// UC120_InterruptHandled(ctx);
}

void Mystery1InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);

	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);
	UCHAR Unk = 0x48;
	NTSTATUS Status;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the Mystery 1");
	DbgPrint("LumiaUSBC: Got an interrupt from Mystery 1! R5 48 responded\n");

	Status = WriteRegister(ctx, 5, &Unk, sizeof(Unk));
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 5: %!STATUS!", Status);
	}
}

void Mystery2InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	UNREFERENCED_PARAMETER(AssociatedObject);
	// PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the Mystery 2");
	DbgPrint("LumiaUSBC: Got an interrupt from Mystery 2!\n");
}