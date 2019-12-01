#include "Driver.h"
#include "SPI.tmh"

#define IOCTL_QUP_SPI_CS_MANIPULATION 0x610

#define IOCTL_QUP_SPI_AUTO_CS     CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUP_SPI_ASSERT_CS   CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUP_SPI_DEASSERT_CS CTL_CODE(FILE_DEVICE_CONTROLLER, IOCTL_QUP_SPI_CS_MANIPULATION | 0x0, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS ReadRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
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

NTSTATUS WriteRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length)
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