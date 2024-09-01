from litex.gen import *
from litex.soc.cores.clock.common import *


class DataEncoder(LiteXModule):
    def __init__(self, adc_count=1):
        self.adc_if = [
            Record([("data_cha", 16), ("data_chb", 16)]) for i in range(adc_count)
        ]
        self.smp_clk = Signal(1)
        self.fifo_if = Record([("fifo_din", 32), ("fifo_we", 1), ("fifo_writable", 1)])

        self._frame_counter = Signal(16, reset=0)

        self.fsm = FSM(reset_state="IDLE")
        self.fsm.act(
            "IDLE",
            NextValue(self.fifo_if.fifo_we, 0),
            If(self.smp_clk, NextState("push_header")),
        )
        self.fsm.act(
            "push_header",
            NextValue(self.fifo_if.fifo_we, 1),
            NextValue(self.fifo_if.fifo_din, Cat(0xF0F0, self._frame_counter)),
            NextState("push_adc0"),
        )
        for i in range(adc_count - 1):
            self.fsm.act(
                f"push_adc{i}",
                NextState(f"push_adc{i+1}"),
                NextValue(
                    self.fifo_if.fifo_din,
                    Cat(self.adc_if[i].data_cha, self.adc_if[i].data_chb),
                ),
                NextValue(self.fifo_if.fifo_we, 1),
            )
        self.fsm.act(
            f"push_adc{adc_count-1}",
            NextState("wait_for_smp_clk_low"),
            NextValue(
                self.fifo_if.fifo_din,
                Cat(
                    self.adc_if[adc_count - 1].data_cha,
                    self.adc_if[adc_count - 1].data_chb,
                ),
            ),
            NextValue(self.fifo_if.fifo_we, 1),
        )
        self.fsm.act(
            "wait_for_smp_clk_low",
            NextValue(self.fifo_if.fifo_we, 0),
            If(
                ~self.smp_clk,
                NextState("IDLE"),
                NextValue(self._frame_counter, self._frame_counter + 1),
            ),
        )
