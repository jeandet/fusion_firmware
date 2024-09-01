from .fusion import Fusion
import numpy as np


class AnalogTwo(Fusion):
    def __init__(self, bitstream_path):
        super().__init__(bitstream_path=bitstream_path)

    def get_samples(self, count, flush_before=True):
        self.data_if.read(1024 * 32)
        nbytes = count * 6 * 2 + 12
        buffer = self.data_if.read(nbytes)
        values = np.frombuffer(buffer, dtype=np.int16)
        for i in range(6):
            if values[i] == -3856:
                values = values[i:].copy()
        return [values[i::6] for i in range(2, 6)]
