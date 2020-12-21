//****************************************************************************
// @Project Includes
//****************************************************************************

#include "MAIN.h"

//****************************************************************************
// @Defines
//****************************************************************************

#define RESET_INDICATOR     ((SCU_RSTSTAT.reg & ((uword)(0x0001001B))))
#define WATCHDOG_RESET      ((uword)0x00000008)
#define SOFTWARE_RESET      ((uword)0x00000010)
#define ESR0_RESET          ((uword)0x00000001)
#define ESR1_RESET          ((uword)0x00000002)
#define POWERON_RESET       ((uword)0x00010000)

//****************************************************************************
// @Global Variables
//****************************************************************************
volatile unsigned int DummyToForceRead; 

//****************************************************************************
// @Function      void MAIN_vInit(void) 
//
//----------------------------------------------------------------------------
// @Description   This function initializes the microcontroller.
//
//----------------------------------------------------------------------------
// @Returnvalue   None
//
//----------------------------------------------------------------------------
// @Parameters    None
//
//----------------------------------------------------------------------------
// @Date          12/18/2020
//
//****************************************************************************



void MAIN_vInit(void)
{
  ///  -----------------------------------------------------------------------
  ///  Clock System:
  ///  -----------------------------------------------------------------------
  ///  - external clock frequency: 20.00 MHz
  ///  - input divider (PDIV): 2
  ///  - PLL operation (VCOBYP = 0)
  ///  - VCO range: 400 MHz - 500 MHz
  ///  - feedback divider (NDIV): 40
  ///  - the VCO output frequency is: 400.00 MHz
  ///  - output divider (KDIV): 2
  ///  - CPU clock: 200.00 MHz
  ///  - the ratio fcpu /ffpi is  2 / 1
  ///  - the ratio fcpu /fsri is  1 / 1
  ///  - the ratio fcpu /fpcp is  1 / 1
  ///  - system clock: 100.00 MHz


   /// Comparing with the Compiler settings
   if(((SCU_PLLCON0.reg  & 0X1004E00) != 0X1004E00) || ((SCU_PLLCON1.reg & 0X20001) != 0X20001) \
       || ((SCU_CCUCON0.reg & 0X1) != 0X1))
   {

  //// - after a software reset PLL refuse to lock again unless oscillator is 
  ////   disconnected first

    MAIN_vResetENDINIT();
  SCU_PLLCON0.bits.VCOBYP  = 0;  // reset VCO bypass
  SCU_PLLCON0.bits.SETFINDIS  = 1; // disconnect OSC from PLL
    MAIN_vSetENDINIT();


    if (!SCU_PLLSTAT.bits.PWDSTAT)
    {
      MAIN_vResetENDINIT();
      SCU_CCUCON0.reg  = 0x00000001; // set FPI,SRI and PCP dividers
      SCU_PLLCON0.bits.VCOBYP  = 1; // set VCO bypass (goto Prescaler Mode)
      while (!SCU_PLLSTAT.bits.VCOBYST);// wait for prescaler mode
      SCU_PLLCON0.reg  = 0x01054E21; // set P,N divider, connect OSC
      SCU_PLLCON1.reg  = 0x00020001; // set K1,K2 divider
      MAIN_vSetENDINIT();
      while (SCU_PLLSTAT.bits.VCOLOCK == 0);// wait for LOCK
      MAIN_vResetENDINIT();
      SCU_PLLCON0.bits.VCOBYP  = 0; // Reset VCO bypass (Leave Prescaler Mode)
      MAIN_vSetENDINIT();
    }
   }

   MAIN_vResetENDINIT();
   WDT_CON1.reg = 0X8; // Disable watchdog timer
   MAIN_vSetENDINIT();

  ///  -----------------------------------------------------------------------
  ///  Interrupt System:
  ///  -----------------------------------------------------------------------
  ///  - four arbitration cycles (max. 255 interrupt sources)
  ///  - two clocks per arbitration cycle

  MTCR(0xFE2C, 0x00000000);      // load CPU interrupt control register
  ISYNC();

  ///  -----------------------------------------------------------------------
  ///  Peripheral Control Processor (PCP):
  ///  -----------------------------------------------------------------------
  ///  - stop the PCP internal clock when PCP is idle

  ///  - use Full Context save area (R[0] - R[7])
  ///  - start progam counter as left by last invocation
  ///  - channel watchdog is disabled
  ///  - maximum channel number checking is disabled

  MAIN_vResetENDINIT();
  PCP_CLC.reg    = 0x00000000;   // load PCP clock control register
  PCP_CS.reg     = 0x00000200;   // load PCP control and status register
  MAIN_vSetENDINIT();

  ///  - four arbitration cycles (max. 255 PCP channels)
  ///  - two clocks per arbitration cycle
  PCP_ICR.reg    = 0x00000000;   // load PCP interrupt control register

  ///  - the PCP warning mechanism is disabled
  PCP_ITR.reg    = 0x00000000;   // load PCP interrupt threshold register

  ///  - type of service of PCP node 4 is CPU interrupt
  PCP_SRC4.reg   = 0x00001000;   // load service request control register 4

  ///  - type of service of PCP node 5 is CPU interrupt
  PCP_SRC5.reg   = 0x00001000;   // load service request control register 5

  ///  - type of service of PCP node 6 is CPU interrupt
  PCP_SRC6.reg   = 0x00001000;   // load service request control register 6

  ///  - type of service of PCP node 7 is CPU interrupt
  PCP_SRC7.reg   = 0x00001000;   // load service request control register 7

  ///  - type of service of PCP node 8 is CPU interrupt
  PCP_SRC8.reg   = 0x00001000;   // load service request control register 8

  ///  -----------------------------------------------------------------------
  ///  Configuration of the DMA Module Clock:
  ///  -----------------------------------------------------------------------
  ///  - enable the DMA module

  MAIN_vResetENDINIT();
  DMA_CLC.reg    = 0x00000008;   // DMA clock control register
  DummyToForceRead  = DMA_CLC.reg; // read it back to ensure it is read
  MAIN_vSetENDINIT();

  //   -----------------------------------------------------------------------
  //   Initialization of the Peripherals:
  //   -----------------------------------------------------------------------
  //   initializes the MultiCAN Controller 
  CAN_vInit();

  ///  -----------------------------------------------------------------------
  ///  System Start Conditions:
  ///  -----------------------------------------------------------------------

  //// - the CPU interrupt system is globally disabled
  DISABLE();
}


//****************************************************************************
// @Function      void MAIN_vWriteWDTCON0(uword uwValue) 
//
//----------------------------------------------------------------------------
// @Description   This function writes the parameter uwValue to the WDT_CON0 
//                register which is password protected. 
//
//----------------------------------------------------------------------------
// @Returnvalue   None
//
//----------------------------------------------------------------------------
// @Parameters    uwValue: 
//                Value for the WDTCON0 register
//

void MAIN_vWriteWDTCON0(uword uwValue)
{
  uword uwDummy;

  uwDummy        = WDT_CON0.reg;
  uwDummy |=  0x000000F0;       //  set HWPW1 = 1111b

  if(WDT_CON1.bits.DR)
  {
    uwDummy |=  0x00000008;     //  set HWPW0 = WDTDR
  }
  else
  {
    uwDummy &= ~0x00000008;     //  set HWPW0 = WDTDR
  }
  if(WDT_CON1.bits.IR)
  {
    uwDummy |=  0x00000004;     //  set HWPW0 = WDTIR
  }
  else
  {
    uwDummy &= ~0x00000004;     //  set HWPW0 = WDTIR
  }

  uwDummy &= ~0x00000002;       //  set WDTLCK = 0
  WDT_CON0.reg =  uwDummy;          //  unlock access

  uwValue  |=  0x000000F2;      //  set HWPW1 = 1111b and WDTLCK = 1
  uwValue  &= ~0x0000000C;      //  set HWPW0 = 00b
  WDT_CON0.reg  =  uwValue;         //  write access and lock

}

struct bootloader_state boot;

void MAIN_sendCan(ubyte canData[]) {
	while(CAN_ubRequestMsgObj(1) != 1);
	CAN_vLoadData(1, canData);
	CAN_vTransmit(1);
}

void MAIN_CMD_readDeviceId() {
	ubyte i;
	ubyte canData[8];
	ubyte *device_id = (ubyte *)0xD0000000;
	canData[0] = 0x1;
	canData[1] = 0x0;
	for(i = 0; i < 6; i++) {
		canData[i + 2] = device_id[i];
	}
	MAIN_sendCan(canData);
	canData[1] = 0x1;
	for(i = 0; i < 6; i++) {
		canData[i + 2] = device_id[i + 6];
	}
	MAIN_sendCan(canData);
}

void MAIN_CMD_write32SetAddress(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16) | (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	ubyte canData[8];
	canData[0] = 0x4;
	canData[4] = (address>>24) & 0xFF;
	canData[3] = (address>>16) & 0xFF;
	canData[2] = (address>>8) & 0xFF;
	canData[1] = address & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
	boot.address = address;
	boot.status = WAIT_WRITE32_DATA;
}

void MAIN_CMD_write32SetData(CAN_SWObj *cur_msg) {
	uword data = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16) | (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	*(uword *)boot.address = data;
	ubyte canData[8];
	canData[0] = 0x4;
	canData[4] = (data>>24) & 0xFF;
	canData[3] = (data>>16) & 0xFF;
	canData[2] = (data>>8) & 0xFF;
	canData[1] = data & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

void MAIN_CMD_read32(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16) | (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	uword address_value = *(uword *)address;
	ubyte canData[8];
	canData[0] = 0x2;
	canData[4] = (address_value>>24) & 0xFF;
	canData[3] = (address_value>>16) & 0xFF;
	canData[2] = (address_value>>8) & 0xFF;
	canData[1] = address_value & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

void MAIN_processMessage(CAN_SWObj *cur_msg) {
	switch(boot.status) {
		case WAIT_COMMAND:
		{
			boot.command = cur_msg->ubData[0];
			switch(boot.command) {
				case READ_DEVICEID: // Read Device ID
				{
					MAIN_CMD_readDeviceId();
					break;
				}
				case READ_MEM32: // Read32 from Arbitrary Address
				{
					MAIN_CMD_read32(cur_msg);
					break;
				}
				case WRITE_MEM32: // Write32 to Arbitrary Address
				{
					MAIN_CMD_write32SetAddress(cur_msg);
					break;
				}
				default:
				{
					break;
				}
			}
			break;
		}
		case WAIT_WRITE32_DATA:
		{
			boot.command = cur_msg->ubData[0];
			switch(boot.command) {
				case WRITE_MEM32:
				{
					MAIN_CMD_write32SetData(cur_msg);
					break;
				}
				default:
				{
					ubyte canData[8];
					canData[0] = 0x4F;
					canData[4] = 0xFF;
					canData[3] = 0xFF;
					canData[2] = 0xFF;
					canData[1] = 0xFF;
					canData[5] = 0xFF;
					canData[6] = 0xFF;
					canData[7] = 0xFF;
					MAIN_sendCan(canData);
					break;
				}
			}
			boot.address = 0;
			boot.status = WAIT_COMMAND;
			break;
		}
		default:
		{
			break;
		}
	}
}

sword main(void)
{

  MAIN_vInit();
  CAN_SWObj cur_msg;
  memset(&boot, 0x00, sizeof(struct bootloader_state));
  do {
	  if(CAN_ubNewData(0)) { // check for new message
		  CAN_vGetMsgObj(0, &cur_msg); // copy and free to allow next msg into slot
		  CAN_vReleaseObj(0);
		  MAIN_processMessage(&cur_msg);
	  }
  } while (1);
} //  End of function main