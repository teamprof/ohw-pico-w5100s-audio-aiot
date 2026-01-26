/* Copyright 2026 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <mbed.h>
#include <rtos.h>
#include <math.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

#include "i2s.h"
#include "i2s-32b.pio.h"

#include "audio_const.h"
#include "../pins.h"
#include "../ArduProfApp.h"

#define PICO_I2S_PIO 0 // select PIO instance for I2S, either 0 or 1
#define i2s_pio __CONCAT(pio, PICO_I2S_PIO)

namespace pioi2s
{
    const config_t i2s_config_default = {
        AUDIO_SAMPLING_RATE,
        32, // 32-bit per channel
        PIN_I2S_DI,
        PIN_I2S_BCLK,
    };

    static float pio_div(float freq, uint16_t *div, uint8_t *frac)
    {
        float clk = (float)clock_get_hz(clk_sys);
        float ratio = clk / freq;
        float d;
        float f = modff(ratio, &d);
        *div = (uint16_t)d;
        *frac = (uint8_t)(f * 256.0f);

        // Use post-converted values to get actual freq after any rounding
        float result = clk / ((float)*div + ((float)*frac / 256.0f));

        String sClk = String(clk, 2);
        String sResult = String(result, 2);
        LOG_TRACE("clk=", sClk.c_str(), ", freq=", freq, ", div=", *div, ", frac=", (uint32_t)(*frac), ", result=", sResult.c_str());

        return result;
    }

    static inline void calc_clocks_master(const config_t *config, clocks_t *clocks)
    {
        float bck_hz = config->fs * (float)config->bit_depth * 2.0f;
        clocks->bck_pio_hz = pio_div(bck_hz * (float)pio_i2s_master_in_program_mult, &clocks->bck_d, &clocks->bck_f);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // PIO initialization
    ///////////////////////////////////////////////////////////////////////////////
    static bool i2s_master_in_mono_left_init_pio(const config_t *config, pio_i2s_t *i2s)
    {
        i2s->sm_mask = 0;

        clocks_t clocks_master;
        calc_clocks_master(config, &clocks_master);
        LOG_TRACE("clocks_master.bck_d=", clocks_master.bck_d, ", clocks_master.bck_f=", clocks_master.bck_f);

        // Claim an unused state machine and add the program to a PIO instance.
        // The helper `pio_claim_free_sm_and_add_program` isn't available in this
        // build environment, so perform the steps explicitly using `pio0`.
        PIO pio = i2s_pio;
        uint sm = pio_claim_unused_sm(pio, true);
        uint offset = pio_add_program(pio, &i2s_master_in_mono_left_program);
        bool success = true;
        if (success)
        {
            i2s->pio = pio;
            i2s->sm_din = sm;
            i2s->offset_din = offset;
            i2s->sm_mask |= (1u << i2s->sm_din);
            i2s_master_in_mono_left_program_init(pio, sm, offset, config->bit_depth, config->din_pin, config->clock_pin_base);
            pio_sm_set_clkdiv_int_frac(pio, sm, clocks_master.bck_d, clocks_master.bck_f);
        }
        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // DMA initialization
    ///////////////////////////////////////////////////////////////////////////////
    static void i2s_init_dma_in(pio_i2s_t *i2s, void (*dma_handler)(void))
    {
        i2s->dma_ch_in_ctrl = dma_claim_unused_channel(true);
        i2s->dma_ch_in_data = dma_claim_unused_channel(true);

        i2s->dma_in_ctrl_blocks[0] = i2s->dma_in_buffer[0];
        i2s->dma_in_ctrl_blocks[1] = i2s->dma_in_buffer[1];

        dma_channel_config c = dma_channel_get_default_config(i2s->dma_ch_in_ctrl);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        channel_config_set_ring(&c, false, 3);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        dma_channel_configure(i2s->dma_ch_in_ctrl,
                              &c,
                              &dma_hw->ch[i2s->dma_ch_in_data].al2_write_addr_trig, // Destination pointer
                              i2s->dma_in_ctrl_blocks,                              // Source pointer
                              1,                                                    // Number of transfers
                              false                                                 // don't start
        );

        c = dma_channel_get_default_config(i2s->dma_ch_in_data);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, false);
        channel_config_set_write_increment(&c, true);
        channel_config_set_chain_to(&c, i2s->dma_ch_in_ctrl);
        channel_config_set_dreq(&c, pio_get_dreq(i2s->pio, i2s->sm_din, false));

        dma_channel_configure(i2s->dma_ch_in_data,
                              &c,
                              NULL,                        // Will be set by ctrl channel
                              &i2s->pio->rxf[i2s->sm_din], // Source pointer
                              DMA_BUFFER_SIZE,             // Number of transfers
                              false                        // don't start
        );

        // Input channel triggers the DMA interrupt handler, hopefully these stay
        // in perfect sync with the output.
        dma_channel_set_irq0_enabled(i2s->dma_ch_in_data, true);
        irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
        irq_set_enabled(DMA_IRQ_0, true);
    }

    static void i2s_master_in_init_dma(pio_i2s_t *i2s, void (*dma_handler)(void))
    {
        i2s_init_dma_in(i2s, dma_handler);

        // Enable dma channel
        uint32_t ch_mask = (1u << i2s->dma_ch_in_ctrl);
        dma_start_channel_mask(ch_mask);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool master_in_mono_left_start(const config_t *config, void (*dma_handler)(void), pio_i2s_t *i2s)
    {
        assert(!((uint32_t)(i2s->dma_in_ctrl_blocks) % 8) && !((uint32_t)(i2s->dma_in_buffer) % 8));

        // memset(i2s, 0, sizeof(*i2s));
        if (i2s_master_in_mono_left_init_pio(config, i2s))
        {
            i2s_master_in_init_dma(i2s, dma_handler);
            pio_enable_sm_mask_in_sync(i2s->pio, i2s->sm_mask);
            rtos::ThisThread::sleep_for(std::chrono::milliseconds(263)); // skip 2^18 clock cycles for INMP441 to be normal operation mode
            return true;
        }
        else
        {
            LOG_TRACE("i2s_master_in_mono_left_init_pio() failed");
            return false;
        }
    }

} // namespace pioi2s
