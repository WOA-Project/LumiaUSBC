EXTERN_C_START

NTSTATUS ReadRegister(
	PDEVICE_CONTEXT ctx,
	int reg,
	void* value,
	ULONG length
);

NTSTATUS WriteRegister(
	PDEVICE_CONTEXT ctx,
	int reg,
	void* value,
	ULONG length
);

NTSTATUS ReadRegisterFullDuplex(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
);

NTSTATUS WriteRegisterFullDuplex(
	PDEVICE_CONTEXT pContext,
	int Register,
	PVOID Value,
	ULONG Length
);

EXTERN_C_END