""" This is a basic class to make ethercat slave classes
"""
import pysoem
import numpy

class EthercatSlave:
    def __init__(self, ec_slave):
        self.inputs = None
        self.outputs = None
        self.ec_slave = ec_slave # ethercat slave handle from pysoem