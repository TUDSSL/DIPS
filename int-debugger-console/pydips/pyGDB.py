import sys
from pydips.EnergyBudgetFinder import EnergyBudgetFinder
from pydips.GdbController import GdbController
from pydips.Breakpoints import BreakpointController
from pydips.EnergyGuard import EnergyGuardController
from pydips.EnergyBreakpoints import EnergyBreakpointsController
from pydips.MemCheck import MemCheck
from pydips.PeripheralCheck import PeripheralCheck
from pydips.ForwardProgressCheck import ForwardProgressCheck
from pydips.Profiler import Profiler


GDB_EXECUTABLE = ['arm-none-eabi-gdb', "--nx", '--interpreter=mi2']


class GDBController:
    """
    GDB Controller is the custom-built interface for ARM-NONE-EABI-GDB
    it is the first layer of communication between the PC and Debugger
    """
    __running = False  # set when running
    __connected = False  # set when connected

    __stop_point = []  # update stop breakpoints (kill on next line)

    def __init__(self, args):
        """
        Construct the GDB controller, start thread and run GDB.
        Also run other plugins for this gdb interface
        :param gdb_executable: Custom gdb executable or parameters
        """
        # self.__proc = GdbController(command=GDB_EXECUTABLE + sys.argv[1:])
        self.__proc = GdbController(command=GDB_EXECUTABLE + sys.argv[1:])
        self.__eb_c = EnergyBreakpointsController()
        self.__bc_c = BreakpointController()
        self.__eg_c = EnergyGuardController()
        self.__test_memcheck = MemCheck()
        self.__test_peripheral = PeripheralCheck()
        self.__test_forward_progress = ForwardProgressCheck()
        self.__find_energy_budget = EnergyBudgetFinder()
        self.__profiler = Profiler()
        self.__machine_printing = (args.interpreter == 'mi2')

        # Set callbacks for interactivity
        self.__bc_c.set_callback(self)
        self.__eg_c.set_callbacks(self, self.__bc_c)
        self.__eb_c.set_callbacks(self, self.__bc_c)
        self.__test_memcheck.set_callback(self, self.__bc_c)
        self.__test_peripheral.set_callback(self, self.__bc_c)
        self.__find_energy_budget.set_callback(self, self.__bc_c)
        self.__test_forward_progress.set_callback(self, self.__bc_c)
        self.__profiler.set_callback(self, self.__bc_c)
        
    def done(self):
        if self.__machine_printing:
            print("^done")
            print("(gdb) ")

    def error(self, msg):
        print(f'~"{repr(msg)[1:-1]}"')
        self.done()

    def print_f(self, output, pref=None, skip_on_term=False):
        if self.__machine_printing and not skip_on_term:
            print(output)
        else:
            if not pref:
                print(repr(output)[1:-1])
            else:
                print(repr(f'{pref}"{output}"')[1:-1])

    def interrupt(self):
        """
        Callback ran on interruption (Ctrl+C) from client terminal
        """
        self.__proc.write("-exec-interrupt", raise_error_on_timeout=False)
        self.print(self.__proc.get_gdb_response(raise_error_on_timeout=False))

    def start(self) -> None:
        """
        Start procedure, set target to async for running gdb in parallel to this wrapper
        """
        pass
        self.__proc.write("-gdb-set target-async on")

    def quit(self):
        self.__proc.exit()

    def test_mem_check(self, func_check, func_restore):
        """
        Run memory check automated testing procedure
        :param func_check:
        :param func_restore:
        """
        self.__test_memcheck.setup()
        self.__test_memcheck.set_check_function(func_check)
        self.__test_memcheck.set_restore_function(func_restore)

        self.print_f("Test memory check initialized", '~')
        self.print_f("starting script now", '~')

        self.__test_memcheck.execute(self.__proc)

    def test_forward_progress(self, func_check):
        """
        Run forward progress check automated testing procedure
        :param func_check:
        """
        self.__test_forward_progress.setup()
        self.__test_forward_progress.set_check_function(func_check)

        self.print_f("Test forward progress check initialized", '~')
        self.print_f("starting script now", '~')

        self.__test_forward_progress.execute(self.__proc)
        
    def test_peripheral(self, func_check, func_restore):
        """
        Run peripheral automated testing procedure
        :param func_check:
        :param func_restore:
        :return:
        """
        self.__test_peripheral.setup()
        self.__test_peripheral.set_check_function(func_check)
        self.__test_peripheral.set_restore_function(func_restore)

        self.print_f("Test peripheral check initialized", '~')
        self.print_f("starting script now", '~')

        self.__test_peripheral.execute(self.__proc)

    def find_energy_budget(self, energyemulator, func_check, func_restore):
        """
        Run minimum energy budget automated testing procedure
        :param energy_emulator:
        :param func_check:
        :param func_restore:
        """
        self.__find_energy_budget.setup()
        self.__find_energy_budget.set_check_function(func_check)
        self.__find_energy_budget.set_restore_function(func_restore)

        self.print_f("Find energy budget script initialized", '~')
        self.print_f("starting script now", '~')

        self.__find_energy_budget.execute(self.__proc, energyemulator)
        
    def profiler(self, energyemulator, func_indicate_end, func_possible_reboot):
        """
        Run profiler automated testing procedure
        :param energy_emulator:
        :param func_possible_reboot:
        :param func_indicate_end:
        """
        self.__profiler.setup()
        self.__profiler.set_possible_reboot_function(func_possible_reboot)
        self.__profiler.set_indicate_end_function(func_indicate_end)

        self.print_f("Find energy budget script initialized", '~')
        self.print_f("starting script now", '~')    
        
        self.__profiler.execute(self.__proc, energyemulator)

        
    def cont(self):
        """
        Continue GDB execution, without breaking the wrapper
        :return:
        """
        self.print(self.__proc.write("-exec-continue", raise_error_on_timeout=False))

    def connect(self, port):
        """
        Connect to the GDB server via target extended protocol
        Also continue and interrupt the program, this avoids some weird behaviour
        where the wrapper thinks the device is not running while it is running
        :param port:
        :return:
        """
        # cmds = [,
        #         "mon s",
        #         "attach 1",
        #         ]
        self.print(self.__proc.write(f"tar ext /dev/{port}", raise_error_on_timeout=False), internal_step=True, log=True)
        self.print(self.__proc.write(f"mon auto_scan", raise_error_on_timeout=False), internal_step=True, log=True)
        self.print(self.__proc.write(f"attach 1", raise_error_on_timeout=False), internal_step=True, log=True)
        self.get_response()
        print('^connected')

    def wait_connect(self, port):
        """
        Connect to the GDB server via target extended protocol
        Also continue and interrupt the program, this avoids some weird behaviour
        where the wrapper thinks the device is not running while it is running
        :param port:
        :return:
        """
        self.print(self.__proc.write(f"tar ext /dev/{port}", raise_error_on_timeout=False), internal_step=True, log=True)

        while(True):
            exit = False
            obj = self.__proc.write(f"mon wait_attach", raise_error_on_timeout=False)
            
            for line in obj[0]:
                if "true" in line:
                    #print("attached")
                    exit = True
                    break
            
            if(exit == True):
                break      
        self.print(self.__proc.write(f"mon auto_scan", raise_error_on_timeout=False), internal_step=True, log=True)
        self.print(self.__proc.write(f"attach 1", raise_error_on_timeout=False), internal_step=True, log=True)
        self.get_response()
        print('^connected')
        
    def write(self, cmd, raise_error_on_timeout=False, internal_step=False):
        """
        Write a command to GDB
        :param cmd: command to write
        :param raise_error_on_timeout: set timeout
        :return:
        """
        self.__running = False
        obj = self.__proc.write(cmd, raise_error_on_timeout=raise_error_on_timeout)
        # print(obj)
        # obj = self.__proc.get_gdb_response()

        self.print(obj, internal_step=internal_step)
        return obj

    def get_response(self):
        """
        Wait extra for response
        """
        self.print(self.__proc.get_gdb_response(0.1, False), internal_step=True)

    def is_running(self):
        """
        Check if gdb is in a running state
        :return: running
        """
        return self.__running

    def is_connected(self):
        """
        Check if gdb is in a connected state
        :return connected
        """
        return self.__connected

    def set_energybreakpoint(self, args):
        """
        Set an energy breakpoint on the gdb server
        :param args: [function, threshold]
        :return: None
        """
        if not self.__connected:
            self.error("Device not connected")
            return

        if len(args) != 2:
            self.error("Wrong formatting, use function and threshold as arguments")
            return

        try:
            self.__eb_c.append(args[0], int(args[1]))

            self.print_f("Energy breakpoint set", pref="~")
            self.done()

        except ValueError:
            self.error("Wrong formatting, use function and threshold as arguments")
            return

    def delete_energybreakpoint(self, args):
        """
        Remove an energy breakpoint from the gdb server
        :param args:
        :return:
        """
        if not self.__connected:
            self.error("Device not connected")
            return

        if len(args) != 1:
            self.error("Wrong formatting, use breakpoint number as argument")
            return

        try:
            self.__eb_c.remove(int(args[0]))
            self.done()

        except ValueError:
            self.error("Wrong formatting, use breakpoint number as argument")
            return

    def set_energyguard(self, args):
        """
        Set an energyguard on the gdb server
        :param args:
        :return:
        """
        if not self.__connected:
            self.error("Device not connected")
            return

        if len(args) != 2:
            self.error("Wrong formatting, use function_start function_end as arguments")
            return

        self.__eg_c.append(args[0], args[1])
        self.done()

    def delete_energyguard(self, args):
        """
        Remove an energygurard from the gdb server
        :param args:
        :return:
        """
        if not self.__connected:
            self.error("Device not connected")
            return

        if len(args) != 2:
            self.error("Wrong formatting, use breakpoint numbers as arguments")
            return

        try:
            self.__eg_c.remove(int(args[0]), int(args[1]))
            self.done()

        except ValueError:
            self.error("Wrong formatting, use breakpoint numbers as arguments")
            return

    def add_interrupt_point(self, function_name):
        """
        Add an point where the system should interrupt the power
        :param function_name: name of function of line number to set a breakpoint
        :return:
        """
        nmr = self.__bc_c.set_breakpoint(function_name)
        self.__stop_point.append(nmr.number)
        self.done()

    def on_breakpoint(self, bkpt_nmr):
        """
        Callback ran on breakpoint
        :param bkpt_nmr:
        :return:
        """
        if self.__eg_c.check(bkpt_nmr):
            self.__eg_c.execute(bkpt_nmr)

        if self.__eb_c.check(bkpt_nmr):
            self.__eb_c.execute(bkpt_nmr)

        if bkpt_nmr in self.__stop_point:
            self.write("mon intermit")
            self.cont()

    def print(self, gdb_response, internal_step=False, log=False):
        """
        Handle gdb response
        :param internal_step:
        :param log:
        :param response: Response from GDB
        :return:
        """
        raw, response = gdb_response

        if self.__machine_printing:
            for raw_line in raw:
                if not internal_step:
                    print(raw_line)
                elif raw_line != '^done':
                    print(raw_line)

        for msg in response:
            if msg['message'] == 'running':
                self.__running = True
                self.__connected = True

            if msg['message'] == 'stopped':
                self.__running = False
                self.__connected = True
                if 'bkptno' in msg['payload']:
                    self.on_breakpoint(int(msg['payload']['bkptno']))

            if not self.__machine_printing:
                if msg['type'] == 'target':
                    print(msg['payload'], end='')

                if msg['type'] == 'console':
                    print(msg['payload'], end='')

                if msg['type'] == 'log':
                    print(msg['payload'], end='')
