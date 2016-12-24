#pragma once
#include "WaveIntf.h"

class FFWaveDecoderCreator : public tTVPWaveDecoderCreator
{
public:
    //VorbisWaveDecoderCreator() { TVPRegisterWaveDecoderCreator(this); }
    tTVPWaveDecoder * Create(const ttstr & storagename, const ttstr & extension);
};
