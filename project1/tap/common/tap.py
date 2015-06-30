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
        
        pass
       
    def reset(self):
        """ set TAP state to Test_Logic_Reset """
        # assert TMS for 5 TCKs in a row
        pass

    def reset2ShiftIR(self):
        """ shift TAP state from reset to shiftIR """
        
        pass 

    def exit1IR2ShiftDR(self):
        """ shift TAP state from exit1IR to shiftDR """

        pass

    def exit1DR2ShiftIR(self):
        """ shift TAP state from exit1DR to shiftIR """
        
        pass

    def shiftInData(self, tdi_str):    
        """ shift in IR/DR data

        :param tdi_str: TDI data to shift in
        :type tdo_str: str

        """

        pass

    def shiftOutData(self, length):
        """ get IR/DR data

        :param length: chain length        
        :type length: int
        :returns: int - TDO data

        """

        return 0

    def getChainLength(self):
        """ get chain length

        :returns: int -- chain length	

        """

        return 0
