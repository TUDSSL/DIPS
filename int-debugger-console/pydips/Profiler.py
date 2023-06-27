import time
import json
from datetime import datetime
from pydips.memdiff import check_mem
from pydips.EnergyEmulator import EnergyEmulator
import os
import re
import time

DUMP_FOLDER = 'dumps/'
STACK_SIZE = 32
MEM_DUMP_BASENAME = '_forward_progress_mem_dump_'

memory_dump_ranges = {}
checkpoints = []

profile_times = []


def build_bin_filename(prefix, memname):
    fname = DUMP_FOLDER + prefix + MEM_DUMP_BASENAME + memname + '.bin'
    return fname


def touch_file(filename):
    f = open(filename, "w")
    f.close()


class Profiler:
    __gdb_c = None
    __breakpoint_c = None
    __possibleReboot_functionBreakpoint = None
    __indicateEnd_functionBreakpoint = None
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

    def set_possible_reboot_function(self, check_function):
        if self.__possibleReboot_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__possibleReboot_functionBreakpoint)
        self.__possibleReboot_functionBreakpoint = self.__breakpoint_c.set_breakpoint(check_function)

    def set_indicate_end_function(self, restore_function):
        if self.__indicateEnd_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__indicateEnd_functionBreakpoint)
        self.__indicateEnd_functionBreakpoint = self.__breakpoint_c.set_breakpoint(restore_function)

    def __exit_forwardprogress(self):
        self.__breakpoint_c.delete_breakpoint(self.__possibleReboot_functionBreakpoint)
        self.__breakpoint_c.delete_breakpoint(self.__indicateEnd_functionBreakpoint)
        self.__gdb_c.write("mon interrupt_on_power_failure disable")
        self.__gdb_c.write("mon profile_mode disable")

    def show_result_and_exit(self, energyEmulator: EnergyEmulator):
        print(f"During test duration, always forward progress and no mem difference")
        energyEmulator.displayEnergySettings()
        self.__exit_forwardprogress()
        
    def handlePossibleReboot(self, energyemulator: EnergyEmulator, proc):
        self.__rebootPointPassed += 1
        print(f"Possible reboot detected: " + str(self.__rebootPointPassed))
        
        if(self.__rebootPointPassed == 2):
            numberRegEx = re.compile(r"\d+")

            output = proc.write('mon profile_time result', raise_error_on_timeout=False)
            output_list =  output[0]
            biggest_time = numberRegEx.findall(output_list[2])[0]
            smallest_time = numberRegEx.findall(output_list[3])[0]
            cycles = numberRegEx.findall(output_list[4])[0]
            if(cycles != '1'):
                print("something weird is happening")
            profile_time_ouput = proc.write('mon profile_time', raise_error_on_timeout=False)
            profile_time_ouput_time =  numberRegEx.findall(output[0][2])[0]
            profile_times.append(profile_time_ouput_time)
            # Restart device
            energyemulator.initConstantVoltage(0)
            self.__powerOff = True
            self.__rebootPointPassed  = 0
            self.__gdb_c.write("mon profile_time reset now")
            

        
    def handleIndicateEnd(self):
        print(f"Indicate end detected")
    
    def handleReboot(self):
        print(f"Reboot detected")

    def execute(self, proc, energyemulator: EnergyEmulator):
        energyemulator.initConstantVoltage(3000)
        self.__gdb_c.write("mon profile_time a a")
        out = proc.write('-exec-continue', raise_error_on_timeout=False)
        energyemulator.resendEnergy()
        self.__rebootPointPassed = 0
        self.__powerOff = False
        running = True

        while True:
            try:
                timestamp = str(datetime.now())
                for output in out[1]:
                    if output['type'] == 'notify':
                        if output['message'] == 'stopped':
                            running = False
                            if 'reason' in output['payload']:

                                if output['payload']['reason'] == 'signal-received':
                                   self.handleReboot()
                                   print( output['payload']['reason'])
                                
                                elif output['payload']['reason'] == 'breakpoint-hit':

                                    if output['payload']['bkptno'] == str(self.__possibleReboot_functionBreakpoint.number):
                                       self.handlePossibleReboot(energyemulator, proc)
                                    if output['payload']['bkptno'] == str(self.__indicateEnd_functionBreakpoint.number):
                                       self.handleIndicateEnd()
                                            
                                    

                        if output['message'] == 'running':
                            running = True

                if not running:
                    out = proc.write('-exec-continue', raise_error_on_timeout=False)
                    energyemulator.resendEnergy()
                else:
                    out = proc.get_gdb_response(raise_error_on_timeout=False)
                    
                if(self.__powerOff):
                    time.sleep(.5)
                    energyemulator.initConstantVoltage(3000)

            except KeyboardInterrupt:
                self.__exit_forwardprogress()

                print(f"{timestamp} Manually exited memcheck procedure\n")
                return

            # except Exception as e: # continue on all exceptions
            #     print(f"{e}\n")
            #     continue
