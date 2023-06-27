import time

START_GUARD = 1
END_GUARD = 0


class EnergyGuard:
    """
    Object containing EnergyGuard information
    start: Start breakpoint of the energyguard
    stop: End breakpoint of the energyguard
    """
    start = None
    end = None

    def __init__(self, start, end):
        """
        Initialize start and end of the energyguard
        :param start:
        :param end:
        """
        self.start = start
        self.end = end


class EnergyGuardController(object):
    """
    Controller of the energyguard breakpoints
    """
    __start_breakpoints = []  # list of all starting breakpoints
    __end_breakpoints = []  # last of all ending breakpoints

    __guard_enabled = False    # currently in a energyguard
    
    def __init__(self):
        """
        Construct callbacks and energyGuard controller
        """
        self.__gdb_callback = None
        self.__breakpoint_callback = None

    def set_callbacks(self, gdb_proc, breakpoint_c):
        """
        Set callbacks
        :param gdb_proc:
        :param breakpoint_c:
        :return:
        """
        self.__gdb_callback = gdb_proc
        self.__breakpoint_callback = breakpoint_c

    def check(self, bkptnmr):
        """
        Check if breakpoint is part of an energyGuard
        :param bkptnmr:
        :return:
        """
        if bkptnmr in self.__start_breakpoints:
            return True
        elif bkptnmr in self.__end_breakpoints:
            return True
        else:
            return False

    def execute(self, bkpt_nmr):
        """
        Execute energyGuard on a breakpoint
        :param bkpt_nmr:
        :return:
        """

        print("Energy guard detected")
        print(f'breakpoint: {bkpt_nmr}')

        print(self.__start_breakpoints)
        print(self.__end_breakpoints)

        if bkpt_nmr in self.__start_breakpoints:
            if not self.__guard_enabled:
                print("Begin Energyguard")
                self.__gdb_callback.write('mon pre_bp_eg', raise_error_on_timeout=False)
                self.__gdb_callback.cont()
                self.__guard_enabled = True
            else:
                self.__gdb_callback.cont()

        elif bkpt_nmr in self.__end_breakpoints:
            if self.__guard_enabled:
                print("Stop Energyguard")
                self.__gdb_callback.write('mon delete_eg', raise_error_on_timeout=False)
                self.__gdb_callback.cont()
                self.__guard_enabled = False
            else:
                time.sleep(1)
                self.__gdb_callback.cont()

    def remove(self, bkpt_nmr1, bkpt_nmr2):
        """
        Remove an energyguard
        :param bkpt_nmr1:
        :param bkpt_nmr2:
        :return:
        """
        print(f"Removing breakpoint {bkpt_nmr1} and {bkpt_nmr2}")
        if bkpt_nmr1 in self.__start_breakpoints:
            if self.__delete_breakpoint(True, bkpt_nmr1):
                print(f"Removed breakpoint {bkpt_nmr1}")
        if bkpt_nmr2 in self.__end_breakpoints:
            if self.__delete_breakpoint(False, bkpt_nmr2):
                print(f"Removed breakpoint {bkpt_nmr2}")

    def __delete_breakpoint(self, start, bkpt_nmr):
        """
        Delete an energyguard breakpoint
        :param start:
        :param bkpt_nmr:
        :return:
        """
        if start == START_GUARD:
            if bkpt_nmr in self.__start_breakpoints:
                self.__start_breakpoints.remove(bkpt_nmr)
                self.__gdb_callback.write(f'd {bkpt_nmr}')
                return True
        elif start == END_GUARD:
            if bkpt_nmr in self.__end_breakpoints:
                self.__end_breakpoints.remove(bkpt_nmr)
                self.__gdb_callback.write(f'd {bkpt_nmr}')
                return True

        return False

    def append(self, function_1, function_2):
        """
        Append, create an energyguard
        :param function_1:
        :param function_2:
        :return:
        """
        b1 = self.__breakpoint_callback.set_breakpoint(function_1)
        b2 = self.__breakpoint_callback.set_breakpoint(function_2)
        
        if b1 and b2:
            self.__start_breakpoints.append(b1.number)
            self.__end_breakpoints.append(b2.number)
            print(f"Energy guard created with breakpoints {b1.number} and {b2.number}")
        else:
            self.__delete_breakpoint(START_GUARD, b1)
            self.__delete_breakpoint(END_GUARD, b2)
            print(f"Can't set energy guard between functions {function_1} and {function_2}")
