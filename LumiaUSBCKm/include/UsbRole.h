EXTERN_C_START

NTSTATUS USBC_ChangeRole(
    PDEVICE_CONTEXT deviceContext, UCM_TYPEC_PARTNER target, unsigned int side);
NTSTATUS USBC_Detach(PDEVICE_CONTEXT deviceContext);

EXTERN_C_END