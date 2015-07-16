##########################################################
# PSU ECE510 Post-silicon Validation Project 1
# --------------------------------------------------------
# Filename: tap.py
# --------------------------------------------------------
# Purpose: TAP Controller Class
##########################################################

from tap.common.tap_gpio import *
from tap.log.logging_setup import *
import time

class Tap(Tap_GPIO):
    """ Class for JTAG TAP Controller"""

    def __init__(self,log_level=logging.INFO):
        """ initialize TAP """
        self.logger = get_logger(__file__,log_level)
        self.max_length = 1000

        #set up the RPi TAP pins
        Tap_GPIO.__init__(self)

    def toggle_tck(self, tms, tdi):
        """ toggle TCK for state transition 
        :param tms: data for TMS pin
        :type tms: int (0/1)
        :param tdi: data for TDI pin
        :type tdi: int (0/1)
        """
        
    def reset(self):
        """ set TAP state to Test_Logic_Reset """
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        # assert TMS for 5 TCKs in a row
        for i in range(0,5):
            Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
            Tap_GPIO.delay(0.01)
            Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
            Tap_GPIO.delay(0.01)

    def reset2ShiftIR(self):
        """ shift TAP state from reset to shiftIR """
        #Reset to Run-test/Idle
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Run-test/Idle to Select DR-Scan
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Select DR-Scan to Select IR-Scan
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Select IR-Scan to Capture-IR
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Capture-IR to Shift-IR
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
    def exit1IR2ShiftDR(self):
        """ shift TAP state from exit1IR to shiftDR """
        #Exit1-IR to Update-IR
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Update-IR to Select DR-Scan
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Select DR-Scan to Capture-DR
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Capture-DR to Shift-DR-Scan
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
    def exit1DR2ShiftIR(self):
        """ shift TAP state from exit1DR to shiftIR """
        #Exit1-DR to Update-DR
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Update-DR to Select DR-Scan
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Select DR-Scan to Select IR-Scan
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Select IR-Scan to Capture-IR
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)
        
        #Capture-IR to Shift-IR
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)
        Tap_GPIO.delay(0.01)

    def shiftInData(self, tdi_str):    
        """ shift in IR/DR data

        :param tdi_str: TDI data to shift in
        :type tdo_str: str

        """
        for i in range(len(tdi_str)-1):
            Tap_GPIO.set_io_data(self,tms=0,tdi=int(tdi_str[i]),tck=1)
            Tap_GPIO.delay(0.01)
            Tap_GPIO.set_io_data(self,tms=0,tdi=int(tdi_str[i]),tck=0)
            Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=int(tdi_str[5]),tck=1)
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=int(tdi_str[5]),tck=0)
        Tap_GPIO.delay(0.01)


    def shiftOutData(self, length):
        """ get IR/DR data

        :param length: chain length        
        :type length: int
        :returns: int - TDO data

        """
        rslt=0
        for i in range(0,length-1):
            Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=1)    
            rslt=rslt|(Tap_GPIO.read_tdo_data(self) << i)
            Tap_GPIO.delay(0.01)
            Tap_GPIO.set_io_data(self,tms=0,tdi=0,tck=0)    
            Tap_GPIO.delay(0.01)
        rslt=rslt|(Tap_GPIO.read_tdo_data(self) << 31)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=1)    
        Tap_GPIO.delay(0.01)
        Tap_GPIO.set_io_data(self,tms=1,tdi=0,tck=0)    
        Tap_GPIO.delay(0.01)
        return rslt

    def getChainLength(self):
        """ get chain length
        :returns: int -- chain length    
        """

        return 0
