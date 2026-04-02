import pysoem
import threading

class EthercatMaster:
    def __init__(self):
        self.master = pysoem.Master() 
    
    def get_adapters(self):
        #Get adapter names
        self.adapters = pysoem.find_adapters()

        if not self.adapters:
            print("No adapters found! Check if Npcap or WinPcap is installed")
            return
        print(f"Found {len(self.adapters)} adapters:")
        for i, adapter in enumerate(self.adapters):
            print(f"{i}. {adapter.desc.decode('utf-8')}")

        #pysoem needs the adapter.name but we need to read the description to know which adapter to connect to so we'll make a dict connecting the two to add to a GUI
        self.adapters_dict = {adapter.desc.decode('utf-8'): adapter.name for adapter in self.adapters}



        



if __name__ == "__main__":
    ec = EthercatMaster()