#include "Driver.h"
#include "SPI.h"
#include "SPI.tmh"

#include <spb.h>

NTSTATUS ReadRegister(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
)
{
	return ReadRegisterFullDuplex(pContext, Register, Value, Length);
}

NTSTATUS WriteRegister(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
)
{
	return WriteRegisterFullDuplex(pContext, Register, Value, Length);
}

NTSTATUS ReadRegisterFullDuplex(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "-> ReadRegisterFullDuplex");
	DbgPrint("-> ReadRegisterFullDuplex\n");

	// Params
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR command = (UCHAR) (Register << 3);

	WDFMEMORY SpbTransferOutputMemory;
	ULONG SpbTransferOutputMemorySize = 2 + Length;
	UCHAR* pSpbTransferOutputMemory = NULL;

	WDFMEMORY SpbTransferInputMemory;
	ULONG SpbTransferInputMemorySize = 1;
	UCHAR* pSpbTransferInputMemory = NULL;

	WDF_MEMORY_DESCRIPTOR SpbTransferListMemoryDescriptor;

	// One register write and read
	SPB_TRANSFER_LIST_AND_ENTRIES(2) Sequence;
	SPB_TRANSFER_LIST_INIT(&(Sequence.List), 2);

	// Take lock
	WdfObjectAcquireLock(pContext->Device);

	// Allocate the memory that holds output buffer
	status = WdfMemoryCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		PagedPool,
		'12CU',
		SpbTransferInputMemorySize,
		&SpbTransferInputMemory,
		&pSpbTransferInputMemory
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed %!STATUS!", status);
		DbgPrint("ReadRegisterFullDuplex: WdfMemoryCreate failed 0x%x\n", status);
		goto exit;
	}

	// Allocate the memory that holds output buffer
	status = WdfMemoryCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		PagedPool,
		'12CU',
		SpbTransferOutputMemorySize,
		&SpbTransferOutputMemory,
		&pSpbTransferOutputMemory
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed %!STATUS!", status);
		DbgPrint("ReadRegisterFullDuplex: WdfMemoryCreate failed 0x%x\n", status);
		WdfObjectDelete(SpbTransferInputMemory);
		goto exit;
	}

	RtlCopyMemory(pSpbTransferInputMemory, &command, 1);

	{
		//
		// PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
		// "struct hack" size but using an index variable quiets 
		// the warning. This is a false positive from OACR.
		// 

		Sequence.List.Transfers[0] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
			SpbTransferDirectionToDevice,
			0,
			pSpbTransferInputMemory,
			(ULONG)SpbTransferInputMemorySize
		);

		Sequence.List.Transfers[1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
			SpbTransferDirectionFromDevice,
			0,
			pSpbTransferOutputMemory,
			(ULONG) SpbTransferOutputMemorySize
		);
	}

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
		&SpbTransferListMemoryDescriptor,
		(PVOID) &Sequence,
		sizeof(Sequence)
	);

	status = WdfIoTargetSendIoctlSynchronously(
		pContext->Spi,
		NULL,
		IOCTL_SPB_FULL_DUPLEX,
		&SpbTransferListMemoryDescriptor,
		NULL,
		NULL,
		NULL
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfIoTargetSendIoctlSynchronously failed %!STATUS!", status);
		DbgPrint("ReadRegisterFullDuplex: WdfIoTargetSendIoctlSynchronously failed 0x%x\n", status);
		WdfObjectDelete(SpbTransferInputMemory);
		WdfObjectDelete(SpbTransferOutputMemory);
		goto exit;
	}

	RtlCopyMemory(Value, pSpbTransferOutputMemory + 2, Length);
	WdfObjectDelete(SpbTransferInputMemory);
	WdfObjectDelete(SpbTransferOutputMemory);

	if (Length == 1) {
		DbgPrint("ReadRegisterFullDuplex: register 0x%x value 0x%x\n", Register, (UCHAR) *((UCHAR*) Value));
	}

exit:
	WdfObjectReleaseLock(pContext->Device);
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<- ReadRegisterFullDuplex");
	DbgPrint("<- ReadRegisterFullDuplex\n");
	return status;
}

NTSTATUS WriteRegisterFullDuplex(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
)
{
	if (Value == NULL || Length < 1) {
		return STATUS_INVALID_PARAMETER;
	}

	// Params
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR command = (UCHAR) ((Register << 3) | 1);

	WDFMEMORY SpbTransferInputMemory;
	WDF_MEMORY_DESCRIPTOR SpbTransferInputMemoryDescriptor;
	UCHAR* pSpbTransferInputMemory;

	// Take lock
	WdfObjectAcquireLock(pContext->Device);

	status = WdfMemoryCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		PagedPool,
		'12CU',
		Length + 1,
		&SpbTransferInputMemory,
		&pSpbTransferInputMemory
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed %!STATUS!", status);
		goto exit;
	}

	// First byte is command
	pSpbTransferInputMemory[0] = command;

	// The rest is content
	RtlCopyMemory(pSpbTransferInputMemory + 1, Value, Length);

	// IOCTL
	status = WdfIoTargetSendWriteSynchronously(
		pContext->Spi,
		NULL,
		&SpbTransferInputMemoryDescriptor,
		0,
		NULL,
		NULL
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfIoTargetSendWriteSynchronously failed %!STATUS!", status);
		WdfObjectDelete(SpbTransferInputMemory);
		goto exit;
	}

	WdfObjectDelete(SpbTransferInputMemory);

exit:
	WdfObjectReleaseLock(pContext->Device);
	return status;
}
