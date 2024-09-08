from litex.gen import *
from litex.soc.cores.clock.common import *
from migen.genlib.fifo import SyncFIFO

from nor_interface import Stm32FmcNorInterface
from ads92x4 import Ads92x4
from data_encoder import DataEncoder
from fifo_8_to_32_bits import Fifo8to32Bits
from serialized_fifo import SerializeFifo


class AcquisitionPipeline(LiteXModule):
    def __init__(
        self,
        adc_count,
        fmc_data_width=32,
        fmc_address_width=3,
        use_chained_fifo=True,
        fifo_depth=256,
        fifo_count=16,
        smp_clk_is_synchronous=True,
        oversampling=1,
        zone=2,
    ):
        self.smp_clk = Signal()
        self.have_data = Signal()

        self.nor_if = Stm32FmcNorInterface(
            data_width=fmc_data_width, address_width=fmc_data_width
        )
        self.submodules += self.nor_if
        self.adcs = [
            Ads92x4(
                smp_clk_is_synchronous=smp_clk_is_synchronous,
                oversampling=oversampling,
                zone=2,
            )
            for _ in range(adc_count)
        ]
        for adc in self.adcs:
            self.submodules += adc
            self.comb += adc.smp_clk.eq(self.smp_clk)

        self.data_encoder = DataEncoder(adc_count=adc_count)
        self.submodules.data_encoder = self.data_encoder

        for i, adc in enumerate(self.adcs):
            self.comb += self.data_encoder.acd_data[i][0].eq(adc.data_cha)
            self.comb += self.data_encoder.acd_data[i][1].eq(adc.data_chb)

        if use_chained_fifo:
            self.fifo_8bits = SerializeFifo(
                width=8, fifo_depth=fifo_depth, fifo_count=fifo_count
            )
        else:
            self.fifo_8bits = SyncFIFO(width=8, depth=fifo_depth * fifo_count)
        self.submodules += self.fifo_8bits

        self.comb += self.fifo_8bits.din.eq(self.data_encoder.fifo_din)
        self.comb += self.fifo_8bits.we.eq(self.data_encoder.fifo_we)
        self.comb += self.data_encoder.fifo_writable.eq(self.fifo_8bits.writable)

        self._fifo8to32 = Fifo8to32Bits()
        self.submodules += self._fifo8to32

        self.comb += self._fifo8to32.input_fifo_has_word.eq(self.fifo_8bits.level >= 4)
        self.comb += self._fifo8to32.din.eq(self.fifo_8bits.dout)
        self.comb += self.fifo_8bits.re.eq(self._fifo8to32.re)

        self.nor_if_has_enough_space = Signal()
        self.comb += self.nor_if_has_enough_space.eq(
            self.nor_if.level < self.nor_if.fifo.depth - 4
        )
        self.comb += self._fifo8to32.output_fifo_has_enough_space.eq(
            self.nor_if_has_enough_space
        )
        self.comb += self.nor_if.fifo_din.eq(self._fifo8to32.dout)
        self.comb += self.nor_if.fifo_we.eq(self._fifo8to32.we)
        self.comb += self.data_encoder.smp_clk.eq(self.adcs[0].smp_clk_out)

        self.comb += self.have_data.eq(self.nor_if.have_data)


if __name__ == "__main__":
    from migen.fhdl.verilog import convert

    convert(AcquisitionPipeline(adc_count=2, use_chained_fifo=False)).write(
        "AcquisitionPipeline.v"
    )
