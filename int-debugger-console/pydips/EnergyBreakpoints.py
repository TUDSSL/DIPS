from pydips.Breakpoints import Breakpoint


class EnergyBreakpoint:
    """
    Energy Breakpoint object,
    consist of threshold and breakpoint
    """

    threshold = 2000
    breakpoint = None

    def __init__(self, bkpt, threshold):
        self.threshold = threshold
        self.breakpoint = bkpt


class EnergyBreakpointsController:
    """
    Controller for setting energybreakpoints
    """

    __gdb = None  # set GDB Controller Callback
    __bc = None  # set Breakpoint Controller Callback
    __breakpoints = {}  # object with breakpoints

    def __init__(self):
        """
        Construct enerygbreakpoint controller
        """
        return

    def set_callbacks(self, callback_gdb, callback_bc):
        """
        Set callbacks to GDB and breakpoints controllers
        :param callback_gdb: class
        :param callback_bc: class
        :return:
        """
        self.__gdb = callback_gdb
        self.__bc = callback_bc

    def check(self, bkpt_nmr):
        """
        Check if current breakpoint is an energybreakpoint
        :param bkpt_nmr:
        :return:
        """
        if bkpt_nmr in self.__breakpoints.keys():
            return True

    def execute(self, bkpt_nmr):
        """
        Execute breakpoint as energybreakpoint
        :param bkpt_nmr:
        :return:
        """
        bkpt = self.__breakpoints[bkpt_nmr]
        print(f"EB: Threshold {bkpt.threshold}mV\n")
        output = self.__gdb.write("mon voltage")

        voltage = False

        # print(f"output: {output}\n")

        for out in output[1]:
            if out['type'] == 'target':
                try:
                    voltage = int(out['payload'])
                except:
                    voltage = False

        if voltage:
            if voltage > bkpt.threshold:
                self.__gdb.cont()

            else:
                print(f"EB: Voltage {voltage}mV under threshold {bkpt.threshold}mV")
                return
        else:
            print(f"Error reading voltage, continue..")
            self.__gdb.cont()

    def append(self, function, threshold=2000):
        """
        Create a new energybreakpoint
        :param function:
        :param threshold:
        :return:
        """
        bkpt = self.__bc.set_breakpoint(function)

        if bkpt:
            e_bkpt = EnergyBreakpoint(bkpt, threshold)
            self.__breakpoints[bkpt.number] = e_bkpt
            return True

        return False

    def remove(self, bkpt_nmr):
        """
        Remove energybreakpoint
        :param bkpt_nmr:
        :return:
        """
        if bkpt_nmr in self.__breakpoints.keys():
            bkpt = self.__breakpoints.pop(bkpt_nmr)
            self.__bc.delete_breakpoint(bkpt.breakpoint)
