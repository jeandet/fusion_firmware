from litex.gen import *
from litex.soc.cores.clock.common import *


class ads92x4(LiteXModule):
    def __init__(self):
        super().__init__()
        self.smp_clk = Signal()
        self.data_cha = Signal(16)
        self.data_chb = Signal(16)

        self.pads = Record(
            [
                ("conv_st", 1),
                ("cs", 1),
                ("ready_strobe", 1),
                ("sclk", 1),
                ("mosi", 1),
                ("miso_a", 1),
                ("miso_b", 1),
            ]
        )

        self.read_cycles = Signal(5, reset=0)
        self.shift_reg_a = Signal(16, reset=0)
        self.shift_reg_b = Signal(16, reset=0)

        self.comb += self.pads.conv_st.eq(self.smp_clk)

        self.fsm = FSM(reset_state="IDLE")
        self.fsm.act(
            "IDLE",
            NextValue(self.data_cha, self.shift_reg_a),
            NextValue(self.data_chb, self.shift_reg_b),
            NextValue(self.read_cycles, 0),
            NextValue(self.pads.cs, 1),
            If(self.smp_clk, NextState("WAIT_RDY")),
        )
        self.fsm.act(
            "WAIT_RDY",
            If(
                self.pads.ready_strobe,
                NextState("ASSERT_CS"),
                NextValue(self.pads.cs, 0),
            ),
        )
        self.fsm.act(
            "ASSERT_CS",
            NextState("READ"),
            NextValue(self.shift_reg_a, Cat(self.pads.miso_a, self.shift_reg_a[:-1])),
            NextValue(self.shift_reg_b, Cat(self.pads.miso_b, self.shift_reg_b[:-1])),
        )
        self.fsm.act(
            "READ",
            If(
                self.read_cycles == 15,
                NextState("ENSURE_SMP_CLK_LOW"),
                NextValue(self.pads.cs, 1),
                NextValue(self.read_cycles, 0),
            ).Else(
                NextValue(self.read_cycles, self.read_cycles + 1),
                NextValue(
                    self.shift_reg_a, Cat(self.pads.miso_a, self.shift_reg_a[:-1])
                ),
                NextValue(
                    self.shift_reg_b, Cat(self.pads.miso_b, self.shift_reg_b[:-1])
                ),
            ),
        )
        self.fsm.act(
            "ENSURE_SMP_CLK_LOW",
            NextValue(self.pads.cs, 1),
            If(~self.smp_clk, NextState("IDLE")),
        )

        self.comb += self.pads.sclk.eq(ClockSignal() & self.fsm.ongoing("READ"))
