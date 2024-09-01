#include "fusion_bsp/board.hpp"
#include <modm/processing/rtos.hpp>
#include <modm/processing.hpp>
#include "usb_task.hpp"
#include "hk_task.hpp"
#include "fpga_task.hpp"
#include "commands_handler.hpp"
#include <stdlib.h>


using namespace Board;
using namespace modm::platform;
using namespace std::chrono_literals;

const char* tud_string_desc_arr[] = {
        NULL,                           // 0: Language
        "LPP",                      // 1: Manufacturer
        "Fusion Board",       // 2: Product
        NULL,                           // 3: Serials, should use chip ID
        "Fusion CMD",         // 4: CDC0 Interface
        "Fusion DATA",         // 4: CDC0 Interface
};




UsbTask usbTask;
//HKTask hkTask;
CommandsHandler cmdHandlerTask;
FPGATask fpga_task;

int main()
{
    Board::initialize();
    Board::Leds::toggle();
    modm::rtos::Scheduler::schedule();
}
