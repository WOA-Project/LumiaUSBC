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

void Uc120InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the UC120");
	DbgPrint("LumiaUSBC: Got an interrupt from the UC120!\n");

	UC120_GetCurrentRegisters(ctx, UC120_NOTIFICATION);
	UC120_InterruptHandled(ctx);
}

void PlugDetInterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER((Interrupt, AssociatedObject));
	//PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the PLUGDET");
	DbgPrint("LumiaUSBC: Got an interrupt from PLUGDET!\n");

	//UC120_GetCurrentRegisters(ctx, 1);
	//UC120_InterruptHandled(ctx);
}

void Mystery1InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the Mystery 1");
	DbgPrint("LumiaUSBC: Got an interrupt from Mystery 1!\n");

	UC120_GetCurrentRegisters(ctx, UC120_MYSTERY1_VBUS);
	UC120_InterruptHandled(ctx);
}

void Mystery2InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INTERRUPT, "Got an interrupt from the Mystery 2");
	DbgPrint("LumiaUSBC: Got an interrupt from Mystery 2!\n");

	UC120_GetCurrentRegisters(ctx, UC120_MYSTERY2_HDMI);
	UC120_InterruptHandled(ctx);
}