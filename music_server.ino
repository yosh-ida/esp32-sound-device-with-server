#include <WiFi.h>
#include "FS.h"
#include "SD.h"
//#include "SDControll"

IPAddress apIP(192, 168, 1, 1);
WiFiServer server(80);

const int CS = 5;

const char htmlHeader[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>title</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"http://192.168.1.1/style.css\" />\r\n</head>\r\n<body>\r\n";
//const char htmlHeader[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>title</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"http://192.168.1.1/style.css\" />\r\n<script type=\"text/javascript\">\r\nfunction doNoting() {}\r\n</script></head>\r\n<body>\r\n";
const char style[] = ".submitbutton {\r\n\tbackground:none;\r\n\tborder:0;\r\n\tcolor :#1a1aff;\r\n}\r\n.submitbutton:hover {\r\n}";

void setup() 
{

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
      //httpPOST();
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
        //client.print("<form method=\"post\" onsubmit=\"doNoting();return false;\"><input type=\"submit\" value=\"");
        client.print("<form method=\"post\"><input type=\"submit\" value=\"");
        for (++p; file.name()[p] != '\0'; p++)
          client.print(file.name()[p]);
        client.println("\" name=\"play\" class=\"submitbutton\"></form><br>");
        //Serial.print("  SIZE: ");
        //Serial.println(file.size());
      }
      file = root.openNextFile();
  }

  client.println("</body>");
  client.println("</html>");
}

void httpPOST(String &str, WiFiClient &client)
{

  //  music player
  if (str != "/play.php")
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

    if (currentLine.startsWith("play=")) 
    {
      break;
    } 
    else if (currentLine.startsWith("skip="))
    {
      break;
    }
    else if (currentLine.startsWith("back="))
    {
      break;
    } 
    else if (currentLine.startsWith("stop="))
    {
      break;
    }

  }

    
}
