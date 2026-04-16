""" This is a basic class to make ethercat slave classes
"""
import pysoem
import numpy
import threading
import ctypes
import time

class EthercatSlave:
    def __init__(self, ec_slave):
        self.inputs = None
        self.outputs = None
        self.ec_slave = ec_slave # ethercat slave handle from pysoem

        self.inputs = InputPDO()
        self.outputs = OutputPDO()

        self.stop_event = threading.Event()
        self.rx_thread = threading.Thread(target = self.rx_pdo)
        self.tx_thread = threading.Thread(target = self.tx_pdo)

    def read(self):
        #reads the pdo inputs
        ctypes.memmove(ctypes.addressof(self.inputs), self.ec_slave.input, ctypes.sizeof(self.inputs))

    def write(self):
        #adds outputs to ec_slave outputs
        self.ec_slave.output = bytes(self.outputs)

    
    def rx_pdo(self):
        #read input values
        while not self.stop_event.is_set():
            #read values
            self.read()
        
            #do something

            #wait
            time.sleep(0.01)


    def tx_pdo(self):
        #write output values
        while not self.stop_event.is_set():
            #do something
            self.outputs.test_out += 2
            print(f"tx: {self.outputs.test_out}")

            if self.outputs.LED_out == 1:
                self.outputs.LED_out = 0
            else:
                self.outputs.LED_out = 1

            #transmit
            self.write()
            time.sleep(2)

    def start(self):
        self.tx_thread.start()
        self.rx_thread.start()
        print(f"Starting {self.ec_slave.name}")

    def stop(self):
        self.stop_event.set()
        self.tx_thread.join()
        self.rx_thread.join()

        print(f"Stopping {self.ec_slave.name}")

class InputPDO(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("test_in", ctypes.c_uint8),
        ("LED_in", ctypes.c_uint8),
    ]

class OutputPDO(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("test_out", ctypes.c_uint8),
        ("LED_out", ctypes.c_uint8),
    ]