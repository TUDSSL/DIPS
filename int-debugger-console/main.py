#!/usr/bin/env python3
import re
import serial.tools.list_ports
from prompt_toolkit import PromptSession
from pydips.pyGDB import GDBController
import json
import argparse
from prompt_toolkit.history import FileHistory
from prompt_toolkit.auto_suggest import AutoSuggestFromHistory
from pydips.EnergyEmulator import EnergyEmulator

parser = argparse.ArgumentParser()
parser.add_argument('--version', action='store_true')
parser.add_argument('executable-file', nargs='?', default='')

parser.add_argument('--args')
parser.add_argument('--core')
parser.add_argument('--exec')
parser.add_argument('--pid')
parser.add_argument('--directory')
parser.add_argument('--se')
parser.add_argument('--symbols')
parser.add_argument('--readnow', action='store_true')
parser.add_argument('--readnever', action='store_true')
parser.add_argument('--write', action='store_true')

parser.add_argument('--command', '-x', )
parser.add_argument('--init-command', '-ix', action='store_const', const='FILE')
parser.add_argument('--eval-command', '-ex', action='store_const', const='COMMAND')
parser.add_argument('--init-eval-command', '-iex', action='store_const', const='COMMAND')
parser.add_argument('--nh', action='store_true')
parser.add_argument('--nx', action='store_true')

parser.add_argument('--fullname', action='store_true')
parser.add_argument('--interpreter')
parser.add_argument('--tty', action='store_const', const='TTY')
parser.add_argument('--w', action='store_true')
parser.add_argument('--nw', action='store_true')
parser.add_argument('--tui', action='store_true')
parser.add_argument('--dbx', action='store_true')
parser.add_argument('--quit', '--silent', '-q', action='store_true')

parser.add_argument('--batch', action='store_true')
parser.add_argument('--batch-silent', action='store_true')
parser.add_argument('--return-child-result', action='store_true')
parser.add_argument('--configuration', action='store_true')
parser.add_argument('-b', action='store_const', const='BAUDRATE')
parser.add_argument('-l', action='store_const', const='TIMEOUT')
parser.add_argument('--cd', action='store_const', const='DIR')
parser.add_argument('--data-directory', '-D', action='store_const', const='DIR')

args = parser.parse_args()

if args.version:
    print("GNU gdb (GNU Arm Embedded Intermittent Debugger 1-2022-q3) 1.0.0\n" +
          "Copyright (C) 2022 TU Delft.")
    exit(0)

if args.interpreter == 'mi2':
    session = None

else:
    #    Make sure there is a history file
    history = open('.dipscmdhistory', 'a')
    history.close()

    # Create session using prompt_toolkit
    session = PromptSession(history=FileHistory('.dipscmdhistory'))

# Used for autodetection of the serialport
DIPS_DEVICE_ID = 'Black Magic Probe  v0.0.1-dev-int-debug - Black Magic GDB Server'

# start the GDB Controller
gdb_c = GDBController(args)

# define emulator object
energy_emulator = EnergyEmulator()


def close_terminal():
    """
    Close terminal and kill all running processes
    :return:
    """
    exit(0)


def print_dips_help() -> None:
    """
    Print the dips help menu
    :return:
    """
    print_f("List of custom DIPS commands:\n", '~')
    print()
    print_f("break_mode -- Set supply to pause mode\n", '~')
    print_f("connect -- automatically detect the DIPS\n", '~')
    print_f("dips_gui -- Open the GUI of the power emulator\n", '~')
    print_f("dips_help -- Displays this help information in the cli\n", '~')
    print_f("dips_mem_check -- Start intermittency autotesting procedure for checkpoint correctness\n", '~')
    print_f("dips_peripheral_check -- Start intermittency autotesting procedure for peripheral correctness\n", '~')
    print_f("dips_forward_progress -- Start intermittency autotesting procedure for forward progress\n", '~')
    print_f("dips_energy_budget_finder -- Start intermittency autotesting procedure for finding the energy budget\n", '~')
    print_f("dips_profiler -- Start intermittency autotesting procedure for profiling\n", '~')
    print_f("energy_breakpoint -- Set an energy breakpoint on a line\n", '~')
    print_f("energy_guard -- Set an energysafe zone beteen two lines\n", '~')
    print_f("intermittent_count -- Return number of intermittences\n", '~')
    print_f("intermittent_mode -- Set debugger to intermittent or regular use\n", '~')
    gdb_c.done()


def argument_parser(cmd) -> None:
    """
    Format the command and run program accordingly
    :param cmd: command
    :return:
    """
    funct = cmd.split(" ")[0]
    #     # if funct[0].isdigit():
    #     #     funct = funct[1:]
    if funct == "dips_gui":
        pass
        # print("Not implemented, open DIPS GUI Manually")
    elif funct == "dips_help":
        print_dips_help()
    elif funct == "dips_mem_check":
        dips_memory_check(cmd.split(" ")[1:])
    elif funct == "dips_peripheral_check":
        dips_peripheral_check(cmd.split(" ")[1:])
    elif funct == "dips_forward_progress":
        dips_forward_progress_check(cmd.split(" ")[1:])
    elif funct == "dips_energy_budget_finder":
        dips_energy_budget_finder(energy_emulator, cmd.split(" ")[1:])
    elif funct == "dips_profiler":
        dips_profiler(energy_emulator, cmd.split(" ")[1:])
    elif funct == "intermittent_count":
        intermittent_count()
    elif funct == "intermittent_mode":
        intermittent_mode(cmd.split(" ")[1:])
    elif funct == "connect" or funct == "-target-select remote":
        dips_connect(cmd.split(" "))
    elif funct == "connect-emulator":
        energy_emulator.auto_connect()
    elif funct == "clear" or funct == "cls":
        clear_screen()
    elif funct == "energy_guard" or funct == "eg":
        gdb_c.set_energyguard(cmd.split(" ")[1:])
    elif funct == "1-gdb-version2-environment-cd":
        return
    elif funct == "delete_energy_guard" or funct == "d_eg":
        gdb_c.delete_energyguard(cmd.split(" ")[1:])
    elif funct == "energy_breakpoint" or funct == "eb" or funct == "energy_break":
        gdb_c.set_energybreakpoint(cmd.split(" ")[1:])
    elif funct == "delete_energy_breakpoint" or funct == "d_eb" or funct == "d_energy_break":
        gdb_c.delete_energybreakpoint(cmd.split(" ")[1:])
    elif funct == "quit":
        gdb_c.quit()
        close_terminal()
    elif funct == "dips_interrupt":
        gdb_c.add_interrupt_point(cmd.split(" ")[1])
    elif funct == "c" or funct[:4] == "cont":
        gdb_c.cont()
    elif funct[:2] == "17":
        dips_connect(["dips_connect"])
        return
    elif funct == "-target-select":
        dips_connect(["dips_connect"])
    # elif funct == '-interpreter-exec' and cmd.split(" ")[5] == 'console':
    #     return argument_parser(re.findall('"([^"]*)"', cmd)[0])
    elif funct == "wait_connect":
        dips_wait_connect(["dips_connect"])
    else:
        gdb_c.write(cmd)


def dips_memory_check(args):
    """
    Start dips memory check automated testing procedure
    :param args:
    :return:
    """
    if not gdb_c.is_connected():
        print_f("No device connected\n", '~')
        return

    print_f("Starting memory automated testing procedure....\n", '~')
    if len(args) != 2:
        print_f("Using checkpoint and restore functions from default configuration\n", '~')
        print_f("Add checkpoint and restore as arguments to change this\n", '~')
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            gdb_c.test_mem_check(
                data['memcheck']['checkpoint_function'],
                data['memcheck']['restore_function']
            )
    else:
        gdb_c.test_mem_check(args[0], args[1])

def dips_forward_progress_check(args):
    """
    Start dips forward progress check automated testing procedure
    :param args:
    :return:
    """
    if not gdb_c.is_connected():
        print_f("No device connected\n", '~')
        return

    print_f("Starting memory automated testing procedure....\n", '~')
    if len(args) != 1:
        print_f("Using checkpoint from default configuration\n", '~')
        print_f("Add checkpoint as arguments to change this\n", '~')
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            gdb_c.test_forward_progress(
                data['memcheck']['checkpoint_function'],
            )
    else:
        gdb_c.test_forward_progress(args[0])

def dips_peripheral_check(args):
    """
    Start dips peripheral check automated testing procedure
    :param args:
    :return:
    """
    if not gdb_c.is_connected():
        print_f("No device connected\n", '~')
        return

    print_f("Starting peripheral automated testing procedure....\n", '~')
    if len(args) != 2:
        print_f("Using checkpoint and restore functions from default configuration\n", '~')
        print_f("Add checkpoint and restore as arguments to change this\n", '~')
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            gdb_c.test_peripheral(
                data['peripheral']['checkpoint_function'],
                data['peripheral']['restore_function']
            )
    else:
        gdb_c.test_peripheral(args[0], args[1])


def dips_energy_budget_finder(energy_emulator, args):
    """
    Start energy budget finder automated testing procedure
    :param args:
    :return:
    """
    if not gdb_c.is_connected():
        print_f("No device connected\n", '~')
        return

    print_f("Starting forward progress automated testing procedure....\n", '~')
    if len(args) != 2:
        print_f("Using checkpoint and restore functions from default configuration\n", '~')
        print_f("Add checkpoint and restore as arguments to change this\n", '~')
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            gdb_c.find_energy_budget(
                energy_emulator,
                data['forwardprogress']['checkpoint_function'],
                data['forwardprogress']['restore_function'],
            )
    else:
        gdb_c.find_energy_budget(energy_emulator, args[0], args[1])

def dips_profiler(energy_emulator, args):
    """
    Start energy budget finder automated testing procedure
    :param args:
    :return:
    """
    if not gdb_c.is_connected():
        print_f("No device connected\n", '~')
        return

    print_f("Starting forward progress automated testing procedure....\n", '~')
    if len(args) != 2:
        print_f("Using checkpoint and restore functions from default configuration\n", '~')
        print_f("Add checkpoint and restore as arguments to change this\n", '~')
        with open('dips_testing.json') as json_file:
            data = json.load(json_file)
            gdb_c.profiler(
                energy_emulator,
                data['profiler']['indicate_end_function'],
                data['profiler']['possible_reboot_function'],
            )
    else:
        gdb_c.profiler(energy_emulator, args[0], args[1])

def intermittent_count():
    """
    Return number of intermittences
    :return: amount of intermittences
    """
    gdb_c.write('mon int_count')


def intermittent_mode(args):
    """
    Enable intermittent mode on the device
    :param args:
    :return:
    """
    if len(args) > 0:
        if args[0] == "true":
            gdb_c.write('mon int enable', raise_error_on_timeout=False)
        elif args[0] == "false":
            gdb_c.write('mon int disable', raise_error_on_timeout=False)
        else:
            print_f("Unknown command\n", '~')
    else:
        gdb_c.write('mon int', raise_error_on_timeout=False)


def dips_connect(args: []) -> None:
    """
    Autoconnect GDB using the known device name
    :param args:
    :return:
    """
    if len(args) == 1:
        port = return_serialport()
    else:
        port = args[1]
    if port is None:
        print_f("No device found with autoconnect", pref='^error,msg=')
        return

    print_f(f"found port: {port}", '~')
    gdb_c.connect(port)

def dips_wait_connect(args: []) -> None:
    
    if len(args) == 1:
        port = return_serialport()
    else:
        port = args[1]
    if port is None:
        print_f("No device found with autoconnect", pref='^error,msg=')
        return

    print_f(f"found port: {port}, only connects when DUT triggers UART RX pin", '~')
    
    gdb_c.wait_connect(port)


def return_serialport():
    """
    Get Blackmagic serialport
    :return: Serialport | None
    """
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if p.description == DIPS_DEVICE_ID:
            return p.name

    return None


def clear_screen():
    """
    Clear the terminal
    """
    if not args.interpreter == 'mi2':
        session.output.erase_screen()


def print_f(output, pref=None, skip_on_term=False):
    if args.interpreter != 'mi2' and not skip_on_term:
        print(output)
    else:
        if not pref:
            print(repr(output)[1:-1])
        else:
            print(repr(f'{pref}"{output}"')[1:-1])


def main():
    """
    Start gdb, and terminal session, handle interrupts
    """
    gdb_c.start()
    # await _read_stdin_pipe()
    gdb_c.write('-gdb-set mi-async on', internal_step=True)
    # print(repr(f'~"dips program settings\n"'))

    gdb_c.get_response()

    while True:
        try:
            if args.interpreter == 'mi2':
                cmd = input()
                argument_parser(cmd)
                print("~" + cmd)

            else:
                cmd = session.prompt("(gdb) ",
                                     in_thread=True,
                                     auto_suggest=AutoSuggestFromHistory())

                argument_parser(cmd)
                while gdb_c.is_running():

                    gdb_c.get_response()
                    if not gdb_c.is_running():
                        break

        except KeyboardInterrupt:
            gdb_c.interrupt()
            return

        except BrokenPipeError:
            close_terminal()

        except EOFError as e:
            print(end="")


main()
