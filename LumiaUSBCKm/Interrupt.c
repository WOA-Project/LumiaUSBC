#include <Driver.h>

#ifndef RESHUB_USE_HELPER_ROUTINES
#define RESHUB_USE_HELPER_ROUTINES
#endif

#include <reshub.h>
#include <wdf.h>

#include <GPIO.h>
#include <Registry.h>
#include <SPI.h>
#include <UC120.h>
#include <Uc120GPIO.h>
#include <WorkItems.h>

#include "Interrupt.tmh"

BOOLEAN EvtUc120InterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);
  UNREFERENCED_PARAMETER(Interrupt);

  return TRUE;
}

BOOLEAN EvtPlugDetInterruptIsr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);
  UNREFERENCED_PARAMETER(Interrupt);

  // It actually does nothing
  return TRUE;
}

BOOLEAN EvtPmicInterrupt1Isr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);

  WdfInterruptQueueWorkItemForIsr(Interrupt);
  return TRUE;
}

BOOLEAN EvtPmicInterrupt2Isr(WDFINTERRUPT Interrupt, ULONG MessageID)
{
  UNREFERENCED_PARAMETER(MessageID);
  UNREFERENCED_PARAMETER(Interrupt);

  return TRUE;
}
