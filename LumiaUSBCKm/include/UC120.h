EXTERN_C_START

NTSTATUS UC120_UploadCalibrationData(
    PDEVICE_CONTEXT deviceContext, unsigned char *calibrationFile,
    unsigned int length);

NTSTATUS UC120_SetVConn(PDEVICE_CONTEXT DeviceContext, UCHAR VConnStatus);

NTSTATUS UC120_SetPowerRole(PDEVICE_CONTEXT DeviceContext, UCHAR PowerRole);

NTSTATUS
UC120_SetDataRole(PDEVICE_CONTEXT DeviceContext, UCHAR DataRole);

NTSTATUS UC120_HandleInterrupt(PDEVICE_CONTEXT DeviceContext);

NTSTATUS UC120_ProcessIncomingPdMessage(PDEVICE_CONTEXT DeviceContext);

NTSTATUS UC120_ReadIncomingMessageStatus(PDEVICE_CONTEXT DeviceContext);

NTSTATUS UC120_ToggleReg4Bit1(PDEVICE_CONTEXT DeviceContext, UCHAR PowerRole);

NTSTATUS
Uc120_ReadIncomingPdMessage(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);

EXTERN_C_END