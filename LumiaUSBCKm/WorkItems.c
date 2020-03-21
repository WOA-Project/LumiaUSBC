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

#ifndef DBG_PRINT_EX_LOGGING
#include "WorkItems.tmh"
#endif

#include "UC120Registers.h"
#include "USBRole.h"

void Uc120InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject); 
	NTSTATUS Status;

	UC120_REG2 Reg2;
	UC120_REG7 Reg7;
	UCHAR Buf;
	UCM_TYPEC_PARTNER UcmPartnerType = UcmTypeCPartnerInvalid;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the UC120");
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Got an interrupt from the UC120!");

	// Read register 2
	Status = ReadRegister(ctx, 2, &Reg2, 1);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 2: %!STATUS!", Status);
		goto exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg2: 0x%x; Charger1 = %d, Charger2 = %d, Dongle = %d, PoweredCableWithUfp = %d, PoweredCable = %d, PoweredCableNoDfp = %d",
		Reg2.RegisterData,
		Reg2.RegisterContent.Charger1,
		Reg2.RegisterContent.Charger2,
		Reg2.RegisterContent.Dongle,
		Reg2.RegisterContent.PoweredCableWithUfp,
		Reg2.RegisterContent.PoweredCable,
		Reg2.RegisterContent.PoweredCableNoDfp);

	// Read register 7
	Status = ReadRegister(ctx, 7, &Reg7, 1);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 7: %!STATUS!", Status);
		goto exit;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg7: 0x%x; CableType = %d, Polarity = %d, SkipPDNegotiation = %d",
		Reg7.RegisterData, Reg7.RegisterContent.CableType, Reg7.RegisterContent.Polarity, Reg7.RegisterContent.SkipPDNegotiation);

	if (Reg2.RegisterContent.Dongle) {
		Status = ReadRegister(ctx, 5, &Buf, 1);
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 5: %!STATUS!", Status);
			goto exit;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg5: Read 0x%x", Buf);

		if (Buf == 0x9) {
			Buf = 0x29;
		}

		Status = WriteRegister(ctx, 5, &Buf, 1);
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg5: Write 0x%x", Buf);
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 5: %!STATUS!", Status);
			goto exit;
		}

		Status = ReadRegister(ctx, 5, &Buf, 1);
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg5: Read 0x%x", Buf);
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 5: %!STATUS!", Status);
			goto exit;
		}

		Status = WriteRegister(ctx, 5, &Buf, 1);
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg5: Write 0x%x", Buf);
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 5: %!STATUS!", Status);
			goto exit;
		}
	}

	Buf = 5;
	Status = WriteRegister(ctx, 4, &Buf, 1);
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg4: Write 0x%x", Buf);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 4: %!STATUS!", Status);
		goto exit;
	}

	// Handle changes now
	if (((Reg2.RegisterContent.Charger1 || Reg2.RegisterContent.Charger2) && Reg2.RegisterContent.Dongle) || Reg2.RegisterContent.PoweredCableWithUfp) {
		UcmPartnerType = UcmTypeCPartnerPoweredCableWithUfp;
	}
	else if (Reg2.RegisterContent.Charger1 || Reg2.RegisterContent.Charger2 || (Reg2.RegisterContent.PoweredCable && Reg2.RegisterContent.PoweredCableNoDfp)) {
		UcmPartnerType = UcmTypeCPartnerPoweredCableNoUfp;
	}
	else if (Reg2.RegisterContent.Dongle) {
		UcmPartnerType = UcmTypeCPartnerUfp;
	}
	else if (Reg2.RegisterContent.PoweredCable) {
		UcmPartnerType = UcmTypeCPartnerDfp;
	}

	Status = USBC_ChangeRole(ctx, UcmPartnerType, Reg7.RegisterContent.Polarity);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to set USB Role: %!STATUS!", Status);
		goto exit;
	}

exit:
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "UC120 interrupt acknowledged");
	UC120_InterruptHandled(ctx);

	// Set the flag finally
	ctx->Connected = TRUE;
}

void PlugDetInterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);
	NTSTATUS status;
	UCHAR Buf = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the PLUGDET");
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Got an interrupt from PLUGDET!");

	if (ctx->Connected) {
		// Reset the connector
		status = USBC_Detach(ctx);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, TRACE_INTERRUPT, "Failed to reset connector. Ignore anyway");
		}
		// Read register 5
		status = ReadRegister(ctx, 5, &Buf, 1);
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Reg5: Read 0x%x", Buf);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to read register 5: %!STATUS!", status);
		}
		// Write two registers
		Buf = 0x8;
		status = WriteRegister(ctx, 5, &Buf, 1);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 5: %!STATUS!", status);
		}

		Buf = 0x7;
		status = WriteRegister(ctx, 4, &Buf, 1);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INTERRUPT, "Failed to write register 4: %!STATUS!", status);
		}

		// Set flag
		ctx->Connected = FALSE;
	}
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
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Got an interrupt from Mystery 1! R5 48 responded");

	Status = WriteRegister(ctx, 5, &Unk, 1);
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
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Got an interrupt from Mystery 2!");
}