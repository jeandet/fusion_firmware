#!/usr/bin/env python3

from migen import *
from litex.gen import *
from litex.build.generic_platform import *
from litex.build.lattice import LatticeECP5Platform
import os

__HERE__ = os.path.abspath(os.path.dirname(__file__))

import sys

sys.path.append(os.path.join(__HERE__, "../"))
sys.path.append(os.path.join(__HERE__, "../../HDL/"))

from fusion_platform import AnalogTwoPlatform
from nor_interface import Stm32FmcNorInterface
from ads92x4 import ads92x4
from data_encoder import DataEncoder


class TopModule(LiteXModule):
    def __init__(self, platform):
        self.smp_clk_cntr = Signal(16, reset=0)
        self.sync += self.smp_clk_cntr.eq(self.smp_clk_cntr + 1)

        self.smp_clk = Signal()
        self.comb += self.smp_clk.eq(self.smp_clk_cntr[7])

        self.nor_if = Stm32FmcNorInterface(pads=platform.fmc_pads)
        self.submodules += self.nor_if
        self.adc1 = ads92x4()
        self.adc2 = ads92x4()
        self.submodules.adc1 = self.adc1
        self.submodules.adc2 = self.adc2

        self.data_encoder = DataEncoder(adc_count=2)
        self.submodules.data_encoder = self.data_encoder

        self.comb += self.adc1.smp_clk.eq(self.smp_clk)
        self.comb += self.adc2.smp_clk.eq(self.smp_clk)

        platform.connect_adc1(self.adc1)
        platform.connect_adc2(self.adc2)

        for i, adc in enumerate((self.adc1, self.adc2)):
            self.comb += self.data_encoder.adc_if[i].data_cha.eq(adc.data_cha)
            self.comb += self.data_encoder.adc_if[i].data_chb.eq(adc.data_chb)

        self.comb += self.nor_if.fifo_din.eq(self.data_encoder.fifo_if.fifo_din)
        self.comb += self.nor_if.fifo_we.eq(self.data_encoder.fifo_if.fifo_we)
        self.comb += self.data_encoder.smp_clk.eq(self.smp_clk)

        self.comb += platform.have_data.eq(self.nor_if.have_data)


platform = AnalogTwoPlatform()
top = TopModule(platform)
platform.register_main_clock(top)

# Build --------------------------------------------------------------------------------------------

platform.build(top)
