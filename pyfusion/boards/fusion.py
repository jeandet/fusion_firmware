import serial
from serial.tools.list_ports import comports
import numpy as np



def find_device(dev_type="CMD"):
    ports = comports()
    for p in ports:
        if p.interface is not None and f"Fusion {dev_type}" in p.interface:
            return p.device


def set_status_led(s: serial.Serial, value):
    data = bytearray(2)
    data[0] = 2
    data[1] = value
    s.write(data)


def set_fpga_reset(s: serial.Serial, value):
    data = bytearray(2)
    data[0] = 3
    data[1] = value
    s.write(data)


def load_bitstream(cmd_if: serial.Serial, data_if: serial.Serial, path: str):
    print(f"CMD if = {cmd_if.port}  DATA if = {data_if.port}")
    with open(path, "rb") as bs:
        data = bs.read()
        cmd = bytearray(5)
        bs_size = len(data)
        print(f"Loading bitstream, size={bs_size}")
        cmd[0] = 1
        cmd[1] = bs_size // 2**24 & 255
        cmd[2] = bs_size // 2**16 & 255
        cmd[3] = bs_size // 2**8 & 255
        cmd[4] = bs_size & 255
        print(cmd)
        cmd_if.write(cmd)
        data_if.write(data)


class Fusion:
    def __init__(self, bitstream_path=None):
        self.cmd_if = None
        self.data_if = None
        self.bitstream_path = bitstream_path

    def open(self):
        self.cmd_if = serial.Serial(find_device("CMD"))
        self.data_if = serial.Serial(find_device("DATA"))
        if self.bitstream_path is not None:
            self.load_bitstream(self.bitstream_path)

    def close(self):
        self.cmd_if.close()
        self.data_if.close()

    def load_bitstream(self, path: str):
        load_bitstream(self.cmd_if, self.data_if, path)

    def set_status_led(self, value):
        set_status_led(self.cmd_if, value)

    def set_fpga_reset(self, value):
        set_fpga_reset(self.cmd_if, value)

