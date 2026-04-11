import os
import sys
import pysoem
import threading
import time

# Add parent directory to sys.path for importing EthercatSlave
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from EthercatSlave.ec_slave import EthercatSlave

class EthercatMaster:
    def __init__(self):
        self.master = pysoem.Master()
        self.if_name = ""
        self._wkc_ = 0
        self.pdo_thread_stop_event = threading.Event()

        self.slaves_list = []
    
    def get_adapters(self):
        #Get adapter names
        self.adapters = pysoem.find_adapters()

        if not self.adapters:
            print("No adapters found! Check if Npcap or WinPcap is installed")
            return
        print(f"Found {len(self.adapters)} adapters:")
        for i, adapter in enumerate(self.adapters):
            print(f"{i}. {adapter.desc.decode('utf-8')}")

        #pysoem needs the adapter.name but we need to read the description to know which adapter to connect to so we'll make a dict linking the two to add to a GUI
        self.adapters_dict = {adapter.desc.decode('utf-8'): adapter.name for adapter in self.adapters}
        return self.adapters_dict.copy()

    def choose_adapter(self):
        #use if just using terminal to start ethercat
        self.get_adapters()
        idx = int(input("Choose the adapter number:"))
        self.if_name = self.adapters[idx].name
        print(f"Choosing {self.adapters[idx].desc.decode('utf-8')}")
    
    def start(self):
        try:
            #start ethercat
            self.master.open(self.if_name)

            #initialize all the slaves
            if not self.master.config_init() > 0:
                raise Exception("No slaves detected!")

            #make list of slaves
            self.init_slaves()
            if len(self.slaves_list) < self.master.config_init()-1:
                raise Exception(f"Issue initializing slave {len(self.slaves_list)}")




        except Exception as e:
            self.stop()
            print(f"Intitialization error: {e}")

    
    def stop(self):
        self.pdo_thread_stop_event.set()
        self.master.close()

    def init_slaves(self):
        #create the slave classes to handle each ethercat slave
        for i, slave in enumerate(self.master.slaves):
            print(slave.name)
            match slave.name:
                case "EtherCAT_Slave":
                    self.slaves_list.append(EthercatSlave(slave))
                case _:
                    continue # skip unknown slaves
            


    def pdo_thread(self):
        #actual thread to read and write data
        while not self.pdo_thread_stop_event.is_set():
            self.master.send_processdata()
            self._wkc_ = self.master.receive_processdata()

            time.sleep(0.01) #100 Hz rate




        



if __name__ == "__main__":
    ec = EthercatMaster()
    ec.choose_adapter()
    ec.start()
    ec.stop()