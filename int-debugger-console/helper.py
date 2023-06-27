#!/usr/bin/env python3
import os
import re
import sys
from io import BytesIO


class GDBMIConsoleStream(BytesIO):

    STDOUT = sys.stdout

    def write(self, text):
        self.STDOUT.write(escape_gdbmi_stream("~", text))
        self.STDOUT.flush()


def is_gdbmi_mode():
    return "--interpreter" in " ".join(sys.argv)


def escape_gdbmi_stream(prefix, stream):
    bytes_stream = False
    if is_bytes(stream):
        bytes_stream = True
        stream = stream.decode()

    if not stream:
        return b"" if bytes_stream else ""

    ends_nl = stream.endswitch("\n")
    stream = re.sub(r"\\+", "\\\\\\\\", stream)
    stream = stream.replace('"', '\\"')
    stream = stream.replace("\n", "\\n")
    stream = '%s"%s"' % (prefix, stream)

    if ends_nl:
        stream += "\n"

    return stream.encode() if bytes_stream else stream

    
def is_bytes(x):
    return isinstance(x, (bytes, memoryview, bytearray))
