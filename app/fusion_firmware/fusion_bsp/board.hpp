/*
 * Copyright (c) 2016-2017, Sascha Schade
 * Copyright (c) 2016-2018, Niklas Hauser
 * Cpoyright (c) 2020, Felix Petriconi
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef MODM_FUSION_HPP
#define MODM_FUSION_HPP

#include <modm/architecture/interface/clock.hpp>
#include <modm/debug/logger.hpp>
#include <modm/driver/storage/block_device_spiflash.hpp>
#include <modm/platform.hpp>
#include <modm/platform/spi/bitbang_spi_master.hpp>

#include "../lattice_fpga.hpp"

#define MODM_BOARD_HAS_LOGGER

using namespace modm::platform;

namespace Board
{

using namespace modm::literals;

/// STM32F7 running at 216MHz from the external 12MHz clock
struct SystemClock
{
    static constexpr uint32_t Frequency = 168_MHz;
    static constexpr uint32_t Apb1 = Frequency / 4;
    static constexpr uint32_t Apb2 = Frequency / 2;
    static constexpr uint32_t Apb1Timer = Apb1 * 2;
    static constexpr uint32_t Apb2Timer = Apb2 * 2;

    static constexpr uint32_t Ethernet = Frequency;

    static constexpr uint32_t Adc = Apb2;

    static constexpr uint32_t Adc1 = Apb2;
    static constexpr uint32_t Adc2 = Apb2;
    static constexpr uint32_t Adc3 = Apb2;

    static constexpr uint32_t Dac = Apb1;

    static constexpr uint32_t Spi1 = Apb2;
    static constexpr uint32_t Spi2 = Apb1;
    static constexpr uint32_t Spi3 = Apb1;
    static constexpr uint32_t Spi4 = Apb2;
    static constexpr uint32_t Spi5 = Apb2;
    static constexpr uint32_t Spi6 = Apb2;

    static constexpr uint32_t Usart1 = Apb2;
    static constexpr uint32_t Usart2 = Apb1;
    static constexpr uint32_t Usart3 = Apb1;
    static constexpr uint32_t Uart4 = Apb1;
    static constexpr uint32_t Uart5 = Apb1;
    static constexpr uint32_t Usart6 = Apb2;
    static constexpr uint32_t Uart7 = Apb1;
    static constexpr uint32_t Uart8 = Apb1;

    static constexpr uint32_t Can1 = Apb1;

    static constexpr uint32_t I2c1 = Apb1;
    static constexpr uint32_t I2c2 = Apb1;
    static constexpr uint32_t I2c3 = Apb1;

    static constexpr uint32_t Timer1 = Apb2Timer;
    static constexpr uint32_t Timer2 = Apb1Timer;
    static constexpr uint32_t Timer3 = Apb1Timer;
    static constexpr uint32_t Timer4 = Apb1Timer;
    static constexpr uint32_t Timer5 = Apb1Timer;
    static constexpr uint32_t Timer6 = Apb1Timer;
    static constexpr uint32_t Timer7 = Apb1Timer;
    static constexpr uint32_t Timer8 = Apb2Timer;
    static constexpr uint32_t Timer9 = Apb2Timer;
    static constexpr uint32_t Timer10 = Apb2Timer;
    static constexpr uint32_t Timer11 = Apb2Timer;
    static constexpr uint32_t Timer12 = Apb1Timer;
    static constexpr uint32_t Timer13 = Apb1Timer;
    static constexpr uint32_t Timer14 = Apb1Timer;

    static constexpr uint32_t Usb = 48_MHz;

    static constexpr uint32_t Iwdg = Rcc::LsiFrequency;

    static bool inline enable()
    {
        Rcc::enableExternalCrystal(); // 12 MHz
        const Rcc::PllFactors pllFactors {
            .pllM = 8, // 12MHz / M=4   -> 3MHz
            .pllN = 224, // 2MHz * N=216 -> 336MHz
            .pllP = 2, // 336MHz / P=2 -> 168MHz = F_cpu
            .pllQ = 7 // 336MHz / Q=7 -> 48MHz (USB, etc.)
        };
        Rcc::enablePll(Rcc::PllSource::Hse, pllFactors);
        // Required for 216 MHz clock
        Rcc::enableOverdriveMode();
        Rcc::setFlashLatency<Frequency>();
        Rcc::enableSystemClock(Rcc::SystemClockSource::Pll);
        // APB1 is running at 54MHz, since AHB / 4 = 54MHz (= limit)
        Rcc::setApb1Prescaler(Rcc::Apb1Prescaler::Div4);
        // APB2 is running at 108MHz, since AHB / 2 = 108MHz (= limit)
        Rcc::setApb2Prescaler(Rcc::Apb2Prescaler::Div2);
        Rcc::setClock48Source(Rcc::Clock48Source::PllQ);
        Rcc::updateCoreFrequency<Frequency>();

        return true;
    }
};

using Button1 = GpioInputG9;
using Button2 = GpioInputI5;

using LED1 = GpioOutputI4; // LED1
using LED2 = GpioOutputE1; // LED2
using STATUS_LED = LED1;
using Leds = SoftwareGpioPort<LED1, LED2>;

using HK_3_3V = GpioInputA0;
using HK_1_1V = GpioInputA1;
using HK_2_5V = GpioInputA3;

using ADC_IN0 = GpioInputF4;
using ADC_IN1 = GpioInputF5;
using ADC_IN2 = GpioInputF6;
using ADC_IN3 = GpioInputF7;
using ADC_IN4 = GpioInputF8;
using ADC_IN5 = GpioInputF9;

namespace FPGA_CFG
{
    using SCK = GpioOutputB3;
    using MOSI = GpioOutputC3;
    using PROGRAM = GpioOutputC0;
    using DONE = GpioInputC1;
    using INIT = GpioOutputC2;

    using SPI = modm::platform::BitBangSpiMaster<SCK, MOSI>;
}; // namespace FPGA_CFG

namespace FMC
{
    using D0 = GpioOutputD14;
    using D1 = GpioOutputD15;
    using D2 = GpioOutputD0;
    using D3 = GpioOutputD1;
    using D4 = GpioOutputE7;
    using D5 = GpioOutputE8;
    using D6 = GpioOutputE9;
    using D7 = GpioOutputE10;
    using D8 = GpioOutputE11;
    using D9 = GpioOutputE12;
    using D10 = GpioOutputE13;
    using D11 = GpioOutputE14;
    using D12 = GpioOutputE15;
    using D13 = GpioOutputD8;
    using D14 = GpioOutputD9;
    using D15 = GpioOutputD10;
    using D16 = GpioOutputH8;
    using D17 = GpioOutputH9;
    using D18 = GpioOutputH10;
    using D19 = GpioOutputH11;
    using D20 = GpioOutputH12;
    using D21 = GpioOutputH13;
    using D22 = GpioOutputH14;
    using D23 = GpioOutputH15;
    using D24 = GpioOutputI0;
    using D25 = GpioOutputI1;
    using D26 = GpioOutputI2;
    using D27 = GpioOutputI3;
    using D28 = GpioOutputI6;
    using D29 = GpioOutputI7;
    using D30 = GpioOutputI9;
    using D31 = GpioOutputI10;

    using A0 = GpioOutputF0;
    using A1 = GpioOutputF1;
    using A2 = GpioOutputF2;

    using CLK = GpioOutputD3;
    using NOE = GpioOutputD4;
    using NWE = GpioOutputD5;
    using NWAIT = GpioOutputD6;
    using NE1 = GpioOutputD7;

    using FPGA_HAVE_DATA = GpioOutputF3;
}; // namespace FMC

using FPGA_IO_0 = GpioOutputC4;
using FPGA_IO_1 = GpioOutputC5;

using FPGA_HAVE_DATA = GpioOutputF3;

using FPGA_RESET = FPGA_IO_0;

using FPGA
    = lattice_fpga_t<FPGA_CFG::SPI, FPGA_CFG::PROGRAM, FPGA_HAVE_DATA, FPGA_CFG::INIT, 0xC000'0000>;
/// @}

namespace Flash
{
    using Spi = SpiMaster4;
    namespace SpiPins
    {
        using Sck = GpioOutputE2;
        using Miso = GpioOutputE5;
        using Mosi = GpioOutputE6;
    }
    using Cs = GpioOutputE4;
    using memory_t = modm::BdSpiFlash<Spi, Cs, 32 * 1024*1024 / 8>;
    extern memory_t memory;
    static constexpr inline uint32_t partition_size = 1024 * 1024 / 8;
}

namespace usb_hs
{

    using Dm = GpioB14;
    using Dp = GpioB15;
    using Vbus = GpioB13;

    using Device = UsbHs;

} // namespace usb_hs

namespace stlink
{
    using Tx = GpioOutputA9;
    using Rx = GpioInputA10;
    using Uart = BufferedUart<UsartHal1, UartTxBuffer<2048>>;
} // namespace stlink

using LoggerDevice = modm::IODeviceWrapper<stlink::Uart, modm::IOBuffer::BlockIfFull>;

inline void initializeUsbHs(uint8_t priority = 3)
{
    using Connector
        = GpioConnector<Peripheral::Usbotghs, usb_hs::Dp::Dp, usb_hs::Dm::Dm, usb_hs::Vbus::Vbus>;
    using Dp = typename Connector::template GetSignal<Gpio::Signal::Dp>;
    using Dm = typename Connector::template GetSignal<Gpio::Signal::Dm>;
    using Vbus = typename Connector::template GetSignal<Gpio::Signal::Vbus>;
    GpioSet<Dp, Dm>::configure(Gpio::OutputType::PushPull, Gpio::OutputSpeed::VeryHigh);
    GpioSet<Vbus>::configure(Gpio::InputType::Floating);
    Connector::connect();
    // Gpio::Signal::Vbus
    // usb_hs::Vbus::

    // usb_hs::Device::connect<usb_hs::Dm::Dm, usb_hs::Dp::Dp,
    // usb_hs::Vbus::Vbus>();

    // Deactivate VBUS Sensing B
    USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_VBDEN;

    RCC->APB2ENR |= RCC_APB2ENR_OTGPHYCEN;
    Rcc::enable<Peripheral::Usbotghs>();
    Rcc::enable<Peripheral::Usbotghsulpi>();
    NVIC_SetPriority(OTG_HS_IRQn, priority);
    NVIC_SetPriority(OTG_HS_EP1_IN_IRQn, priority);
    NVIC_SetPriority(OTG_HS_EP1_OUT_IRQn, priority);
    NVIC_SetPriority(OTG_HS_WKUP_IRQn, priority);

    USB_OTG_HS->GCCFG &= ~(USB_OTG_GCCFG_PWRDWN);

    /* Init The UTMI Interface */
    USB_OTG_HS->GUSBCFG
        &= ~(USB_OTG_GUSBCFG_TSDPS | USB_OTG_GUSBCFG_ULPIFSLS | USB_OTG_GUSBCFG_PHYSEL);

    /* Select vbus source */
    USB_OTG_HS->GUSBCFG &= ~(USB_OTG_GUSBCFG_ULPIEVBUSD | USB_OTG_GUSBCFG_ULPIEVBUSI);

    /* Select UTMI Interface */
    USB_OTG_HS->GUSBCFG &= ~USB_OTG_GUSBCFG_ULPI_UTMI_SEL;

    USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_PHYHSEN;

    USB_HS_PHYC->USB_HS_PHYC_LDO |= USB_HS_PHYC_LDO_ENABLE;
    auto count = 0UL;
    while ((USB_HS_PHYC->USB_HS_PHYC_LDO & USB_HS_PHYC_LDO_STATUS) == 0U)
    {
        count++;
    }
    USB_HS_PHYC->USB_HS_PHYC_TUNE |= 0x00000F13U;

    /* Enable PLL internal PHY */
    USB_HS_PHYC->USB_HS_PHYC_PLL |= USB_HS_PHYC_PLL_PLLEN;

    USB_OTG_HS->GUSBCFG |= USB_OTG_GUSBCFG_ULPIEVBUSD;

    // Force device mode
    USB_OTG_HS->GUSBCFG &= ~USB_OTG_GUSBCFG_FHMOD;
    USB_OTG_HS->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;
}

inline void initializeAdc()
{
    {
        using Connector = GpioConnector<Peripheral::Adc1, HK_1_1V::In1, HK_2_5V::In3, HK_3_3V::In0>;
        Connector::connect();

        Adc1::connect<GpioInputA0::In0>();
        Adc1::initialize<Board::SystemClock>();
        Adc1::setPinChannel<HK_1_1V>();
        Adc1::setPinChannel<HK_2_5V>();
        Adc1::setPinChannel<HK_3_3V>();
    }

    {
        using Connector = GpioConnector<Peripheral::Adc3, ADC_IN0::In14, ADC_IN1::In15,
            ADC_IN2::In4, ADC_IN3::In5, ADC_IN4::In6, ADC_IN5::In7>;
        Connector::connect();

        Adc3::connect<ADC_IN0::In14, ADC_IN1::In15, ADC_IN2::In4, ADC_IN3::In5, ADC_IN4::In6,
            ADC_IN5::In7>();
        Adc3::initialize<Board::SystemClock>();
    }
}

inline void initializeFPGA_CFG_if()
{
    // SpiMaster1::connect<FPGA_CFG_MISO::Miso, FPGA_CFG_MOSI::Mosi,
    // FPGA_CFG_SCK::Sck>();
    FPGA_CFG::SPI::connect<FPGA_CFG::SCK::BitBang, FPGA_CFG::MOSI::BitBang>();
    FPGA::initialize<Board::SystemClock, 20_MHz>();
    FPGA_CFG::DONE::setInput();
    FPGA_CFG::INIT::setInput();
    FPGA_CFG::PROGRAM::setOutput();
    FPGA_CFG::PROGRAM::set(1);
    FPGA_RESET::setOutput();
    FPGA_RESET::set(0);
    FPGA_HAVE_DATA::setInput();
}

inline void initializeFPGA_FMC_if()
{
    static constexpr auto AddressSetupTime = 0;
    static constexpr auto AddressHoldTime = 0;
    static constexpr auto DataSetupTime = 0;
    static constexpr auto BusTurnAroundDuration = 0;
    static constexpr auto CLKDivision = 3;
    static constexpr auto DataLatency = 2;
    static constexpr auto AccessMode = 0;

    using namespace FMC;
    Rcc::enable<Peripheral::Fmc>();
    GpioSet<D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18,
        D19, D20, D21, D22, D23, D24, D25, D26, D27, D28, D29, D30, D31, A0, A1, A2, CLK, NOE, NWE,
        NWAIT, NE1>::configure(Gpio::OutputType::PushPull, Gpio::OutputSpeed::VeryHigh);
    using Connector = GpioConnector<Peripheral::Fmc, D0::D0, D1::D1, D2::D2, D3::D3, D4::D4, D5::D5,
        D6::D6, D7::D7, D8::D8, D9::D9, D10::D10, D11::D11, D12::D12, D13::D13, D14::D14, D15::D15,
        D16::D16, D17::D17, D18::D18, D19::D19, D20::D20, D21::D21, D22::D22, D23::D23, D24::D24,
        D25::D25, D26::D26, D27::D27, D28::D28, D29::D29, D30::D30, D31::D31, A0::A0, A1::A1,
        A2::A2, CLK::Clk, NOE::Noe, NWE::Nwe, NWAIT::Nwait, NE1::Ne1>;
    Connector::connect();
    FMC_Bank1->BTCR[0] = (FMC_BCR1_MWID_1 | FMC_BCR1_MTYP_0 | FMC_BCR1_BURSTEN | FMC_BCR1_CCLKEN
        | FMC_BCR1_MBKEN | (1 << 7));
    FMC_Bank1->BTCR[1] = (AddressSetupTime << FMC_BTR1_ADDSET_Pos
        | AddressHoldTime << FMC_BTR1_ADDHLD_Pos | DataSetupTime << FMC_BTR1_DATAST_Pos
        | BusTurnAroundDuration << FMC_BTR1_BUSTURN_Pos | (CLKDivision - 1) << FMC_BTR1_CLKDIV_Pos
        | (DataLatency - 2) << FMC_BTR1_DATLAT_Pos | AccessMode << FMC_BTR1_ACCMOD_Pos);

    // Remap FMC bank 0 to uncached address space 0xA0000000 - 0xDFFFFFFF
    // (0xC0000000)
    // https://community.st.com/t5/stm32-mcus-products/known-issues-with-d-cache-and-fmc-on-stm32f7/m-p/276030/highlight/true#M63442

    SYSCFG->MEMRMP |= SYSCFG_MEMRMP_SWP_FMC_0;
}

inline void initializeSpiFlashMem()
{
    using namespace Flash;
    using namespace Flash::SpiPins;
    using Connector = GpioConnector<Peripheral::Spi4, Miso::Miso, Mosi::Mosi, Sck::Sck>;
    Flash::Cs::setOutput(Gpio::OutputType::PushPull, Gpio::OutputSpeed::High);
    Connector::connect();
    SpiMaster4::initialize<SystemClock, 42_MHz>();
    RF_CALL_BLOCKING(memory.initialize());
}


inline void initialize()
{
    SystemClock::enable();
    SysTickTimer::initialize<SystemClock>();
    SCB_EnableICache();
    SCB_EnableDCache();
    initializeFPGA_CFG_if();
    initializeFPGA_FMC_if();
    initializeAdc();
    initializeSpiFlashMem();

    stlink::Uart::connect<stlink::Tx::Tx, stlink::Rx::Rx>();
    stlink::Uart::initialize<SystemClock, 115200_Bd>();

    LED1::setOutput(modm::Gpio::Low);
    LED2::setOutput(modm::Gpio::Low);

    Button1::setInput();
    Button2::setInput();
}
} // namespace Board

#endif //
