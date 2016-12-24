#pragma once
#include "WaveIntf.h"

class VorbisWaveDecoderCreator : public tTVPWaveDecoderCreator
{
public:
    //VorbisWaveDecoderCreator() { TVPRegisterWaveDecoderCreator(this); }
    tTVPWaveDecoder * Create(const ttstr & storagename, const ttstr & extension);
};

class OpusWaveDecoderCreator : public tTVPWaveDecoderCreator
{
public:
    //VorbisWaveDecoderCreator() { TVPRegisterWaveDecoderCreator(this); }
    tTVPWaveDecoder * Create(const ttstr & storagename, const ttstr & extension);
};