/*
  AudioOutputI2S
  Base class for an I2S output port
  
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

#ifndef _AUDIOOUTPUTI2S_H
#define _AUDIOOUTPUTI2S_H

#include "AudioOutput.h"

class AudioOutputI2S : public AudioOutput
{
  public:
    AudioOutputI2S(int port=0, bool builtInDAC=false);
    virtual ~AudioOutputI2S() override;
    bool SetPinout(int bclkPin, int wclkPin, int doutPin);
    virtual bool SetRate(int hz) override;
    virtual bool SetBitsPerSample(int bits) override;
    virtual bool SetChannels(int channels) override;
    virtual bool begin() override;
    virtual bool ConsumeSample(int16_t sample[2]) override;
    virtual bool stop() override;
    
    bool SetOutputModeMono(bool mono);  // Force mono output no matter the input
    
  protected:
    virtual int AdjustI2SRate(int hz) { return hz; }
    uint8_t portNo;
    bool builtInDAC;
    bool mono;
    bool i2sOn;
};

#endif

