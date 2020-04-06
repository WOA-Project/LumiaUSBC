EXTERN_C_START

NTSTATUS LocalReadRegistryValue(PCWSTR registry_path, PCWSTR value_name, ULONG type,
	PVOID data, ULONG length);

EXTERN_C_END