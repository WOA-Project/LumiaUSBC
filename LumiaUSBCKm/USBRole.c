/*++

Module Name:

	USBRole.c - USB Role Switch rountines for the UC120 driver.

Abstract:

   This file contains the role switch routines.

Environment:

	Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "GPIO.h"
#include "Registry.h"
#include <wchar.h>
#include "USBRole.tmh"

NTSTATUS USBC_ChangeRole(PDEVICE_CONTEXT deviceContext, UCM_TYPEC_PARTNER target, unsigned int side)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char data = 0;
	unsigned char vbus = 0;
	PCONNECTOR_CONTEXT connCtx;

	if (target == UcmTypeCPartnerUfp)
	{
		// Turn on VBUS
		vbus = (unsigned char)1;
	}
	else
	{
		// Turn off VBUS
		vbus = (unsigned char)0;
	}

	// Indicate the correct side
	unsigned char value = (unsigned char)side;
	SetGPIO(deviceContext, deviceContext->PolGpio, &value);

	// Commented out for safety until we're sure everything is working as expected.
	SetGPIO(deviceContext, deviceContext->VbusGpio, &vbus);

	connCtx = ConnectorGetContext(deviceContext->Connector);

	if (connCtx->partner == target)
	{
		goto Exit;
	}

	connCtx->partner = target;

	// Remove the previous connector
	UcmConnectorTypeCDetach(deviceContext->Connector);

	if (target != UcmTypeCPartnerInvalid)
	{
		// Attach the new connector
		UCM_CONNECTOR_TYPEC_ATTACH_PARAMS Params;

		UCM_CONNECTOR_TYPEC_ATTACH_PARAMS_INIT(&Params, target);
		Params.CurrentAdvertisement = UcmTypeCCurrentDefaultUsb;
		status = UcmConnectorTypeCAttach(deviceContext->Connector, &Params);

		if (target == UcmTypeCPartnerPoweredCableNoUfp || target == UcmTypeCPartnerPoweredCableWithUfp || target == UcmTypeCPartnerDfp)
		{
			UCM_PD_POWER_DATA_OBJECT Pdos[1];
			UCM_PD_POWER_DATA_OBJECT_INIT_FIXED(&Pdos[0]);

			Pdos[0].FixedSupplyPdo.VoltageIn50mV = 100;         // 5V
			Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = 50;  // 500 mA - can be overridden in Registry
			if (NT_SUCCESS(MyReadRegistryValue(
				(PCWSTR)L"\\Registry\\Machine\\System\\usbc",
				(PCWSTR)L"ChargeCurrent",
				REG_DWORD,
				&data,
				sizeof(ULONG))))
			{
				Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = data / 10;
			}
			status = UcmConnectorPdPartnerSourceCaps(deviceContext->Connector, Pdos, 1);
			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS PdParams;
			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&PdParams, UcmPdConnStateNotSupported);
			PdParams.ChargingState = UcmChargingStateNominalCharging;
			status = UcmConnectorPdConnectionStateChanged(deviceContext->Connector, &PdParams);
			UcmConnectorPowerDirectionChanged(deviceContext->Connector, TRUE, UcmPowerRoleSink);
			status = UcmConnectorTypeCCurrentAdChanged(deviceContext->Connector, UcmTypeCCurrent3000mA);
			status = UcmConnectorChargingStateChanged(deviceContext->Connector, PdParams.ChargingState);
		}
		else if (target == UcmTypeCPartnerUfp)
		{
			UCM_PD_POWER_DATA_OBJECT Pdos[1];
			UCM_PD_POWER_DATA_OBJECT_INIT_FIXED(&Pdos[0]);

			Pdos[0].FixedSupplyPdo.VoltageIn50mV = 100;         // 5V
			Pdos[0].FixedSupplyPdo.MaximumCurrentIn10mA = 50;  // 500 mA
			status = UcmConnectorPdSourceCaps(deviceContext->Connector, Pdos, 1);
			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS PdParams;
			UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&PdParams, UcmPdConnStateNotSupported);
			PdParams.ChargingState = UcmChargingStateNotCharging;
			status = UcmConnectorPdConnectionStateChanged(deviceContext->Connector, &PdParams);
			UcmConnectorPowerDirectionChanged(deviceContext->Connector, TRUE, UcmPowerRoleSource);
			status = UcmConnectorChargingStateChanged(deviceContext->Connector, PdParams.ChargingState);
		}
	}

Exit:
	return status;
}