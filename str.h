
static bool is_str_same(char *str1,const char *str2) {
  int i = 0;
  while(true) {
    if (str2[i] == 0)
      break;
    if (str1[i] != str2[i])
      return false;
    i++;
  }
  return true;
}

static int index_of_char(char* str,char b,int len)
{
  for (int i=0;i<len;i++) {
    if (str[i] == b)
      return i;
  }
  return -1;
}

static bool is_buf_same(char *str1,char *str2,char len)
{
  for (int i=0;i<len;i++) {
    if (str1[i] != str2[i])
      return false;
  }
  return true;
}

static bool is_hex_char(char ascii)
{
  char tmp;
  tmp = ascii;
  if (tmp >= '0' && tmp <= '9') {
    return true;
  } else if (tmp >= 'a' && tmp <= 'f') {
    return true;
  } else if (tmp >= 'A' && tmp <= 'F') {
    return true;
  }
  return false;
}

static char single_ascii_to_hex(char ascii)
{
  char tmp;
  tmp = ascii;
  if (tmp >= '0' && tmp <= '9') {
    tmp = tmp - '0';
  } else if (tmp >= 'a' && tmp <= 'f') {
    tmp = tmp - 'a' + 10;
  } else if (tmp >= 'A' && tmp <= 'F') {
    tmp = tmp - 'A' + 10;
  } else {
    tmp = 0;
  }
  return tmp;
}

static char ascii_to_hex(char *buf,int num)
{
  char ret = 0;
  char tmp;
  int i;
  for (i=0;i<num;i++) {
    tmp = single_ascii_to_hex(buf[i]);
    ret <<= 4;
    ret |= tmp;
  }
  return ret;
}

static char ascii_to_dec_char(char *buf,int num)
{
  char ret = 0;
  char tmp;
  int i;
  for (i=0;i<num;i++) {
    tmp = single_ascii_to_hex(buf[i]);
    ret *= 10;
    ret += tmp;
  }
  return ret;
}

static int ascii_to_dec_int(char *buf,int num)
{
  int ret = 0;
  char tmp;
  int i;
  for (i=0;i<num;i++) {
    tmp = single_ascii_to_hex(buf[i]);
    ret *= 10;
    ret += tmp;
  }
  return ret;
}

int char_to_int(char val)
{
  int tmp = val;
  return tmp&0xff;
}
