#include "Driver.h"
#include "SPI.h"
#include "SPI.tmh"

#include <spb.h>

#ifdef LEGACY_SPI_WRITE

#define IOCTL_QUP_SPI_CS_MANIPULATION 0x610

#define IOCTL_QUP_SPI_AUTO_CS     CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUP_SPI_ASSERT_CS   CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUP_SPI_DEASSERT_CS CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x0, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS ReadRegister(
	PDEVICE_CONTEXT ctx, 
	int reg, 
	void* value, 
	ULONG length
)
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
	for (int i = 0; i < 2; i++) {
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

NTSTATUS WriteRegister(
	PDEVICE_CONTEXT ctx, 
	int reg, 
	void* value, 
	ULONG length
)
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

#else

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

	// Params
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR command = (UCHAR) (Register << 3);

	WDFMEMORY SpbTransferOutputMemory;
	ULONG SpbTransferOutputMemorySize = 2 + Length;
	UCHAR* pSpbTransferOutputMemory = NULL;

	WDFMEMORY SpbTransferListMemory;
	WDF_MEMORY_DESCRIPTOR SpbTransferListMemoryDescriptor;

	// One register write and read
	SPB_TRANSFER_LIST_AND_ENTRIES(2) Sequence;
	SPB_TRANSFER_LIST_INIT(&(Sequence.List), 2);

	// Take lock
	WdfObjectAcquireLock(pContext->Device);

	// Allocate the memory that holds output buffer
	status = WdfMemoryCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		NonPagedPoolNx,
		'12CU',
		SpbTransferOutputMemorySize,
		&SpbTransferOutputMemory,
		(PVOID) pSpbTransferOutputMemory
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed %!STATUS!", status);
		goto exit;
	}

	{
		//
		// PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
		// "struct hack" size but using an index variable quiets 
		// the warning. This is a false positive from OACR.
		// 

		ULONG index = 0;
		Sequence.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
			SpbTransferDirectionToDevice,
			0,
			(PVOID) &command,
			(ULONG) 1
		);

		Sequence.List.Transfers[++index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
			SpbTransferDirectionFromDevice,
			0,
			(PVOID) &pSpbTransferOutputMemory,
			(ULONG) SpbTransferOutputMemorySize
		);
	}

	status = WdfMemoryCreatePreallocated(
		WDF_NO_OBJECT_ATTRIBUTES,
		(PVOID)&Sequence,
		sizeof(Sequence),
		&SpbTransferListMemory
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreatePreallocated failed %!STATUS!", status);
		WdfObjectDelete(SpbTransferOutputMemory);
		goto exit;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&SpbTransferListMemoryDescriptor, SpbTransferListMemory, 0);

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
		WdfObjectDelete(SpbTransferOutputMemory);
		goto exit;
	}

	RtlCopyMemory(Value, &pSpbTransferOutputMemory[2], Length);
	WdfObjectDelete(SpbTransferOutputMemory);

exit:
	WdfObjectReleaseLock(pContext->Device);
	return status;
}

NTSTATUS WriteRegisterFullDuplex(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
)
{
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
		NonPagedPoolNx,
		'12CU',
		Length + 1,
		&SpbTransferInputMemory,
		(PVOID)&pSpbTransferInputMemory
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed %!STATUS!", status);
		goto exit;
	}

	// First byte is command
	pSpbTransferInputMemory[0] = command;

	// The rest is content
	RtlCopyMemory(&pSpbTransferInputMemory[1], Value, Length);

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

#endif