#include "board_states.hpp"
#include "buffers.hpp"
#include "fusion_bsp/board.hpp"
#include <charconv>
#include <modm/processing.hpp>
#include <modm/processing/rtos.hpp>

using namespace Board;
using namespace modm::platform;
using namespace std::chrono_literals;

// volatile uint32_t *FPGA_Space = reinterpret_cast<uint32_t *>(0x6000'0000);
volatile uint32_t* FPGA_Space = reinterpret_cast<uint32_t*>(0xC000'0000);

class FPGATask : modm::rtos::Thread
{
public:
    FPGATask() : Thread(1, 2048, "FPGATask task") { }

    void run()
    {

        auto p = USBDataQueue::new_empty_packet();
        while (1)
        {
            while (board_state == BoardState::Running)
            {
                while (p.count < p.capacity)
                {
                    while (!FPGA::have_data())
                        ;
                    // have data means fpga fifo has at least 32 words
                    for (auto _ = 0U; _ < 32; _++)
                    {
                        reinterpret_cast<uint32_t*>(p.data)[p.count / 4] = FPGA::value(p.count / 4);
                        p.count += 4;
                    }
                    // since fpga clk is FMC clk ~50MHz and CPU clk is >100MHz we have to wait few
                    // cpu cycles to let have_data pin go low
                    for (volatile int nop = 0; nop < 10; nop++)
                        ;
                }
                USBDataQueue::push_tx(std::move(p));
                p = USBDataQueue::new_empty_packet();
            }

            vTaskDelay(1);
        }

        vTaskDelete(0);
    }
};
