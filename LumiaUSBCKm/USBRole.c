#include <Driver.h>
#include <Registry.h>
#include <Uc120GPIO.h>
#include <wchar.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "USBRole.tmh"
#endif

#include <USBRole.h>

NTSTATUS USBC_Detach(PDEVICE_CONTEXT deviceContext)
{
  NTSTATUS      status;
  unsigned char vbus = 0;

  // Turn off VBUS if needed
  SetGPIO(deviceContext, deviceContext->VbusGpio, &vbus);

  status = UcmConnectorChargingStateChanged(
      deviceContext->Connector, UcmChargingStateNotCharging);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_WARNING, TRACE_DRIVER,
        "UcmConnectorChargingStateChanged failed: %!STATUS!", status);
  }

  status = UcmConnectorTypeCDetach(deviceContext->Connector);
  if (!NT_SUCCESS(status)) {
    TraceEvents(
        TRACE_LEVEL_WARNING, TRACE_DRIVER,
        "UcmConnectorTypeCDetach failed: %!STATUS!", status);
  }

  return status;
}

NTSTATUS USBC_ChangeRole(
    PDEVICE_CONTEXT deviceContext, UCM_TYPEC_PARTNER target, unsigned int side)
{
  NTSTATUS           status = STATUS_SUCCESS;
  unsigned char      vbus   = 0;
  PCONNECTOR_CONTEXT connCtx;

  if (target == UcmTypeCPartnerUfp) {
    // Turn on VBUS
    vbus = (unsigned char)1;
  }
  else {
    // Turn off VBUS
    vbus = (unsigned char)0;
  }

  // Indicate the correct side
  unsigned char value = (unsigned char)side;
  SetGPIO(deviceContext, deviceContext->PolGpio, &value);

  // Turn on VBUS if needed
  SetGPIO(deviceContext, deviceContext->VbusGpio, &vbus);

  connCtx = ConnectorGetContext(deviceContext->Connector);

  if (connCtx->partner == target) {
    goto Exit;
  }

  connCtx->partner = target;

  if (target != UcmTypeCPartnerInvalid) {
    // Attach the new connector
    UCM_CONNECTOR_TYPEC_ATTACH_PARAMS Params;

    UCM_CONNECTOR_TYPEC_ATTACH_PARAMS_INIT(&Params, target);
    Params.CurrentAdvertisement = UcmTypeCCurrent3000mA;
    status = UcmConnectorTypeCAttach(deviceContext->Connector, &Params);
    if (!NT_SUCCESS(status)) {
      TraceEvents(
          TRACE_LEVEL_WARNING, TRACE_DRIVER,
          "UcmConnectorTypeCAttach failed to attach: %!STATUS!", status);
    }

    if (target == UcmTypeCPartnerPoweredCableNoUfp ||
        target == UcmTypeCPartnerPoweredCableWithUfp ||
        target == UcmTypeCPartnerDfp) {
#if 0
			UCM_PD_POWER_DATA_OBJECT Pdos[1];
			UCM_PD_POWER_DATA_OBJECT_INIT_FIXED(&Pdos[0]);

			Pdos[0].FixedSupplyPdo.VoltageIn50mV = 100;         // 5V
			Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = 300;  // 3000 mA - can be overridden in Registry
			status = UcmConnectorPdPartnerSourceCaps(deviceContext->Connector, Pdos, 1);
			if (!NT_SUCCESS(status)) {
				TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER, "UcmConnectorPdPartnerSourceCaps failed: %!STATUS!", status);
			}

			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS PdParams;
			UCM_PD_REQUEST_DATA_OBJECT StaticRdo;

			StaticRdo.FixedAndVariableRdo.OperatingCurrentIn10mA = 300;
			StaticRdo.FixedAndVariableRdo.MaximumOperatingCurrentIn10mA = 300;
			StaticRdo.Common.ObjectPosition = 1;

			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&PdParams, UcmPdConnStateNegotiationSucceeded);
			PdParams.ChargingState = UcmChargingStateNominalCharging;
			PdParams.Rdo = StaticRdo;
			
			status = UcmConnectorPdConnectionStateChanged(deviceContext->Connector, &PdParams);
			if (!NT_SUCCESS(status)) {
				TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER, "UcmConnectorPdConnectionStateChanged failed: %!STATUS!", status);
			}
#endif

      UcmConnectorPowerDirectionChanged(
          deviceContext->Connector, TRUE, UcmPowerRoleSink);
      status = UcmConnectorTypeCCurrentAdChanged(
          deviceContext->Connector, UcmTypeCCurrent3000mA);
      if (!NT_SUCCESS(status)) {
        TraceEvents(
            TRACE_LEVEL_WARNING, TRACE_DRIVER,
            "UcmConnectorTypeCCurrentAdChanged failed: %!STATUS!", status);
      }

      status = UcmConnectorChargingStateChanged(
          deviceContext->Connector, UcmChargingStateNominalCharging);
      if (!NT_SUCCESS(status)) {
        TraceEvents(
            TRACE_LEVEL_WARNING, TRACE_DRIVER,
            "UcmConnectorChargingStateChanged failed: %!STATUS!", status);
      }
    }
    else if (target == UcmTypeCPartnerUfp) {
#if 0
			UCM_PD_POWER_DATA_OBJECT Pdos[1];
			UCM_PD_POWER_DATA_OBJECT_INIT_FIXED(&Pdos[0]);

			Pdos[0].FixedSupplyPdo.VoltageIn50mV = 100;         // 5V
			Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = 50;  // 500 mA
			status = UcmConnectorPdSourceCaps(deviceContext->Connector, Pdos, 1);
			if (!NT_SUCCESS(status)) {
				TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER, "UcmConnectorPdSourceCaps failed: %!STATUS!", status);
			}

			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS PdParams;
			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&PdParams, UcmPdConnStateNegotiationSucceeded);
			PdParams.ChargingState = UcmChargingStateNotCharging;
			status = UcmConnectorPdConnectionStateChanged(deviceContext->Connector, &PdParams);
			if (!NT_SUCCESS(status)) {
				TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER, "UcmConnectorPdConnectionStateChanged failed: %!STATUS!", status);
			}
#endif

      UcmConnectorPowerDirectionChanged(
          deviceContext->Connector, TRUE, UcmPowerRoleSource);
      status = UcmConnectorTypeCCurrentAdChanged(
          deviceContext->Connector, UcmTypeCCurrentDefaultUsb);
      if (!NT_SUCCESS(status)) {
        TraceEvents(
            TRACE_LEVEL_WARNING, TRACE_DRIVER,
            "UcmConnectorTypeCCurrentAdChanged failed: %!STATUS!", status);
      }
    }
  }

Exit:
  return status;
}

NTSTATUS
LumiaUSBCSetDataRole(UCMCONNECTOR Connector, UCM_DATA_ROLE DataRole)
{
  PCONNECTOR_CONTEXT connCtx;

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetDataRole Entry");

  connCtx = ConnectorGetContext(Connector);

  RtlWriteRegistryValue(
      RTL_REGISTRY_ABSOLUTE, (PCWSTR)L"\\Registry\\Machine\\System\\usbc",
      L"DataRoleRequested", REG_DWORD, &DataRole, sizeof(ULONG));

  UcmConnectorDataDirectionChanged(Connector, TRUE, DataRole);

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetDataRole Exit");

  return STATUS_SUCCESS;
}

NTSTATUS
LumiaUSBCSetPowerRole(UCMCONNECTOR Connector, UCM_POWER_ROLE PowerRole)
{
  PCONNECTOR_CONTEXT connCtx;

  // UNREFERENCED_PARAMETER(PowerRole);

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetPowerRole Entry");

  connCtx = ConnectorGetContext(Connector);

  RtlWriteRegistryValue(
      RTL_REGISTRY_ABSOLUTE, (PCWSTR)L"\\Registry\\Machine\\System\\usbc",
      L"PowerRoleRequested", REG_DWORD, &PowerRole, sizeof(ULONG));

  UcmConnectorPowerDirectionChanged(Connector, TRUE, PowerRole);

  TraceEvents(
      TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "LumiaUSBCSetPowerRole Exit");

  return STATUS_SUCCESS;
}
