//#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
//#include "SPIFFS.h"
#include "AudioFileSourceID3.h"

AudioGeneratorMP3* pMp3;
AudioFileSourceSD* pFile;
AudioOutput* pOutput;
AudioFileSourceID3 *id3;
bool up = false;
bool down = false;

bool reset = false;

const int UP = 2;
const int DOWN = 4;
const int RESET = 15;

//  sdカードに保存するファイル名は0,1,2,...の連番にする
//  index.txtとかに、インデックスと登録名のテーブルを作っておく
//  

const float volume[] = {0.0, 0.01, 0.05, 0.1, 0.2, 0.3};
int gain = 2;
 
void setup() {
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);
  pinMode(RESET, INPUT);
  Serial.begin(115200);

  //SPIFFS.begin();
  SD.begin(5);
  pFile = new AudioFileSourceSD("/sound.mp3" );
  if (!pFile->isOpen()) {
    while (1) {
      Serial.println("Not found.");
      delay(1000);
    }
  } 
  Serial.println("File found.");
  id3 = new AudioFileSourceID3(pFile);

  //out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  //out->SetOutputModeMono(true);
  pOutput = new AudioOutputI2S();
  pOutput->SetGain(volume[gain]);
  pMp3 = new AudioGeneratorMP3();

  pMp3->begin(id3, pOutput);

  //File file = SD.open("/");
  //file.openNextFile()
}

void loop() {
  if (digitalRead(UP) == HIGH) {
    if (!up) {
      if (gain < 5)
        gain++;
      pOutput->SetGain(volume[gain]);
      Serial.println("up to " + String(volume[gain]));
    }
    up = true;
  } else {
    up = false;
  }
  
  if (digitalRead(DOWN) == HIGH) {
    if (!down) {
      if (gain > 0)
        gain--;
      pOutput->SetGain(volume[gain]);
      Serial.println("down to " + String(volume[gain]));
    }
    down = true;
  } else {
    down = false;
  }

  if (digitalRead(RESET) == HIGH) {
    if (!reset) {
      pFile->seek(0, SEEK_SET);
      Serial.println("reset");
    }
    reset = true;
  } else {
    reset = false;
  }
  
  if( pMp3->isRunning() )
  {
    //  曲が終わってないなら再生処理
    if( !pMp3->loop() )
    {
      pMp3->stop();
      Serial.println("MP3 end");
    }
  } else {
    delay(100);
  }
}

