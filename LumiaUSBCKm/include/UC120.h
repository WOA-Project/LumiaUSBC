EXTERN_C_START

NTSTATUS UC120_InterruptHandled(PDEVICE_CONTEXT deviceContext);
NTSTATUS UC120_UploadCalibrationData(PDEVICE_CONTEXT deviceContext, unsigned char* calibrationFile, unsigned int length);

EXTERN_C_END