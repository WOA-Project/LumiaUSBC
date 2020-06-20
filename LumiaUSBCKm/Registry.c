#include <Driver.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "Registry.tmh"
#endif

NTSTATUS LocalReadRegistryValue(
    PCWSTR registry_path, PCWSTR value_name, ULONG type, PVOID data,
    ULONG length)
{
  UNICODE_STRING                 valname;
  UNICODE_STRING                 keyname;
  OBJECT_ATTRIBUTES              attribs;
  PKEY_VALUE_PARTIAL_INFORMATION pinfo;
  HANDLE                         handle;
  NTSTATUS                       rc;
  ULONG                          len, reslen;

  RtlInitUnicodeString(&keyname, registry_path);
  RtlInitUnicodeString(&valname, value_name);

  InitializeObjectAttributes(
      &attribs, &keyname, OBJ_CASE_INSENSITIVE, NULL, NULL);
  rc = ZwOpenKey(&handle, KEY_QUERY_VALUE, &attribs);
  if (!NT_SUCCESS(rc))
    return 0;

  len   = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + length;
  pinfo = ExAllocatePoolWithTag(NonPagedPool, len, 'IgeR');
  rc    = ZwQueryValueKey(
      handle, &valname, KeyValuePartialInformation, pinfo, len, &reslen);
  if ((NT_SUCCESS(rc) || rc == STATUS_BUFFER_OVERFLOW) && pinfo != NULL &&
      reslen >= (sizeof(KEY_VALUE_PARTIAL_INFORMATION) - 1) &&
      (!type || pinfo->Type == type)) {
    reslen = pinfo->DataLength;
    memcpy(data, pinfo->Data, min(length, reslen));
  }
  else {
    reslen = 0;
  }

  if (pinfo != NULL) {
    ExFreePool(pinfo);
  }

  ZwClose(handle);
  return rc;
}
