import time
import json
from datetime import datetime
from pydips.memdiff import check_mem
import os

checkpoints = []

class ForwardProgressCheck:
    __gdb_c = None
    __breakpoint_c = None
    __check_functionBreakpoint = None
    timeout = 60

    def set_callback(self, gdb_callback, bc_callback):
        self.__gdb_c = gdb_callback
        self.__breakpoint_c = bc_callback


    def setup(self):
        self.__gdb_c.write("set pagination off")
        self.__gdb_c.write("set print pretty")
        self.__gdb_c.write("mon interrupt_on_power_failure enable")
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            self.__only_checkpointing = data['memcheck']['only_checkpointing']
            self.timeout = data['memcheck']['timeout']

    def set_check_function(self, check_function):
        if self.__check_functionBreakpoint:
            self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
        self.__check_functionBreakpoint = self.__breakpoint_c.set_breakpoint(check_function)

    def __exit_memcheck(self):
        self.__breakpoint_c.delete_breakpoint(self.__check_functionBreakpoint)
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
                                       
                        if output['message'] == 'running':
                            running = True

                if last_checkpoint is not None:
                    timeout = time.monotonic() - last_checkpoint
                    if timeout > self.timeout:
                        if running:
                            print(proc.write("-exec-interrupt", raise_error_on_timeout=False))
                        print(f"{timestamp} Errors found in application, restore function not finished after "
                                f"{self.timeout} sec\n")
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
