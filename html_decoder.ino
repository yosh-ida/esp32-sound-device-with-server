
const char encording[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', //  16
                    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                    ' ', '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/'};

void decordUrlString(String &str)
{
  char *s;
  s = (char*)malloc(sizeof(char) * str.length());
  int e = 0;
  for (int i = 0; i < str.length(); i++)
  {
    if (str[i] == '+')
    {
      str[e++] = ' ';
      continue;  
    }
    
    if (str[i] != '%')
    {
      str[e++] = str[i];
      continue;
    }
    
    char c = 0;
    if (i + 2 >= str.length())
      break;
      i++;
    if (str[i] >= '0' && str[i] <= '9')
      c += str[i] - '0';
    else if (str[i] >= 'A' && str[i] <= 'F')
      c += str[i] - 'A' + 10;
    c <<= 4;
    i++;
    if (str[i] >= '0' && str[i] <= '9')
      c += str[i] - '0';
    else if (str[i] >= 'A' && str[i] <= 'F')
      c += str[i] - 'A' + 10;
    str[e++] = encording[c];
  }
  str.remove(e);
}
