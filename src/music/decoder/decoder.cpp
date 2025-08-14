#include "decoder.h"

using namespace coloraudio::stream;
using namespace coloraudio::decoder;

Decoder::Decoder(BaseStream *st) : st(st)
{
}

Decoder::~Decoder()
{
}