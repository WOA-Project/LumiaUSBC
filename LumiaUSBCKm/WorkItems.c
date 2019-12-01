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
	//UC120_GetCurrentState(ctx, 1);
	//UC120_InterruptHandled(ctx);
}

void Mystery1InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	DbgPrint("LumiaUSBC: Got an interrupt from Mystery 1!\n");

	//UC120_GetCurrentRegisters(ctx, 1);
	UC120_GetCurrentState(ctx, 3);
	UC120_InterruptHandled(ctx);
}

void Mystery2InterruptWorkItem(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	PDEVICE_CONTEXT ctx = DeviceGetContext(AssociatedObject);

	DbgPrint("LumiaUSBC: Got an interrupt from Mystery 2!\n");

	//UC120_GetCurrentRegisters(ctx, 1);
	UC120_GetCurrentState(ctx, 4);
	UC120_InterruptHandled(ctx);
}