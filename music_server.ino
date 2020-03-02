#include <WiFi.h>
#include "freertos/task.h"
#include "FS.h"
#include "SD.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceID3.h"
#include "music_type.h"

//スレッドセーフにするには
//1.AudioGenerator,AudioFileSourceID3,AudioOutputI2Sは共通でいいが、AudioFileSourceSDは現在再生中のものと、次に再生するものとで分ける。
//切り替わりはグローバルなbool型変数で判断し、再生ループを行っている方がループの最初に切り替わりを判定して、Generatorを構築しなおす。
//また、outputI2Sをスレッドセーフなものにするため、音量の増減もboolで保存しておく。
//2.再生ループとwifi処理のループで分ける。
//こんな感じ
//仕様では、sdから4つまでファイルを開けるから、この方式の最大数(3)で問題ない

const int CS = 5;

const int UP = 2;
const int DOWN = 4;
const int RESET = 15;
const float volume[] = {0.0, 0.01, 0.05, 0.1, 0.2, 0.3};

bool up = false;
bool down = false;
bool reset = false;

IPAddress apIP(192, 168, 1, 1);
WiFiServer server(80);

int gain = 2;

const char htmlHeader[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>title</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"http://192.168.1.1/style.css\" />\r\n</head>\r\n<body>\r\n<iframe name=\"send\" src=\"http://192.168.1.1/send.html\" style=\"width:0px;height:0px;border:0px; border:none;\"></iframe>\r\n";
//const char htmlHeader[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>title</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"http://192.168.1.1/style.css\" />\r\n<script type=\"text/javascript\">\r\nfunction doNoting() {}\r\n</script></head>\r\n<body>\r\n";
const char style[] = ".submitbutton {\r\n\tbackground:none;\r\n\tborder:0;\r\n\tcolor :#1a1aff;\r\n}\r\n.submitbutton:hover {\r\n}";
const char sendHtml[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n</body></html>";

AudioGenerator* agene;
AudioFileSourceSD *currentFile, *nextFile;
AudioOutput* aOutput;
AudioFileSourceID3 *id3;

bool play = false;
bool next = false;  

FILE_TYPE ft = FILE_TYPE_NUM;
TaskHandle_t th;

void setup() 
{
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);
  pinMode(RESET, INPUT);

  aOutput = new AudioOutputI2S();
  aOutput->SetGain(volume[gain]);
  
  Serial.begin(9600);

  if(!SD.begin(CS))
  {
      Serial.println("Card Mount Failed");
      while (1) 
      {
        //  ticker
        delay(1);  
      }
  }

  xTaskCreatePinnedToCore(multiLoop, "loop", 8192, NULL, 1, &th, 0);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(100);
  WiFi.softAP("DNSServerExample", "12345678");
  delay(100);
  server.begin();
}

void multiLoop(void *param)
{
  while (1)
  {
    serverProcess();
    vTaskDelay(10);
  }
}

void loop()
{
  musicPlayer();
}

void musicPlayer()
{
  if (digitalRead(UP) == HIGH) 
  {
    if (!up) 
    {
      if (gain < 5)
        gain++;
      if (aOutput)
        aOutput->SetGain(volume[gain]);
      Serial.println("up to " + String(volume[gain]));
    }
    up = true;
  } 
  else
    up = false;
  
  if (digitalRead(DOWN) == HIGH) 
  {
    if (!down) 
    {
      if (gain > 0)
        gain--;
      if (aOutput)
        aOutput->SetGain(volume[gain]);
      Serial.println("down to " + String(volume[gain]));
    }
    down = true;
  } 
  else
    down = false;

  if (digitalRead(RESET) == HIGH)
  {
    if (!reset)
    {
      if (currentFile)
        currentFile->seek(0, SEEK_SET);
      Serial.println("reset");
    }
    reset = true;
  } 
  else
    reset = false;

  if (next)
  {
    delete currentFile;
    currentFile = nextFile;
    delete id3;
    id3 = new AudioFileSourceID3(currentFile);
    if (agene)
      delete agene;
    agene = audioGenerate(ft);
    agene->begin(id3, aOutput);
    next = false;
  }
  
  if (play && agene && agene->isRunning())
    if(!agene->loop())
    {
      agene->stop();
      Serial.println("MP3 end");
    }
  if (agene && !agene->isRunning())
    play = false;
}

void serverProcess()
{
  WiFiClient client = server.available();

  if (!client)
    return;
  
  String currentLine = "";
  while (client.connected()) 
  {
    if (!client.available())
      continue;
    char c = client.read();
    if (c == '\r')
      continue;
    else if (c != '\n') 
    {
      currentLine += c;
      continue;
    }

    Serial.println("HTTP : " + currentLine);  
    
    if (currentLine.startsWith("GET "))
    {
      currentLine = currentLine.substring(4, currentLine.indexOf(" ", 4));
      Serial.println("GET " + currentLine);
      httpGET(currentLine, client);
      client.stop();
      continue;
    }

    if (currentLine.startsWith("POST ")) 
    {
      currentLine = currentLine.substring(5, currentLine.indexOf(" ", 5));
      httpPOST(currentLine, client);
      client.stop();
      continue;
    }
    currentLine = "";
  }
}

void httpGET(String &str, WiFiClient &client)
{
  if (str == "/style.css")
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/css");
    client.println();
    client.println(style);
    return;
  }

  if (str == "/send.html")
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/css");
    client.println();
    client.println(sendHtml);
    return;
  }

  if (str[str.length() - 1] == '/' && str.length() > 2)
    str = str.substring(0, str.length() - 1);

  decordUrlString(str);

  //  str == GET url
  File root = SD.open(str);
   
  if (!root || !root.isDirectory())
  {
    Serial.println("Not directory or not found");
    client.println("HTTP/1.1 404 OK");
    client.println("Content-type:text/plain");
    client.println();
    client.print("Not Found");      
    return;
  }

  Serial.println("Directory found.");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.print(htmlHeader);

  //  ルートディレクトリでない場合は親へのリンクを作成
  if (str != "/")
    client.println("<a href=\"./\">./</a><br>");

  File file = root.openNextFile();
  while(file)
  {
      int p = 0;
      for (int i = 0; file.name()[i] != '\0'; i++)
        if (file.name()[i] == '/')
          p = i;
      if(file.isDirectory())
      {
        //  create link
        client.print("<a href=\"");
        client.print(file.name());
        client.print("\">");
        for (++p; file.name()[p] != '\0'; p++)
          client.print(file.name()[p]);
        client.println("</a><br>");
      }
      else
      {
        //  create post form
        client.print("<form method=\"post\" target=\"send\">\r\n<input type=\"submit\" value=\"");
        for (++p; file.name()[p] != '\0'; p++)
          client.print(file.name()[p]);
        client.print("\" class=\"submitbutton\"/>\r\n<input type=\"hidden\" name=\"play\" value=\"");
        client.print(file.name());
        client.println("\"/>\r\n</form>");
      }
      file = root.openNextFile();
  }

  client.println("</body>");
  client.println("</html>");
}

void httpPOST(String &str, WiFiClient &client)
{
  //  music player
  //if (str != "/play.php")
  //  return;

  String currentLine = "";
  while (client.connected()) 
  {
    if (!client.available())
      continue;
    char c = client.read();
    if (c == '\r')
      continue;
    else if (c != '\n') 
    {
      currentLine += c;
      continue;
    }

    Serial.println(currentLine);

    if (currentLine == "")
    {
      while (client.available())
      {
        char k = client.read();
        if (k == '\r' || k == '\n')
          break;
        currentLine += k;
      }
      Serial.println(currentLine);

      currentLine = currentLine.substring(currentLine.indexOf("=") + 1);
      //currentLine.replace("%2F", "/");
      decordUrlString(currentLine);
      Serial.println(currentLine);
      
      int i;
      for (i = 0; i < FILE_TYPE_NUM; i++)
      {
        if (!currentLine.endsWith(type[i]))
          continue;
        ft = (FILE_TYPE)i;
      }
      if (ft == FILE_TYPE_NUM)
      {
        Serial.println("invalid format.");
        next = false;
        break;  
      }
      if (nextFile != currentFile)
      {
        Serial.println("delete file reader.");
        delete nextFile;
      }
      nextFile = new AudioFileSourceSD(currentLine.c_str());
      if (!nextFile)
      {
        Serial.println(currentLine + " is not found.");
        break;
      }
      play = true;  
      next = true;
      break;
    }
    currentLine = "";
  }
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/hmtl");
  client.println("");
  client.println("<!DOCTYPE html>");
  client.println("<html>\r\n<head></head>");
  client.println("<body></body>");
  client.println("</html>");
}
