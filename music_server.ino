#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceID3.h"

IPAddress apIP(192, 168, 1, 1);
WiFiServer server(80);

const int CS = 5;

const int UP = 2;
const int DOWN = 4;
const int RESET = 15;
bool up = false;
bool down = false;
bool reset = false;

const float volume[] = {0.0, 0.01, 0.05, 0.1, 0.2, 0.3};
int gain = 2;

const char htmlHeader[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>title</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"http://192.168.1.1/style.css\" />\r\n</head>\r\n<body>\r\n<iframe name=\"send\" src=\"http://192.168.1.1/send.html\" style=\"width:0px;height:0px;border:0px; border:none;\"></iframe>\r\n";
//const char htmlHeader[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>title</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"http://192.168.1.1/style.css\" />\r\n<script type=\"text/javascript\">\r\nfunction doNoting() {}\r\n</script></head>\r\n<body>\r\n";
const char style[] = ".submitbutton {\r\n\tbackground:none;\r\n\tborder:0;\r\n\tcolor :#1a1aff;\r\n}\r\n.submitbutton:hover {\r\n}";
const char sendHtml[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n</body></html>";

AudioGenerator* agene;
AudioFileSourceSD* audioFile;
AudioOutput* aOutput;
AudioFileSourceID3 *id3;

bool play = false;

void setup() 
{
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);
  pinMode(RESET, INPUT);
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
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(100);
  WiFi.softAP("DNSServerExample", "12345678");
  delay(100);
  server.begin();
}

void loop() 
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
      if (audioFile)
        audioFile->seek(0, SEEK_SET);
      Serial.println("reset");
    }
    reset = true;
  } 
  else
    reset = false;
  
  if (play && agene && agene->isRunning()) 
    if(!agene->loop())
    {
      agene->stop();
      Serial.println("MP3 end");
    }
  
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
      currentLine.replace("%2F", "/");
      
      delete agene;    
      if (currentLine.endsWith(".mp3"))
        agene = new AudioGeneratorMP3();
      else if (currentLine.endsWith(".wav"))
        agene = new AudioGeneratorWAV();
      else
      {
        Serial.println("invalid format.");
        break;  
      }
      
      delete audioFile;
      audioFile = new AudioFileSourceSD(currentLine.c_str());
      if (!audioFile)
      {
        Serial.println(currentLine + " is not found.");
        break;
      }
      delete id3;
      id3 = new AudioFileSourceID3(audioFile);
      delete aOutput;
      aOutput = new AudioOutputI2S();
      aOutput->SetGain(volume[gain]);
      
      agene->begin(id3, aOutput);
      play = true;
         
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
