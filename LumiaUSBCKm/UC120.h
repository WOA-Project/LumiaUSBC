EXTERN_C_START

NTSTATUS UC120_GetCurrentRegisters(PDEVICE_CONTEXT deviceContext, unsigned int context);
NTSTATUS UC120_InterruptHandled(PDEVICE_CONTEXT deviceContext);
NTSTATUS UC120_InterruptsEnable(PDEVICE_CONTEXT deviceContext);
NTSTATUS UC120_InterruptsDisable(PDEVICE_CONTEXT deviceContext);
NTSTATUS UC120_D0Entry(PDEVICE_CONTEXT deviceContext);
NTSTATUS UC120_UploadCalibrationData(PDEVICE_CONTEXT deviceContext, unsigned char* calibrationFile, unsigned int length);

EXTERN_C_END