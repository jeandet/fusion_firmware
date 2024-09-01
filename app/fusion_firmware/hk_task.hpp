#include "fusion_bsp/board.hpp"
#include <modm/processing/rtos.hpp>
#include <modm/processing.hpp>
#include "buffers.hpp"
#include <charconv>

using namespace Board;
using namespace modm::platform;
using namespace std::chrono_literals;

class HKTask : modm::rtos::Thread
{
public:
    HKTask() : Thread(1, 2048, "HK task")
    {
    }

    void run()
    {
        float voltage;
        modm::PeriodicTimer tmr{1000ms};

        auto p = USBDataQueue::new_empty_packet();
        
        Adc3::setChannel(Adc3::Channel::Channel4);
        while (1)
        {
            Adc3::startConversion();
            // wait for conversion to finish
            while (!Adc3::isConversionFinished())
                vTaskDelay(1);
            // print result
            int adcValue = Adc3::getValue();
            voltage = adcValue * 3.3 / 0xfff;
            auto [ptr, ec]= std::to_chars(p.data+p.count, p.data+p.capacity-1,voltage);
            ptr[0]='\n';
            p.count=ptr-p.data+1;
            if(p.count>=p.capacity-32)
            {
                USBDataQueue::push_tx(std::move(p));
                p=USBDataQueue::new_empty_packet();
            }
            vTaskDelay(1);
        }

        vTaskDelete(0);
    }
};
