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
        self.rx_thread = threading.Thread(target = self.rx)
        self.tx_thread = threading.Thread(target = self.tx)

    def read(self):
        #reads the pdo inputs
        ctypes.memmove(ctypes.addressof(self.inputs), self.ec_slave.input, ctypes.sizeof(self.inputs))

    def write(self):
        #adds outputs to ec_slave outputs
        self.ec_slave.output = bytes(self.outputs)

    
    def rx(self):
        #read input values
        while not self.stop_event.is_set():
            #read values
            self.read()
        
            #do something

            #wait
            time.sleep(0.01)


    def tx(self):
        #write output values
        while not self.stop_event.is_set():
            #do something
            self.outputs.test_out += 1

            #transmit
            self.write()
            time.sleep(0.01)

    def start(self):
        self.tx_thread.start()
        self.rx_thread.start()

    def stop(self):
        self.stop_event.set()
        self.tx_thread.join()
        self.rx_thread.join()

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