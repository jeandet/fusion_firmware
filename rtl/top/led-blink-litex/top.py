#!/usr/bin/env python3

from migen import *
from litex.gen import *
from litex.build.generic_platform import *
from litex.build.lattice import LatticeECP5Platform
import os
__HERE__ = os.path.abspath(os.path.dirname(__file__))

import sys
sys.path.append(os.path.join(__HERE__, "../"))

from fusion_platform import FusionPlatform

class TopModule(LiteXModule):
    def __init__(self):
        self.led = Signal()
        self.counter = Signal(22)
        self.comb += self.led.eq(self.counter[21])
        self.sync += self.counter.eq(self.counter + 1)

platform = FusionPlatform()
led = platform.request("LED")
top = TopModule()
top.comb += led.eq(top.led)
platform.register_main_clock(top)

# Build --------------------------------------------------------------------------------------------

platform.build(top)