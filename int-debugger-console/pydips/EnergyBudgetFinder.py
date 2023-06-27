import time
import json
from datetime import datetime
from pydips.memdiff import check_mem
from pydips.EnergyEmulator import EnergyEmulator
import os

DUMP_FOLDER = 'dumps/'
STACK_SIZE = 32
MEM_DUMP_BASENAME = '_forward_progress_mem_dump_'

memory_dump_ranges = {}
checkpoints = []


def build_bin_filename(prefix, memname):
    fname = DUMP_FOLDER + prefix + MEM_DUMP_BASENAME + memname + '.bin'
    return fname


def touch_file(filename):
    f = open(filename, "w")
    f.close()


class EnergyBudgetFinder:
    __gdb_c = None
    __breakpoint_c = None
    __check_functionBreakpoint = None
    __restore_functionBreakpoint = None
    timeout = 60
    maximum_task_duration = 10
    minimum_test_duration = 300
    
    def set_callback(self, gdb_callback, bc_callback):
        self.__gdb_c = gdb_callback
        self.__breakpoint_c = bc_callback

    def __dump_memory(self, prefix, proc):
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
            output = proc.write(cmd_str)

    def setup(self):
        self.__gdb_c.write("set pagination off")
        self.__gdb_c.write("set print pretty")
        self.__gdb_c.write("mon interrupt_on_power_failure enable")
        self.__gdb_c.write("mon profile_mode enable")

        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            ranges = data['forwardprogress']['regions']
            
            self.timeout = data['forwardprogress']['timeout']
            self.minimum_test_duration = data['forwardprogress']['minimum_test_duration']
            self.maximum_task_duration = data['forwardprogress']['maximum_task_duration']

            for mem_name in ranges:
                memory_dump_ranges[mem_name] = [int(ranges[mem_name][0], base=16), int(ranges[mem_name][1], base=16)]
        if not os.path.exists(DUMP_FOLDER):
            os.makedirs(DUMP_FOLDER)

    def set_check_function(self, check_function):
        if self.__check_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
        self.__check_functionBreakpoint = self.__breakpoint_c.set_breakpoint(check_function)

    def set_restore_function(self, restore_function):
        if self.__restore_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__restore_functionBreakpoint)
        self.__restore_functionBreakpoint = self.__breakpoint_c.set_breakpoint(restore_function)

    def __exit_forwardprogress(self):
        self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
        self.__breakpoint_c.delete_breakpoint(self.__restore_functionBreakpoint)
        self.__gdb_c.write("mon interrupt_on_power_failure disable")
        self.__gdb_c.write("mon profile_mode disable")

    def show_result_and_exit(self, energyEmulator: EnergyEmulator):
        print(f"During test duration, always forward progress and no mem difference")
        energyEmulator.displayEnergySettings()
        self.__exit_forwardprogress()

    def execute(self, proc, energyemulator: EnergyEmulator):
        energyemulator.constantVoltage(0)
        energyemulator.initSquareWave(2000, .3, 5000)
        cap_uf = 10.0
        cap_mv= 3.0
        high_threshold_mv= 3.001
        low_threshold_mv= 2.001
        cap_out_mv=3.2
        cap_max_mv=3.300
        cap_input_max_ma=0
        outputting= False
        # energyemulator.initVirtCap(cap_uf, cap_mv, high_threshold_mv, low_threshold_mv, cap_out_mv, cap_max_mv, cap_input_max_ma, outputting)
        out = proc.write('-exec-continue', raise_error_on_timeout=False)
        energyemulator.resendEnergy()
        running = True
        checkpoint_triggered = 0
        restorepoint_triggered = 0
        last_checkpoint = time.monotonic()
        time_last_timeout = 0 
        


        while True:
            try:
                timestamp = str(datetime.now())
                for output in out[1]:
                    if output['type'] == 'notify':
                        if output['message'] == 'stopped':
                            running = False
                            if 'reason' in output['payload']:

                                if output['payload']['reason'] == 'signal-received':
                                    print(f"{timestamp} restarted\n")
                                    if(output['payload']['frame']['func']!="iclib_boot"):
                                        print("SIGINT "+ output['payload']['frame']['func'])

                                elif output['payload']['reason'] == 'breakpoint-hit':

                                    if output['payload']['bkptno'] == str(self.__check_functionBreakpoint.number):
                                        checkpoint_triggered += 1
                                        print(f"{timestamp} checkpoint triggered {checkpoint_triggered}")
                                        last_checkpoint = time.monotonic()
                                        if memory_dump_ranges:
                                            title = 'after-cp' + str(len(checkpoints))
                                            self.__dump_memory(title, proc)
                                            checkpoints.append(title)
                                        
                                        timeout = time.monotonic() - time_last_timeout
                                        if (timeout > self.minimum_test_duration )and (time_last_timeout != 0):
                                            print(f"{timestamp} forward progress detected for {self.minimum_test_duration} seconds\n")
                                            self.show_result_and_exit(energyemulator)
                                            print("return")
                                            return
                                            
                                    elif output['payload']['bkptno'] == str(self.__restore_functionBreakpoint.number):
                                        restorepoint_triggered += 1
                                        print(f"{timestamp} restore triggered {restorepoint_triggered}")

                                        if memory_dump_ranges and len(checkpoints)!= 0 and False:
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
                                                print("Errors found in application! t")
                                                # self.__exit_forwardprogress()
                                                # return

                        if output['message'] == 'running':
                            running = True

                time_since_last_checkpoint = time.monotonic() - last_checkpoint
                if time_since_last_checkpoint > self.maximum_task_duration:
                    # Took to long to reach checkpoint
                    if (time.monotonic() - time_last_timeout) > self.maximum_task_duration:
                        # Long enough since last timeout
                        time_last_timeout = time.monotonic()
                        print(f"{timestamp} No forward progress detected, increase dutycycle")
                        
                        # No forward progress detected, increase dutycycle
                        energyemulator.increaseEnergy()
                        

                if not running:
                    out = proc.write('-exec-continue', raise_error_on_timeout=False)
                    # energyemulator.resendEnergy()
                    # print("resend energy")
                else:
                    out = proc.get_gdb_response(raise_error_on_timeout=False)

            except KeyboardInterrupt:
                self.__exit_forwardprogress()

                print(f"{timestamp} Manually exited memcheck procedure\n")
                return

            except: # continue on all exceptions
                continue
