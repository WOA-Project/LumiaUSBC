#include <Driver.h>
#include <SPI.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "SPI.tmh"
#endif

#include <spb.h>

NTSTATUS
ReadRegister(PDEVICE_CONTEXT pContext, int Register, PVOID Value, ULONG Length)
{
  return ReadRegisterFullDuplex(pContext, Register, Value, Length);
}

NTSTATUS
WriteRegister(PDEVICE_CONTEXT pContext, int Register, PVOID Value, ULONG Length)
{
  return WriteRegisterFullDuplex(pContext, Register, Value, Length);
}

NTSTATUS ReadRegisterFullDuplex(
    PDEVICE_CONTEXT pContext, int Register, PVOID Value, ULONG Length)
{
  // Params
  NTSTATUS status  = STATUS_SUCCESS;
  UCHAR    command = (UCHAR)(Register << 3);

  WDFMEMORY SpbTransferOutputMemory;
  ULONG     SpbTransferOutputMemorySize = 2 + Length;
  UCHAR *   pSpbTransferOutputMemory    = NULL;

  WDFMEMORY SpbTransferInputMemory;
  ULONG     SpbTransferInputMemorySize = 1;
  UCHAR *   pSpbTransferInputMemory    = NULL;

  WDF_MEMORY_DESCRIPTOR SpbTransferListMemoryDescriptor;
  WDF_REQUEST_SEND_OPTIONS RequestOptions;

  // One register write and read
  SPB_TRANSFER_LIST_AND_ENTRIES(2) Sequence;
  SPB_TRANSFER_LIST_INIT(&(Sequence.List), 2);

  // Take lock
  WdfObjectAcquireLock(pContext->Device);

  // Allocate the memory that holds output buffer
  status = WdfMemoryCreate(
      WDF_NO_OBJECT_ATTRIBUTES, NonPagedPoolNx, '12CU',
      SpbTransferInputMemorySize, &SpbTransferInputMemory,
      &pSpbTransferInputMemory);

  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed 0x%x", status);
    goto exit;
  }

  // Allocate the memory that holds output buffer
  status = WdfMemoryCreate(
      WDF_NO_OBJECT_ATTRIBUTES, NonPagedPoolNx, '12CU',
      SpbTransferOutputMemorySize, &SpbTransferOutputMemory,
      &pSpbTransferOutputMemory);

  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed 0x%x", status);
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

    Sequence.List.Transfers[0] = SPB_TRANSFER_LIST_ENTRY_INIT_NON_PAGED(
        SpbTransferDirectionToDevice, 0, pSpbTransferInputMemory,
        (ULONG)SpbTransferInputMemorySize);

    Sequence.List.Transfers[1] = SPB_TRANSFER_LIST_ENTRY_INIT_NON_PAGED(
        SpbTransferDirectionFromDevice, 0, pSpbTransferOutputMemory,
        (ULONG)SpbTransferOutputMemorySize);
  }

  WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
      &SpbTransferListMemoryDescriptor, (PVOID)&Sequence, sizeof(Sequence));

  WDF_REQUEST_SEND_OPTIONS_INIT(
      &RequestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
  RequestOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(561);

  status = WdfIoTargetSendIoctlSynchronously(
      pContext->Spi, NULL, IOCTL_SPB_FULL_DUPLEX,
      &SpbTransferListMemoryDescriptor, NULL, &RequestOptions, NULL);

  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WdfIoTargetSendIoctlSynchronously failed 0x%x", status);
    WdfObjectDelete(SpbTransferInputMemory);
    WdfObjectDelete(SpbTransferOutputMemory);
    goto exit;
  }

  RtlCopyMemory(Value, pSpbTransferOutputMemory + 2, Length);
  WdfObjectDelete(SpbTransferInputMemory);
  WdfObjectDelete(SpbTransferOutputMemory);

  if (Length == 1) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "ReadRegisterFullDuplex: register 0x%x value 0x%x", Register,
        (UCHAR) * ((UCHAR *)Value));
  }

exit:
  WdfObjectReleaseLock(pContext->Device);
  return status;
}

NTSTATUS WriteRegisterFullDuplex(
    PDEVICE_CONTEXT pContext, int Register, PVOID Value, ULONG Length)
{
  NTSTATUS status  = STATUS_SUCCESS;
  UCHAR    Command = (unsigned char)((Register << 3) | 1);

  WDFMEMORY                SpbTransferOutputMemory;
  WDF_MEMORY_DESCRIPTOR    SpbTransferOutputMemoryDescriptor;
  ULONG                    SpbTransferOutputMemorySize = 1 + Length;
  UCHAR *                  pSpbTransferOutputMemory    = NULL;
  WDF_REQUEST_SEND_OPTIONS RequestOptions;

  // Take Lock
  WdfObjectAcquireLock(pContext->Device);

  // Allocate the memory that holds output buffer
  status = WdfMemoryCreate(
      WDF_NO_OBJECT_ATTRIBUTES, NonPagedPoolNx, '12CU',
      SpbTransferOutputMemorySize, &SpbTransferOutputMemory,
      &pSpbTransferOutputMemory);

  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCreate failed 0x%x", status);
    goto exit;
  }

  RtlZeroMemory(pSpbTransferOutputMemory, SpbTransferOutputMemorySize);
  RtlCopyMemory(pSpbTransferOutputMemory, &Command, 1);
  RtlCopyMemory(pSpbTransferOutputMemory + 1, Value, Length);

  WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
      &SpbTransferOutputMemoryDescriptor, SpbTransferOutputMemory, 0);

  if (Length == 1) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WriteRegisterFullDuplex: register 0x%x value 0x%x", Register,
        (UCHAR) * ((UCHAR *)Value));
  }

  WDF_REQUEST_SEND_OPTIONS_INIT(
      &RequestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
  RequestOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(561);

  status = WdfIoTargetSendWriteSynchronously(
      pContext->Spi, NULL, &SpbTransferOutputMemoryDescriptor, NULL,
      &RequestOptions, NULL);

  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER,
        "WdfIoTargetSendWriteSynchronously failed 0x%x", status);
    WdfObjectDelete(SpbTransferOutputMemory);
    goto exit;
  }

exit:
  WdfObjectReleaseLock(pContext->Device);
  return status;
}
