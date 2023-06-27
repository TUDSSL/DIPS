import serial
import serial.tools.list_ports
import subprocess
import logging
import time
import threading, multiprocessing
from google.protobuf.internal.decoder import _DecodeVarint32

syntax = "proto2"

import debugger_interface_pb2 as Debugger_interface
import calibration_pb2 as Calibration_interface
import supply_pb2 as Supply_interface

import io
import delimited_protobuf

logger = logging.getLogger(__name__)
DEFAULT_TIMEOUT_SEC = 10

EMULATOR_DEVICE_ID = 'Intermittent Debugger Emulator'

class ConstantVoltage:
    voltage_mv = 0

    def __init__(self, voltage_mv):
        self.voltage_mv = voltage_mv
    
    def increase_energy(self):
        print("not implemented")
    
    def decrease_energy(self):
        print("not implemented")
    
    def displayEnergySettings(self):
        print("Voltage: " + str(self.voltage_mv) + "mV")
        
class SquareWave:
    voltage_mv = 0
    dutycycle = 0.0
    period_ms = 0
    
    def __init__(self, voltage_mv, dutycycle, period_ms):
        self.voltage_mv = voltage_mv
        self.dutycycle = dutycycle
        self.period_ms = period_ms
        
    def increase_energy(self):
        self.dutycycle += .1
        print("On time: " + str(self.dutycycle * self.period_ms /100) + "ms (DC: "+str(self.dutycycle) +")" )
        
    def decrease_energy(self):
        self.dutycycle -= .1
        print("On time: " + str(self.dutycycle * self.period_ms / 1000 /100) + "s")
        
    def displayEnergySettings(self):
        print("Voltage: " + str(self.voltage_mv) + "mV", end='')
        print("Duty Cycle: " + str(self.dutycycle) + "%", end='')
        print("Period: " + str(self.period_ms) + "ms", end='')
            
class VirtCap:
    cap_uf = 0
    cap_mv = 0
    high_threshold_mv = 0
    low_threshold_mv = 0
    cap_out_mv = 0
    cap_max_mv = 0
    cap_input_max_ma = 0
    outputting = False
    
    def __init__(self, cap_uf, cap_mv, high_threshold_mv, low_threshold_mv, cap_out_mv, cap_max_mv, cap_input_max_ma, outputting):
        self.cap_uf = cap_uf
        self.cap_mv = cap_mv
        self.high_threshold_mv = high_threshold_mv
        self.low_threshold_mv =low_threshold_mv
        self.cap_out_mv = cap_out_mv
        self.cap_max_mv = cap_max_mv
        self.cap_input_max_ma = cap_input_max_ma
        self.outputting = outputting
        
    def increase_energy(self):
        self.cap_uf += 1
        print("Cap: " + str(self.cap_uf) + "uF")
                                        
    def decrease_energy(self):
        self.cap_uf += 1
        print("Cap: " + str(self.cap_uf) + "uF")
        
    def displayEnergySettings(self):
        # Display all the settings
        print("cap_uf: " + str(self.cap_uf) + "uF", end='')
        print("cap_mv: " + str(self.cap_mv) + "mV", end='')
        print("high_threshold_mv: " + str(self.high_threshold_mv) + "mV", end='')
        print("low_threshold_mv: " + str(self.low_threshold_mv) + "mV", end='')
        print("cap_out_mv: " + str(self.cap_out_mv) + "mV", end='')
        print("cap_max_mv: " + str(self.cap_max_mv) + "mV", end='')
        print("cap_input_max_ma: " + str(self.cap_input_max_ma) + "mA", end='')
        print("outputting: " + str(self.outputting), end='')  
        

class EnergyEmulator:
    serialEmulator = None
    reading_thread = None
    
    # General variables
    __wave_type = "square"
    
    # Variables for square wave
    __squareWave = SquareWave(0, 0, 0)
    
    # Variables for virtCap
    __virtCap = VirtCap(0, 0, 0, 0, 0, 0, 0, False)
    
    # Variables for constant voltage
    __constantVoltage = ConstantVoltage(0)
    
    
    
    def checkSerialPortOpen(self):
        if self.serialEmulator.isOpen:
            return True
        logger.error("No serial connection with energy emulator")
        return False

    def return_serialport(self):
        """
        Get Emulator serialport
        :return: Serialport | None
        """
        ports = list(serial.tools.list_ports.comports())
        for p in ports:
            logger.error(p.location)
            if p.description == EMULATOR_DEVICE_ID:
                return p.name

        return None
    
    def sendSupplyValueOption(self, key, value):
        value_request = Supply_interface.SupplyValueChangeRequest()
        value_request.key = key
        if (isinstance(value, int)):
            value_request.value_i = value
        elif (isinstance(value, float)):
            value_request.value_f = value   
        elif (isinstance(value, bool)):
            value_request.value_b = value
        else:
            logger.error("Invalid value type")
            return
        downstream_value_request = Debugger_interface.Downstream()
        downstream_value_request.supply_value_request.CopyFrom(value_request)
        self.serialiseAndSend(downstream_value_request)
        
    def sendOperationMode(self, mode):
        mode_request = Supply_interface.ModeRequest()
        mode_request.mode = mode
        mess = Debugger_interface.Downstream()
        mess.mode_request.CopyFrom(mode_request)
        self.serialiseAndSend(mess)
        
    def serialiseAndSend(self, message):
        serialised = message.SerializeToString()
        length = len(serialised)
        
        if(self.checkSerialPortOpen()):
            self.serialEmulator.write(bytes([0x18, 0xaa])+bytes([length])+serialised)
            self.serialEmulator.flush()
    
    def constantVoltage(self, voltage_mv):
        self.sendOperationMode(Supply_interface.OperationMode.ConstantVoltage)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.constantVoltage, voltage_mv)
        
    def UpdateConstantVoltage(self):
        self.sendOperationMode(Supply_interface.OperationMode.ConstantVoltage)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.constantVoltage, self.__constantVoltage.voltage_mv)

    def squareWave(self, voltage_mv, dutycycle, period_ms):
        self.sendOperationMode(Supply_interface.OperationMode.Square)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.squareVoltage, voltage_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.squareDC, dutycycle)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.squarePeriod, period_ms)
        
    def UpdateSquareWave(self):
        self.sendOperationMode(Supply_interface.OperationMode.Square)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.squareVoltage, self.__squareWave.voltage_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.squareDC, self.__squareWave.dutycycle)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.squarePeriod, self.__squareWave.period_ms)
        
    def sawWave(self, voltage_mv, period_ms):        
        self.sendOperationMode(Supply_interface.OperationMode.Sawtooth)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.sawtoothVoltage, voltage_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.sawtoothPeriod, period_ms)
        
    def off(self):
        self.sendOperationMode(Supply_interface.OperationMode.Off)
        
    def triangleWave(self, voltage_mv, period_ms):
        self.sendOperationMode(Supply_interface.OperationMode.Triangle)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.triangleVoltage, voltage_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.trianglePeriod, period_ms)
        
    def virtCapacitance(self, cap_uf, cap_mv, high_threshold_mv, low_threshold_mv, cap_out_mv, cap_max_mv, cap_input_max_ma, outputting):
        self.sendOperationMode(Supply_interface.OperationMode.Replay)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapCapacitanceuF, cap_uf)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapVoltage, cap_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtOutHighThreshholdV, high_threshold_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtOutLowThreshholdV, low_threshold_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapOutputVoltage, cap_out_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapMaxVoltage, cap_max_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapInputCurrent, cap_input_max_ma)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapOutputting, outputting)

    def updateVirtCapacitance(self):
        self.sendOperationMode(Supply_interface.OperationMode.Replay)        
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapCapacitanceuF, self.__virtCap.cap_uf)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtOutHighThreshholdV, self.__virtCap.high_threshold_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtOutLowThreshholdV, self.__virtCap.low_threshold_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapMaxVoltage, self.__virtCap.cap_max_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapInputCurrent, self.__virtCap.cap_input_max_ma)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapOutputting, self.__virtCap.outputting)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapOutputVoltage, self.__virtCap.cap_out_mv)
        self.sendSupplyValueOption(Supply_interface.SupplyValueOption.virtCapVoltage, self.__virtCap.cap_mv)

    def initVirtCap(self, cap_uf, cap_mv, high_threshold_mv, low_threshold_mv, cap_out_mv, cap_max_mv, cap_input_max_ma, outputting):
        self.__wave_type = "virtCap"
        self.__virtCap = VirtCap(cap_uf, cap_mv, high_threshold_mv, low_threshold_mv, cap_out_mv, cap_max_mv, cap_input_max_ma, outputting)
        self.updateVirtCapacitance()
        
    def initSquareWave(self, voltage_mv, dutycycle, period_ms):
        self.__wave_type = "square"
        self.__squareWave.voltage_mv = voltage_mv
        self.__squareWave.period_ms = period_ms
        self.__squareWave.dutycycle = dutycycle
        self.UpdateSquareWave()
    
    def initConstantVoltage(self, voltage_mv):
        self.__wave_type = "constant"
        self.__constantVoltage.voltage_mv = voltage_mv
        self.UpdateConstantVoltage()
        
    def increaseEnergy(self):
        if self.__wave_type == "square":
            self.__squareWave.increase_energy()
            self.UpdateSquareWave()
        if self.__wave_type == "virtCap":
            self.__virtCap.increase_energy()
            self.updateVirtCapacitance()
        if self.__wave_type == "constant":
            self.__constantVoltage.increase_energy()
            self.UpdateConstantVoltage()
    
    def decreaseEnergy(self):
        if self.__wave_type == "square":
            self.__squareWave.decrease_energy()
            self.UpdateSquareWave()
        if self.__wave_type == "virtCap":
            self.__virtCap.decrease_energy()
            self.updateVirtCapacitance()
        if self.__wave_type == "constant":
            self.__constantVoltage.decrease_energy()
            self.UpdateConstantVoltage()
            
    def resendEnergy(self):
        if self.__wave_type == "square":
            self.UpdateSquareWave()
        if self.__wave_type == "virtCap":
            self.updateVirtCapacitance()
        if self.__wave_type == "constant":
            self.UpdateConstantVoltage()
    
    def displayEnergySettings(self):
        if self.__wave_type == "square":
            self.__squareWave.displayEnergySettings()
        if self.__wave_type == "virtCap":
            self.__virtCap.displayEnergySettings()
        
    def handleProtobuf(self,message):
        message: Debugger_interface.Upstream
        field = message.WhichOneof('packet')
        return
        if(field == "datastream"):
            print("v:" + str(message.datastream.volt_current.voltage) + " c:" + str(message.datastream.volt_current.current))
        if(field == "status_stream"):
            print("status_stream: " + str(message.status_stream.status))   
                             
    def read_data_loop(self):
        print ("Start reading data.")
        self.serialEmulator.timeout = 1.0

        i = 10
        while True:
            fio2_data = self.serialEmulator.read_all()
            if(fio2_data==b''):
                continue
            try:
                # ToDo: support multiple messages in one packet
                if(fio2_data[0] != 0x18 or fio2_data[1] != 0xaa):
                    raise Exception("Invalid header")
                
                result = delimited_protobuf.read(io.BytesIO(fio2_data[2:]), Debugger_interface.Upstream)
                self.handleProtobuf(result)
            except Exception as e:
                print(e)
                
            if(self.serialEmulator.closed):
                return
        
    def auto_connect(self):
        """
        Automatically connect to the Blackmagic
        :return: True | False
        """
        serialport = self.return_serialport()
        if serialport is None:
            logger.error('No Emulator found')
            return False
        else:
            self.serialEmulator = serial.Serial("/dev/" + serialport, timeout=0)
            self.constantVoltage(3300)
            logger.error('Emulator Connected\n\n')
            
            # Only start reading if we are not already reading
            if(self.reading_thread == None):
                self.reading_thread = threading.Thread(target=self.read_data_loop)
                self.reading_thread.start() 
                 
            return True
        
    def disconnect(self):
        self.serialEmulator.close()
        

    