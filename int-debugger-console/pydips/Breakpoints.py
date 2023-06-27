class Breakpoint(object):
    """
    Breakpoint object consisting of number and function (or line number)
    """
    number = None
    function = None
    pc = None

    def __init__(self, m_number, m_function):
        self.number = m_number
        self.function = m_function


class BreakpointController:
    """
    Breakpoint controller, taking care of setting,
    reading and executing breakpoints on the gdb application
    """

    __breakpoints = []  # list of set breakpoints

    def __init__(self):
        """
        Construct controller
        """
        self.__gdb = None

    def set_callback(self, callback_gdb):
        """
        Set the callbacks to the gdb controller
        :param callback_gdb:
        :return:
        """
        self.__gdb = callback_gdb

    def set_breakpoint(self, function_name):
        """
        Adding a breakpoint to gdb
        :param function_name:
        :return:
        """
        gdb_f = self.__gdb.write(f"b {function_name}", raise_error_on_timeout=False)
        print(gdb_f)
        for msg in gdb_f[1]:
            if msg['type'] == 'notify':
                if msg['message'] == 'breakpoint-created':
                    bkpt = Breakpoint(int(msg['payload']['bkpt']['number']), function_name)
                    bkpt.pc = msg['payload']['bkpt']['addr']
                    self.__breakpoints.append(bkpt)
                    return bkpt

        return None

    def delete_breakpoint(self, bpt):
        """
        Removing a breakpoint from gdb
        :param bpt:
        :return:
        """
        gdb_f = self.__gdb.write(f"d {bpt.number}", raise_error_on_timeout=False)
        for msg in gdb_f[1]:
            if msg['type'] == 'notify':
                if msg['message'] == 'breakpoint-created':
                    self.__breakpoints.remove(bpt)
                    return True

        return False



