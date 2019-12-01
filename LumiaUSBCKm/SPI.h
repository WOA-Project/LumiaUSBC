EXTERN_C_START

NTSTATUS ReadRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length);
NTSTATUS WriteRegister(PDEVICE_CONTEXT ctx, int reg, unsigned char* value, ULONG length);

EXTERN_C_END