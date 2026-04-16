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
        self.expected_wkc = 0
        self.pdo_thread_stop_event = threading.Event()

        self.slaves_list = []
        self.slaves_in_op = False
    
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
            
            self.expected_wkc = self.master.config_map()

            if self.master.state_check(pysoem.SAFEOP_STATE, 50000) != pysoem.SAFEOP_STATE:
                self.master.read_state()
                for slave in self.master.slaves:
                    if not slave.state == pysoem.SAFEOP_STATE:
                        raise Exception('not all slaves reached SAFEOP state')
                    slave.dc_sync(act = True, sync0_cycle_time=10_000_000)
            
            self.master.state = pysoem.OP_STATE

            #start pdo thread
            self.pdo_thread = threading.Thread(target=self.pdo)
            self.pdo_thread.start() 
            self.master.send_processdata()
            self._wkc_ = self.master.receive_processdata(timeout=2000)

            self.master.write_state()
            while not self.slaves_in_op:
                self.master.state_check(pysoem.OP_STATE, timeout=50_000)
                if self.master.state == pysoem.OP_STATE:
                    self.slaves_in_op = True


            #start slaves
            self.run()

            #do nothing until aborted
            try:
                while 1:
                    #test case to see if outputs are going through: change in output should show in input
                    print(f"rx: {self.slaves_list[0].inputs.test_in}")
                    print(f"tx: {self.slaves_list[0].outputs.test_out}")

                    #do nothing
                    time.sleep(1)
            except KeyboardInterrupt:
                print("Stop requested")

            
            #stop ethercat
            self.stop()




        except Exception as e:
            self.stop()
            print(f"Intitialization error: {e}")


    def init_slaves(self):

        #create the slave classes to handle each ethercat slave
        for i, slave in enumerate(self.master.slaves):
            print(f"Slave {i}: {slave.name}")
            match slave.name:
                case "EtherCAT_Slave":
                    self.slaves_list.append(EthercatSlave(slave))
                case _:
                    continue # skip unknown slaves
    
    def run(self):
        for slave in self.slaves_list:
            slave.start()

    def stop(self):
        #kill the slaves and their threads
        for slave in self.slaves_list:
            slave.stop()

        #kill pdo and check threads
        self.pdo_thread_stop_event.set()
        self.pdo_thread.join()

        #send master back to init state
        self.master.state = pysoem.INIT_STATE
        self.master.write_state()
        
        #close master
        print("Closing master")
        self.master.close()


    def pdo(self):
        #actual thread to read and write data
        while not self.pdo_thread_stop_event.is_set():
            self.master.send_processdata()
            self._wkc_ = self.master.receive_processdata(timeout=2000)

            time.sleep(0.01) #100 Hz rate




        



if __name__ == "__main__":
    ec = EthercatMaster()
    ec.choose_adapter()
    ec.start()