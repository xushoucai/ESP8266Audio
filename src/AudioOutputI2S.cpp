/*
  AudioOutputI2S
  Base class for I2S interface port
  
  Copyright (C) 2017  Earle F. Philhower, III

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#ifdef ESP32
  #include "driver/i2s.h"
#else
  #include <i2s.h>
#endif
#include "AudioOutputI2S.h"

AudioOutputI2S::AudioOutputI2S(int port, bool builtInDAC)
{
  portNo = port;
  i2sOn = false;
#ifdef ESP32
  if (!i2sOn) {
    // don't use audio pll on buggy rev0 chips
    int use_apll = 0;
    esp_chip_info_t out_info;
    esp_chip_info(&out_info);
    if(out_info.revision > 0) {
      use_apll = 1;
    }
    i2s_config_t i2s_config_dac = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | (builtInDAC ? I2S_MODE_DAC_BUILT_IN : 0)),
      .sample_rate = 44100,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)((builtInDAC ? 0 : I2S_COMM_FORMAT_I2S) | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = 64,   //Interrupt level 1
      .use_apll = use_apll // Use audio PLL
    };
    Serial.printf("+%d %p\n", portNo, &i2s_config_dac);
    if (i2s_driver_install((i2s_port_t)portNo, &i2s_config_dac, 0, NULL) != ESP_OK) {
      Serial.println("ERROR: Unable to install I2S drives\n");
    }
    if (builtInDAC) {
      i2s_set_pin((i2s_port_t)portNo, NULL);
      i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    } else {
      SetPinout(26, 25, 22);
    }
    i2s_zero_dma_buffer((i2s_port_t)portNo);
  } 
#else
  if (!i2sOn) {
    i2s_begin();
  }
#endif
  i2sOn = true;
  mono = false;
  SetGain(1.0);
  this->builtInDAC = builtInDAC;
}

AudioOutputI2S::~AudioOutputI2S()
{
#ifdef ESP32
  if (i2sOn) {
    Serial.printf("UNINSTALL I2S\n");
    i2s_driver_uninstall((i2s_port_t)portNo); //stop & destroy i2s driver
  }	
#else
  if (i2sOn) i2s_end();
#endif
  i2sOn = false;
}

bool AudioOutputI2S::SetPinout(int bclk, int wclk, int dout)
{
#ifdef ESP32
  if (builtInDAC) return false; // Not allowed

  i2s_pin_config_t pins = {
    .bck_io_num = bclk,
    .ws_io_num = wclk,
    .data_out_num = dout,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin((i2s_port_t)portNo, &pins);
  return true;
#else
  (void) bclk;
  (void) wclk;
  (void) dout;
  return false;
#endif
}

bool AudioOutputI2S::SetRate(int hz)
{
  // TODO - have a list of allowable rates from constructor, check them
  this->hertz = hz;
#ifdef ESP32
  i2s_set_sample_rates((i2s_port_t)portNo, AdjustI2SRate(hz)); 
#else
  i2s_set_rate(AdjustI2SRate(hz));
#endif
  return true;
}

bool AudioOutputI2S::SetBitsPerSample(int bits)
{
  if ( (bits != 16) && (bits != 8) ) return false;
  this->bps = bits;
  return true;
}

bool AudioOutputI2S::SetChannels(int channels)
{
  if ( (channels < 1) || (channels > 2) ) return false;
  this->channels = channels;
  return true;
}

bool AudioOutputI2S::SetOutputModeMono(bool mono)
{
  this->mono = mono;
  return true;
}

bool AudioOutputI2S::begin()
{
  return true;
}

bool AudioOutputI2S::ConsumeSample(int16_t sample[2])
{
  MakeSampleStereo16( sample );

  if (this->mono) {
    // Average the two samples and overwrite
    uint32_t ttl = sample[LEFTCHANNEL] + sample[RIGHTCHANNEL];
    sample[LEFTCHANNEL] = sample[RIGHTCHANNEL] = (ttl>>1) & 0xffff;
  }
  uint32_t s32 = ((Amplify(sample[RIGHTCHANNEL]))<<16) | (Amplify(sample[LEFTCHANNEL]) & 0xffff);
#ifdef ESP32
  if (builtInDAC) {
    int16_t l = Amplify(sample[LEFTCHANNEL]) + 0x8000;
    int16_t r = Amplify(sample[RIGHTCHANNEL]) + 0x8000;
    s32 = (r<<16) | (l&0xffff);
  }
  return i2s_push_sample((i2s_port_t)portNo, (const char *)&s32, 0);
#else
  return i2s_write_sample_nb(s32); // If we can't store it, return false.  OTW true
#endif
}

bool AudioOutputI2S::stop()
{
#ifdef ESP32
  i2s_zero_dma_buffer((i2s_port_t)portNo);
#endif
  return true;
}


