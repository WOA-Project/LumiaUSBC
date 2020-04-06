#include <Driver.h>
#include <SPI.h>
#include <UC120.h>

#ifndef DBG_PRINT_EX_LOGGING
#include "WorkItems.tmh"
#endif

#include <UC120Registers.h>
#include <USBRole.h>

void PmicInterrupt1WorkItem(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject)
{
  UNREFERENCED_PARAMETER(Interrupt);
  UNREFERENCED_PARAMETER(AssociatedObject);
}
