import time
import json
from datetime import datetime
from cmsis_svd.parser import SVDParser
from pydips.memdiff import check_mem

DUMP_FOLDER = 'dumps/'
STACK_SIZE = 32
MEM_DUMP_BASENAME = '_peripheral_dump_'

memory_dump_ranges = {

}
checkpoints = []


def build_bin_filename(prefix, memname):
    """
    Return filename of combination of memory part and prefix
    :param prefix:
    :param memname:
    :return:
    """
    fname = DUMP_FOLDER + prefix + MEM_DUMP_BASENAME + memname + '.bin'
    return fname


def touch_file(filename):
    """
    Create file before editing
    :param filename:
    :return:
    """
    f = open(filename, "w")
    f.close()


class PeripheralCheck:
    """
    Class consisting the peripheral automated checking procedure
    """

    __gdb_c = None
    __svd = None
    __breakpoint_c = None
    __check_functionBreakpoint = None
    __restore_functionBreakpoint = None

    def set_callback(self, gdb_callback, bc_callback):
        """
        Set callbacks for gdb and breakpoint controllers
        :param gdb_callback:
        :param bc_callback:
        :return:
        """
        self.__gdb_c = gdb_callback
        self.__breakpoint_c = bc_callback

    def setup(self):
        """
        Setup and initialize the peripheral check
        :return:
        """
        self.__gdb_c.write("set pagination off")
        self.__gdb_c.write("set print pretty")
        self.__gdb_c.write("mon interrupt_on_power_failure enable")
        with open('dips_testing.json') as json_file:
            try:
                data = json.load(json_file)
                chip = data['peripheral']['chip']
                self.__svd = SVDParser.for_xml_file(chip)
                print("SVD file loaded!")
                cfgRegisters = data['peripheral']['config_registers']

                for peripheral in self.__svd.get_device().peripherals:
                    for register in peripheral.registers:
                        title = peripheral.name + '_' + register.name
                        if title in cfgRegisters:
                            address = peripheral.base_address + register.address_offset
                            memory_dump_ranges[title] = [address, address + register.size - 1]
                print("Peripheral regions set!")
                print(memory_dump_ranges)

            except FileNotFoundError:
                print("Error reading svd file (not found)")

    def set_check_function(self, check_function):
        """
        Set function where peripheral check should be executed
        :param check_function:
        :return:
        """
        if self.__check_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
        self.__check_functionBreakpoint = self.__breakpoint_c.set_breakpoint(check_function)

    def set_restore_function(self, restore_function):
        """
        Set function where peripheral restore should be executed
        :param restore_function:
        :return:
        """
        if self.__restore_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__restore_functionBreakpoint)
        self.__restore_functionBreakpoint = self.__breakpoint_c.set_breakpoint(restore_function)

    def __dump_memory(self, prefix, proc):
        """
        Dump memory to memname
        :param prefix:
        :return:
        """
        # save this to $1
        # sp = int(str(gdb.newest_frame())[9::].split(',')[0], 16)
        # memory_dump_ranges['stack'] = [sp, sp + STACK_SIZE]
        for memname in memory_dump_ranges:
            title = build_bin_filename(prefix, memname)
            touch_file(title)
            start_addr = str(memory_dump_ranges[memname][0])
            end_addr = str(memory_dump_ranges[memname][1])
            cmd_str = 'dump binary memory ' \
                       + title \
                       + ' ' + start_addr + ' ' + end_addr
            proc.write(cmd_str)

    def __exit(self):
        self.__breakpoint_c.delete_breakpoint(self.__restore_functionBreakpoint)
        self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
        self.__gdb_c.write("mon interrupt_on_power_failure disable")

    def execute(self, proc):
        out = proc.write('-exec-continue', raise_error_on_timeout=False)
        running = True
        checkpoint_triggered = 0
        restorepoint_triggered = 0
        
        while True:
            try:
                timestamp = str(datetime.now())
                for output in out[1]:
                    if output['type'] == 'notify':
                        if output['message'] == 'stopped':
                            #print(output)
                            running = False

                            if output['payload']['reason'] == 'breakpoint-hit':
                                if output['payload']['bkptno'] == str(self.__check_functionBreakpoint.number):
                                    checkpoint_triggered += 1
                                    print(f"{timestamp} checkpoint triggered {checkpoint_triggered}\n")
                                    title = 'after-cp' + str(len(checkpoints))
                                    checkpoints.append(title)
                                    self.__dump_memory(title, proc)

                                if output['payload']['bkptno'] == str(self.__restore_functionBreakpoint.number):
                                    restorepoint_triggered += 1
                                    print(f"{timestamp} restore triggered {restorepoint_triggered}\n")
                                    if len(checkpoints) > 0:
                                        self.__dump_memory('after-rp', proc)
                                        latest = checkpoints[len(checkpoints) - 1]
                                        result = []
                                        for memname in memory_dump_ranges:
                                            filename_rp = build_bin_filename('after-rp', memname)
                                            filename = build_bin_filename(latest, memname)
                                            s_result = check_mem(filename, filename_rp, memory_dump_ranges[memname][0])
                                            if s_result is not None:
                                                result += s_result

                                        if len(result):
                                            print("Errors found in application!")
                                            self.__exit_memcheck()
                                            return

                            restore_name = self.__restore_functionBreakpoint.function
                            check_name = self.__check_functionBreakpoint.function

                            # reinit breakpoints, sometimes the apollo3 forgets its breakpoints T_T
                            proc.write(f'b ${restore_name}', read_response=False)
                            proc.write(f'b ${check_name}', read_response=False)

                        if output['message'] == 'running':
                            running = True

                if not running:
                    out = proc.write('-exec-continue', raise_error_on_timeout=False)
                else:
                    out = proc.get_gdb_response(raise_error_on_timeout=False)

            except KeyboardInterrupt:
                self.__exit()
                print(f"{timestamp} Manually exited peripheralcheck procedure\n")
                return

    # def execute(self, breakpoint_number):
    #     """
    #     Execute the Peripheral check procedure
    #     :param breakpoint_number:
    #     :return:
    #     """
    #     if breakpoint_number == self.__check_functionBreakpoint.number:
    #         # on checkpoint
    #         title = 'after-cp' + str(len(checkpoints))
    #         checkpoints.append(title)
    #         self.__dump_memory(title)
    #
    #     elif breakpoint_number == self.__restore_functionBreakpoint.number:
    #         if len(checkpoints) > 0:
    #             self.__dump_memory('after-rp')
    #             latest = checkpoints[len(checkpoints) - 1]
    #             result = []
    #             for memname in memory_dump_ranges:
    #                 filename_rp = build_bin_filename('after-rp', memname)
    #                 filename = build_bin_filename(latest, memname)
    #                 s_result = check_mem(filename, filename_rp, memory_dump_ranges[memname][0])
    #                 if s_result is not None:
    #                     result += s_result
    #
    #             if len(result):
    #                 print("Memory inconsistency found!")
    #                 for res in result:
    #                     print("")
    #                 return
    #
    #     while not self.__gdb_c.is_running():
    #         try:
    #             self.__gdb_c.cont()
    #         except KeyboardInterrupt:
    #             print("----WARN---- Interrupting during auto testing could influence results")
    #             break
