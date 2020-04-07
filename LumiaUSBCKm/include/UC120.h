EXTERN_C_START

NTSTATUS UC120_UploadCalibrationData(PDEVICE_CONTEXT deviceContext, unsigned char* calibrationFile, unsigned int length);
NTSTATUS UC120_SetVConn(PDEVICE_CONTEXT DeviceContext, BOOLEAN VConnStatus);
NTSTATUS UC120_SetPowerRole(PDEVICE_CONTEXT DeviceContext, BOOLEAN PowerRole);
NTSTATUS UC120_HandleInterrupt(PDEVICE_CONTEXT DeviceContext);
NTSTATUS UC120_ProcessIncomingPdMessage(PDEVICE_CONTEXT DeviceContext);
NTSTATUS UC120_ReadIncomingMessageStatus(PDEVICE_CONTEXT DeviceContext);

// Not yet understood what's this
NTSTATUS UC120_ToggleReg4Bit1(PDEVICE_CONTEXT DeviceContext, BOOLEAN PowerRole);

EXTERN_C_END