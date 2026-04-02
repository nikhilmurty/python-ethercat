# Python Ethercat Repository

This is a repo to help with learning how to use ethercat. When I was learning how to use it, it sucked that there weren't a lot of good tutorials out thereto learn and grasp everything. This sucks because EtherCAT is extremely useful in the world of robotics. So, I wanted to make a repo showing how to make an ethercat master and how to make your own ethercat slaves using a LAN9252 chip.

## Setup
### EthercatMaster
This section sets up a simple ethercat master template using pysoem, an python wrapper for SOEM which is an open source ethercat master library. 

### EthercatSlave
This section is to show how to make your own ethercat slave using a STM32 Nucleo and a Bausano EasyCAT shield, which is a LAN9252 shield that can be used on Arduino UNOs and anything with that footprint.