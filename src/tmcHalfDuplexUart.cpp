/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "tmcHalfDuplexUart.h"

#if defined(TMC2209_UART_HALFDUPLEX)

#include "picoPlatform.h"
#include <TMCStepper.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <hardware/uart.h>
#include <hardware/regs/uart.h>

// Half-duplex on GPIO13 (TMC_SS) only — expansion port pins are never touched.
//
// Keep FUNC=UART0 RX permanently so the FIFO stays armed. Bit-bang TX with pad
// OE/OUT overrides on the same pin: drive strong high/low during the host frame,
 // then force OE off (hi-Z + pull-up) so the TMC2209 can reply onto GPIO13.
class TmcHalfDuplexSerial : public Stream {
public:
    explicit TmcHalfDuplexSerial(pin_size_t pin)
        : _pin(pin), _baud(115200), _bitUs(9), _uartReady(false) {}

    void begin(unsigned long baud = 115200) {
        _baud = baud;
        _bitUs = (1000000UL + (baud / 2)) / baud;
        if (_bitUs < 1) {
            _bitUs = 1;
        }

        ensureUartReady();
        attachUartRx();
        releaseForRx();
    }

    void end() {
        releaseForRx();
        if (_uartReady) {
            while (uart_is_readable(_hwUart)) {
                (void)uart_getc(_hwUart);
            }
            uart_deinit(_hwUart);
            _uartReady = false;
        }
        gpio_set_outover(_pin, GPIO_OVERRIDE_NORMAL);
        gpio_set_oeover(_pin, GPIO_OVERRIDE_NORMAL);
        gpio_set_function(_pin, GPIO_FUNC_SIO);
        gpio_init(_pin);
        gpio_set_dir(_pin, GPIO_IN);
        gpio_pull_up(_pin);
    }

    // After a host datagram: stop driving and drop our TX echo from the FIFO.
    void releaseBus() {
        releaseForRx();
        while (uart_is_readable(_hwUart)) {
            (void)uart_getc(_hwUart);
        }
    }

    size_t write(uint8_t c) override {
        writeByte(c);
        return 1;
    }

    int available() override {
        if (!_uartReady) {
            return 0;
        }
        return uart_is_readable(_hwUart) ? 1 : 0;
    }

    int read() override {
        if (!_uartReady || !uart_is_readable(_hwUart)) {
            return -1;
        }
        return static_cast<int>(uart_getc(_hwUart));
    }

    int peek() override {
        return -1;
    }

    void flush() override {}

    int availableForWrite() override {
        return 1;
    }

private:
    uart_inst_t *_hwUart = uart0;
    pin_size_t _pin;
    unsigned long _baud;
    uint32_t _bitUs;
    bool _uartReady;

    void driveLow() {
        gpio_set_outover(_pin, GPIO_OVERRIDE_LOW);
        gpio_set_oeover(_pin, GPIO_OVERRIDE_HIGH);
    }

    void driveHigh() {
        // Strong push-pull high during the host frame (weak pad pull-up alone was
        // marginal; that is likely why a jumper onto another GPIO pad "helped").
        gpio_set_outover(_pin, GPIO_OVERRIDE_HIGH);
        gpio_set_oeover(_pin, GPIO_OVERRIDE_HIGH);
    }

    void releaseForRx() {
        // Hi-Z so the 2209 can drive the reply; keep the pad pull-up for idle.
        gpio_set_oeover(_pin, GPIO_OVERRIDE_LOW);
        gpio_set_outover(_pin, GPIO_OVERRIDE_NORMAL);
        gpio_pull_up(_pin);
    }

    void writeByte(uint8_t value) {
        absolute_time_t edge = get_absolute_time();

        auto waitBit = [&]() {
            edge = delayed_by_us(edge, _bitUs);
            busy_wait_until(edge);
        };

        // Start bit
        driveLow();
        waitBit();

        for (uint8_t i = 0; i < 8; i++) {
            if (value & 0x01u) {
                driveHigh();
            } else {
                driveLow();
            }
            waitBit();
            value >>= 1;
        }

        // Stop bit — still driven high; OE is released only after the full datagram.
        driveHigh();
        waitBit();
    }

    void ensureUartReady() {
        if (_uartReady) {
            return;
        }
        uart_init(_hwUart, _baud);
        uart_set_format(_hwUart, 8, 1, UART_PARITY_NONE);
        hw_clear_bits(&uart_get_hw(_hwUart)->cr, UART_UARTCR_TXE_BITS);
        _uartReady = true;
    }

    void attachUartRx() {
        gpio_init(_pin);
        gpio_set_dir(_pin, GPIO_IN);
        gpio_pull_up(_pin);
        gpio_set_input_enabled(_pin, true);
        gpio_set_function(_pin, GPIO_FUNC_UART);
        gpio_set_outover(_pin, GPIO_OVERRIDE_NORMAL);
        gpio_set_oeover(_pin, GPIO_OVERRIDE_LOW);

        // GPIO13<->PDN is a direct, resistor-less, unterminated single wire.
        // Slow/weak drive reduces ringing on our own TX edges that could
        // otherwise corrupt the TMC2209's reply riding the same node.
        gpio_set_slew_rate(_pin, GPIO_SLEW_RATE_SLOW);
        gpio_set_drive_strength(_pin, GPIO_DRIVE_STRENGTH_2MA);

        while (uart_is_readable(_hwUart)) {
            (void)uart_getc(_hwUart);
        }
    }
};

static TmcHalfDuplexSerial tmcUart(TMC_UART_RX);

Stream &tmcHalfDuplexStream() {
    return tmcUart;
}

void tmcHalfDuplexBegin(unsigned long baud) {
    tmcUart.begin(baud);
}

void tmcHalfDuplexEnd() {
    tmcUart.end();
}

void tmcHalfDuplexReleaseBus() {
    tmcUart.releaseBus();
}

class TmcTxTracker {
public:
    void reset() {
        _count = 0;
        _expected = 0;
    }

    void onByte(uint8_t data) {
        _count++;
        if (_count == 3) {
            _expected = (data & 0x80) ? 8 : 4;
        }
    }

    bool complete() const {
        return _expected != 0 && _count >= _expected;
    }

private:
    uint8_t _count = 0;
    uint8_t _expected = 0;
};

static TmcTxTracker g_txTracker;

int TMC2208Stepper::available() {
    // Never drain the FIFO here — that wiped pending TMC replies.
    g_txTracker.reset();

    int out = 0;
#if SW_CAPABLE_PLATFORM
    if (SWSerial != nullptr) {
        out = SWSerial->available();
    } else
#endif
    if (HWSerial != nullptr) {
        out = HWSerial->available();
    }

    return out;
}

uint8_t TMC2208Stepper::serial_write(const uint8_t data) {
    int out = 0;
#if SW_CAPABLE_PLATFORM
    if (SWSerial != nullptr) {
        out = SWSerial->write(data);
    } else
#endif
    if (HWSerial != nullptr) {
        if (sswitch != nullptr) {
            sswitch->active();
        }
        out = HWSerial->write(data);
    }

    g_txTracker.onByte(data);
    if (g_txTracker.complete()) {
        tmcUart.releaseBus();
        g_txTracker.reset();
    }

    return out;
}

void TMC2208Stepper::postWriteCommunication() {
    tmcUart.releaseBus();
    g_txTracker.reset();
}

#endif
