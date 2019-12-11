
EXTERN_C_START

// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)

// Control Register 0
// Write only
#include <pshpack1.h>
typedef struct _UC120_REG0
{
	union
	{
		BYTE RegisterData; // Write 0x20 for hard reset
		struct
		{
			unsigned int VDMEnterMode : 1; // Used only when operating as a DFP. Must be set once the EC reads the modes and loads the VDM configuration register
			unsigned int VDMExitAltMode : 1; // Used only when operating as a DFP. When in Alternate Port mode, exit Alt mode and switch to USB configuration
			unsigned int VDMStartAltMode : 1; // Used only when operating as a DFP. Start Structured VDM messaging sequence to enter Alternate Port Mode. The DFP SVID register must be loaded with the SVID to be checked for before this bit is set
			unsigned int HardResetRequest : 1; // If EC wants to reset the device after some config changes, then hard reset is required to reset the port partner
			unsigned int DRSwapRequest : 1; // Request the partner for a Data Role swap (swap between DFP and UFP)
			unsigned int PRSwapRequest : 1; // Request the partner port for a Port Role Swap, if attached partner port is a Dual-Role port (swap between PD Sourceand Sink roles)
			unsigned int GetSourceCaps : 1; // Get the port partner source capabilities and update in the partner port source capabilities register
			unsigned int GetSinkCaps : 1; // Get the port partner sink capabilities and update in the partner port sink capabilities register
		} RegisterContent;
	};
} UC120_REG0, * PUC120_REG0;

// Control Register 1
// Write only
typedef struct _UC120_REG1
{
	union
	{
		BYTE RegisterData;
		struct
		{
			unsigned int VDMConfig : 1;
			unsigned int PoxerTxnReqDone : 1;
			unsigned int VDMSendAttention : 1;
			unsigned int VDMEnterBillBoard : 1;
			unsigned int Reserved0 : 1;
			unsigned int ModeNum0 : 1;
			unsigned int ModeNum1 : 1;
			unsigned int ModeNum2 : 1;
		} RegisterContent;
	};
} UC120_REG1, * PUC120_REG1;

// Status Register 0
// Read only

// This register reflects the status of the PD manager.

typedef struct _UC120_REG2
{
	union
	{
		BYTE RegisterData;
		struct
		{
			unsigned int Reserved0 : 1;
			unsigned int Reserved1 : 1;
			unsigned int Charger1 : 1;
			unsigned int Dongle : 1;
			unsigned int Charger2 : 1;
			unsigned int Reserved3 : 3;
		} RegisterContent;
	};
} UC120_REG2, * PUC120_REG2;

// Status Register 1
// Read only

// This status register contains the error flags. On the event of interrupt and ‘Error’ bit of the Interrupt request register
// is set, status register 1 should be set to know the source of error.

typedef struct _UC120_REG3
{
	union
	{
		BYTE RegisterData; // Cleared to 0xFF on interrupt enable
		struct
		{
			unsigned int CapsMismatch : 1; // A Capability Mismatch occurs when the Sink cannot satisfy its power requirements from the capabilities offered by the Source.
			unsigned int SwapStatus : 2;   // Indicates a ACCEPT/WAIT/REJECT message was received for the corresponding SWAP request made.
										   // 00 = NA - 01 = ACCEPT received - 10 = WAIT received - 11 = REJECT received
			BYTE Reserved0 : 6;
		} RegisterContent;
	};
} UC120_REG3, * PUC120_REG3;

typedef struct _UC120_REG4
{
	union
	{
		BYTE RegisterData;
		struct
		{
			unsigned int EnableInterrupts : 1;
			unsigned int InterruptWorkerArgument6 : 1; // Set to 6th parameter of InterruptWorker (5th IsCableConnected data)
			BYTE Reserved0 : 5;
			unsigned int GoodCRC : 1;
		} RegisterContent;
	};
} UC120_REG4, * PUC120_REG4;

typedef struct _UC120_REG5 // R/W Register
{
	union
	{
		BYTE RegisterData;
		struct
		{
			unsigned int PowerRole : 1;
			BYTE Reserved0 : 1;
			unsigned int DataRole : 1;
			unsigned int D0Enterred : 1;
			BYTE Reserved1 : 1;
			unsigned int VconnRole : 1;
			unsigned int PM1InterruptTriggered : 1;                 // Set on PM1 interrupt, cleared on PM2, Sleep-related?
			unsigned int D0EnterredAndInterruptsYetToBeEnabled : 1; // Cleared on interrupt enable, set on D0 entry 
		} RegisterContent;
	};
} UC120_REG5, * PUC120_REG5;

// Interrupt Request Register 0
// R/W

// Every bit of the Interrupt Request Register is capable of generating an interrupt. The bit which is set will be cleared
// on a write to this register by the EC.

typedef struct _UC120_REG6
{
	union
	{
		BYTE RegisterData; // Cleared to 0xFF on interrupt enable
		struct
		{
			unsigned int CableDetach : 1;              // Set when the USB cable connection is detached. Cleared on a write to this register by EC.
			unsigned int CableAttach : 1;              // Set when a USB cable connection is detected. Cleared on a write to this register by EC.
			unsigned int DRSwapRequestReceived : 1;    // Set when the attached port partner requests for a data role swap. Cleared on a write to this register by EC.
			unsigned int PRSwapRequestReceived : 1;    // Set when the attached port partner requests for a port role swap. Cleared on a write to this register by EC.
			unsigned int VconnSwapRequestReceived : 1; // Set when the Vconn swap request received. Cleared on a write to this register by EC.
			unsigned int PowerTransitionRequest : 1;   // Will be set when there is a PD manager wants the EC to transition the power supply from one level to another during the PD process with a port partner. The EC will have to read the ‘Power Transition Request’ registers to know
													   // the requested power levels, once this bit is set.Cleared on a write to this register by EC.
			unsigned int CurrentRatingUpdate : 1;      // Will be set when there is a new current advertisement from the DFP by increasing the CC line voltage as given in the Type-C spec.
			unsigned int PDContract : 1;               // Will be set when a PD contract has been established once a USB connection is detected. Cleared on a write to this register by EC.
		} RegisterContent;
	};
} UC120_REG6, * PUC120_REG6;

// Interrupt Requesr Register 1
// R/W

// Every bit of the Interrupt Request Register is capable of generating an interrupt. The bit which is set will be cleared
// on a write to this register by the EC.

typedef struct _UC120_REG7
{
	union
	{
		BYTE RegisterData; // Read on interrupt enable
		struct
		{
			unsigned int Reserved0 : 4;
			unsigned int SkipPDNegotiation : 1;
			unsigned int CableType : 1;
			unsigned int Polarity : 1;
			unsigned int Reserved1 : 1;
		} RegisterContent;
	};
} UC120_REG7, * PUC120_REG7;

typedef struct _UC120_REG8
{
	union
	{
		BYTE RegisterData;
		struct
		{
			BYTE Reserved0 : 8;
		} RegisterContent;
	};
} UC120_REG8, * PUC120_REG8;

typedef struct _UC120_REG9 // CC1
{
	union
	{
		BYTE RegisterData;
		struct
		{
			BYTE Reserved0 : 8;
		} RegisterContent;
	};
} UC120_REG9, * PUC120_REG9;

typedef struct _UC120_REG10 // CC2
{
	union
	{
		BYTE RegisterData;
		struct
		{
			BYTE Reserved0 : 8;
		} RegisterContent;
	};
} UC120_REG10, * PUC120_REG10;

typedef struct _UC120_REG11
{
	union
	{
		BYTE RegisterData;
		struct
		{
			BYTE Reserved0 : 8;
		} RegisterContent;
	};
} UC120_REG11, * PUC120_REG11;

// Status Register 2
typedef struct _UC120_REG12
{
	union
	{
		BYTE RegisterData;
		struct
		{
			BYTE Reserved0 : 8;
		} RegisterContent;
	};
} UC120_REG12, * PUC120_REG12;

typedef struct _UC120_REG13
{
	union
	{
		BYTE RegisterData;
		struct
		{
			unsigned int D0NotEnterred : 1;
			unsigned int D0Enterred : 1;
			BYTE Reserved0 : 6;
		} RegisterContent;
	};
} UC120_REG13, * PUC120_REG13;
#include <poppack.h>

typedef struct _UC120_REGISTERS
{
	BYTE Register0;
	BYTE Register1;
	BYTE Register2;
	BYTE Register3;
	BYTE Register4;
	BYTE Register5;
	BYTE Register6;
	BYTE Register7;
	BYTE Register8;
	BYTE Register9;
	BYTE Register10;
	BYTE Register11;
	BYTE Register12;
	BYTE Register13;
} UC120_REGISTERS, * PUC120_REGISTERS;

typedef struct _UC120_REGISTERS_PARSED
{
	UC120_REG0 Register0;
	UC120_REG1 Register1;
	UC120_REG2 Register2;
	UC120_REG3 Register3;
	UC120_REG4 Register4;
	UC120_REG5 Register5;
	UC120_REG6 Register6;
	UC120_REG7 Register7;
	UC120_REG8 Register8;
	UC120_REG9 Register9;
	UC120_REG10 Register10;
	UC120_REG11 Register11;
	UC120_REG12 Register12;
	UC120_REG13 Register13;
} UC120_REGISTERS_PARSED, * PUC120_REGISTERS_PARSED;

EXTERN_C_END