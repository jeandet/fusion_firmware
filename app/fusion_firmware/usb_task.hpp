#pragma once
#include "fusion_bsp/board.hpp"
#include "buffers.hpp"
#include "board_states.hpp"
#include <modm/processing/rtos.hpp>
#include <modm/processing.hpp>
#include <modm/processing/rtos/queue.hpp>

#include <tusb.h>
#include <cmath>

using namespace Board;
using namespace modm::platform;
using namespace std::chrono_literals;

static inline constexpr auto USB_CDC_Cmd = 0;
static inline constexpr auto USB_CDC_Data = 1;

char test[1024 * 8];

class UsbTask : modm::rtos::Thread
{

    template <typename Queue, uint8_t CDC_port>
    inline void receive()
    {
        if (auto packet = Queue::maybe_new_empty_packet(); packet)
        {

            while (tud_cdc_n_available(CDC_port) && packet->count < packet->capacity)
            {
                packet->count += tud_cdc_n_read(CDC_port, packet->data + packet->count, packet->capacity - packet->count);
                LED2::set(0);
            }

            if (packet->count != 0)
            {
                Queue::push_rx(std::move(*packet));
            }
            LED2::set(1);
        }
        else
        {
            LED2::set(0);
        }
    }

    template <typename Queue, uint8_t CDC_port>
    inline void send()
    {
        if (auto data_to_send = Queue::maybe_tx(1); data_to_send)
        {
            tud_cdc_n_write(CDC_port, data_to_send->data, data_to_send->count);
            // tud_cdc_n_write_flush(CDC_port);
        }
    }

public:
    UsbTask() : Thread(1, 1024, "usb_task")
    {
    }

    void
    run()
    {
        Board::initializeUsbHs();
        tusb_init();
        while (1)
        {
            tud_task();
            receive<USBCommands, USB_CDC_Cmd>();
            send<USBCommands, USB_CDC_Cmd>();
            if (board_state != BoardState::Idle)
            {
                receive<USBDataQueue, USB_CDC_Data>();
                send<USBDataQueue, USB_CDC_Data>();
            }
        }
        vTaskDelete(0);
    }
};