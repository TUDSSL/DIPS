import time
import json
from datetime import datetime
from pydips.memdiff import check_mem
import os

DUMP_FOLDER = 'dumps/'
STACK_SIZE = 32
MEM_DUMP_BASENAME = '_memory_dump_'

memory_dump_ranges = {}
checkpoints = []


def build_bin_filename(prefix, memname):
    fname = DUMP_FOLDER + prefix + MEM_DUMP_BASENAME + memname + '.bin'
    return fname


def touch_file(filename):
    f = open(filename, "w")
    f.close()


class MemCheck:
    __gdb_c = None
    __breakpoint_c = None
    __check_functionBreakpoint = None
    __restore_functionBreakpoint = None
    __only_checkpointing = False
    timeout = 60

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
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            self.__only_checkpointing = data['memcheck']['only_checkpointing']
            ranges = data['memcheck']['regions']
            self.timeout = data['memcheck']['timeout']
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

    def __exit_memcheck(self):
        self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
        self.__breakpoint_c.delete_breakpoint(self.__restore_functionBreakpoint)
        self.__gdb_c.write("mon interrupt_on_power_failure disable")

    def execute(self, proc):
        out = proc.write('-exec-continue', raise_error_on_timeout=False)
        running = True
        checkpoint_triggered = 0
        restorepoint_triggered = 0
        last_checkpoint = None

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

                                elif output['payload']['reason'] == 'breakpoint-hit':

                                    if output['payload']['bkptno'] == str(self.__check_functionBreakpoint.number):
                                        checkpoint_triggered += 1
                                        print(f"{timestamp} checkpoint triggered {checkpoint_triggered}\n")
                                        last_checkpoint = time.monotonic()
                                        if memory_dump_ranges:
                                            title = 'after-cp' + str(len(checkpoints))
                                            self.__dump_memory(title, proc)
                                            checkpoints.append(title)

                                    elif output['payload']['bkptno'] == str(self.__restore_functionBreakpoint.number):
                                        restorepoint_triggered += 1
                                        print(f"{timestamp} restore triggered {restorepoint_triggered}\n")
                                        if not self.__only_checkpointing:
                                            last_checkpoint = time.monotonic()

                                        if memory_dump_ranges and len(checkpoints)!= 0:
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

                        if output['message'] == 'running':
                            running = True

                if last_checkpoint is not None:
                    timeout = time.monotonic() - last_checkpoint
                    if timeout > self.timeout:
                        if running:
                            print(proc.write("-exec-interrupt", raise_error_on_timeout=False))
                        print(f"{timestamp} Errors found in application, no checkpoint detected in the last "
                                f"{self.timeout} seconds.\n")
                        self.__exit_memcheck()

                        return

                if not running:
                    out = proc.write('-exec-continue', raise_error_on_timeout=False)
                else:
                    out = proc.get_gdb_response(raise_error_on_timeout=False)

            except KeyboardInterrupt:
                self.__exit_memcheck()

                print(f"{timestamp} Manually exited memcheck procedure\n")
                return

            except Exception as e: # continue on all exceptions
                print(f"{e}\n")
                continue
