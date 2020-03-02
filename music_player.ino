#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioGeneratorAAC.h"
#include "AudioGeneratorFLAC.h"
#include "music_type.h"

AudioGenerator* audioGenerate(FILE_TYPE ft)
{
  switch(ft)
  {
    case FILE_TYPE_MP3: 
      return new AudioGeneratorMP3();
      break;
    case FILE_TYPE_WAV: 
      return new AudioGeneratorWAV();
      break;
    case FILE_TYPE_AAC:
      return new AudioGeneratorAAC();
    case FILE_TYPE_FLAC:
      return new AudioGeneratorFLAC();
    default:
      return NULL;
  }
}
