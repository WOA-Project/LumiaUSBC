#include <driver.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "driver.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, LumiaUSBCKmEvtDeviceAdd)
#pragma alloc_text(PAGE, LumiaUSBCKmEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
  WDF_DRIVER_CONFIG     config;
  NTSTATUS              status;
  WDF_OBJECT_ATTRIBUTES attributes;

#ifndef DBG_PRINT_EX_LOGGING
  //
  // Initialize WPP Tracing
  //
  WPP_INIT_TRACING(DriverObject, RegistryPath);
#endif

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "DriverEntry entry\n");


  //
  // Register a cleanup callback so that we can call WPP_CLEANUP when
  // the framework driver object is deleted during driver unload.
  //
  WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
  attributes.EvtCleanupCallback = LumiaUSBCKmEvtDriverContextCleanup;

  WDF_DRIVER_CONFIG_INIT(&config, LumiaUSBCKmEvtDeviceAdd);

  status = WdfDriverCreate(
      DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);

  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!",
        status);

#ifndef DBG_PRINT_EX_LOGGING
    WPP_CLEANUP(DriverObject);
#endif
    return status;
  }

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");
  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL,
      "DriverEntry exit: 0x%x\n", status);

  return status;
}

NTSTATUS
LumiaUSBCKmEvtDeviceAdd(
    _In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
  NTSTATUS status;

  UNREFERENCED_PARAMETER(Driver);

  PAGED_CODE();

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
  DbgPrintEx(DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCKmEvtDeviceAdd entry\n");

  status = LumiaUSBCKmCreateDevice(DeviceInit);

  DbgPrintEx(
      DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "LumiaUSBCKmEvtDeviceAdd exit: 0x%x\n", status);
  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

  return status;
}

VOID LumiaUSBCKmEvtDriverContextCleanup(_In_ WDFOBJECT DriverObject)
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
  UNREFERENCED_PARAMETER(DriverObject);

  PAGED_CODE();

  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

#ifndef DBG_PRINT_EX_LOGGING
  //
  // Stop WPP Tracing
  //
  WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
#endif
}
