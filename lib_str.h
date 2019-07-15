

//---------------------------
static int str_len(char *str)
{
  char *lp = str;
  while(*lp) {
    lp++;
  }
  return (lp - str);
}

static int str_copy(char *dst,const char *src)
{
  int len =0;
  while(*src != 0) {
    *dst++ = *src++;
    len++;
  }
  *dst = 0;
  return len;
}

static bool is_str_same(char *str1,const char *str2)
{
  while(*str2 != 0) {
    if (*str1++ != *str2++)
      return false;
  }
  return true;
}

static int index_of_char(char *str,char b)
{
  int len = str_len(str);
  for (int i=0;i<len;i++) {
    if (str[i] == b)
      return i;
  }
  return -1;
}

static int index_of_str(char *str1,const char *str2)
{
  int opt_len = str_len(str1) - str_len((char *)str2);
  int ret = -1;
  for (int i=0; i<opt_len; i++) {
    if (is_str_same(&str1[i],str2)) {
      ret = i;
      break;
    }
  }
  return ret;
}


//---------------------------
static bool is_num_ascii(char ascii)
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

static int index_of_num(char *str){
  char *lp = str;
  while(!is_num_ascii(*lp)) {
    lp++;
  }
  return (lp - str);
}

static int index_of_no_num(char *str){
  char *lp = str;
  while(is_num_ascii(*lp)) {
    lp++;
  }
  return (lp - str);
}

static int ascii_hex_to_int(char ascii)
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
  return (int)tmp;
}

static int ascii_dec_to_int(char ascii)
{
  char tmp;
  tmp = ascii;
  if (tmp >= '0' && tmp <= '9') {
    tmp = tmp - '0';
  } else {
    tmp = 0;
  }
  return (int)tmp;
}

static char char_to_ascii_hex(char val)
{
  if (val < 10) {
    return val + '0';
  } else if (val < 16) {
    return val - 10 + 'a';
  } else {
    return '0';
  }
}

static char char_to_ascii_dec(char val)
{
  if (val < 10) {
    return val + '0';
  } else if (val < 16) {
    return val - 10 + 'a';
  } else {
    return '0';
  }
}

//---------------------------
static unsigned long hex_buf_to_long(char *buf)
{
  unsigned long val = 0;
  while (is_num_ascii(*buf)) {
    val <<= 4;
    val += ascii_hex_to_int(*buf);
    buf++;
  }
  return val;
}

static unsigned long dec_buf_to_long(char *buf)
{
  unsigned long val = 0;
  while (is_num_ascii(*buf)) {
    val *= 10;
    val += ascii_dec_to_int(*buf);
    buf++;
  }
  return val;
}

static int long_to_hex_buf(char *buf,unsigned long val)
{
  int num = 0;
  while (val > 0 || num == 0) {
    char tmp = (val & 0xf);
    val >>= 4;
    buf[num++] = char_to_ascii_hex(tmp);
  }
  for (int i=0;i<num/2;i++) {
    char tmp = buf[i];
    buf[i] = buf[num - 1 - i];
    buf[num - 1 - i] = tmp;
  }
  return num;
}

static int long_to_dec_buf(char *buf,unsigned long val)
{
  int num = 0;
  while (val > 0 || num == 0) {
    char tmp = (val % 10);
    val /= 10;
    buf[num++] = char_to_ascii_hex(tmp);
  }
  for (int i=0;i<num/2;i++) {
    char tmp = buf[i];
    buf[i] = buf[num - 1 - i];
    buf[num - 1 - i] = tmp;
  }
  return num;
}

unsigned long char_to_long(char val)
{
  unsigned long tmp = val;
  return tmp&0xff;
}

char long_to_char(unsigned long val)
{
  return (char)(val&0xff);
}

unsigned long int_to_long(int val)
{
  unsigned long tmp = val;
  return tmp&0xffff;
}

int long_to_int(unsigned long val)
{
  return (int)(val&0xffff);
}


//---------------------------
static void can_data_copy(struct CAN_DATA *src,struct CAN_DATA *dst)
{
  dst->id = src->id;
  dst->len = src->len;
  for (int i=0;i<src->len;i++) {
    dst->buf[i] = src->buf[i];
  }
  //dst->tm = src->tm;
}

static int can_data_to_buf(char *info,struct CAN_DATA *pCan,bool full)
{
  char *lp = info;
  int num;
  num = long_to_hex_buf(lp,int_to_long(pCan->id));
  lp += num;
  *lp++ = ',';
  num = long_to_dec_buf(lp,int_to_long(pCan->tm));
  lp += num;
  *lp++ = ',';
  if (full) {
    num = long_to_dec_buf(lp,char_to_long(pCan->len));
    lp += num;
    for(int i=0;i<CAN_FRAME_DATA_MAX && i<pCan->len; i++) {
      *lp++ = ',';
      num = long_to_hex_buf(lp,char_to_long(pCan->buf[i]));
      lp += num;
    }
    *lp++ = 0;
  }
  return (lp - info);
}

const char can_fix_len = CAN_FRAME_DATA_MAX;
const char can_fix_buf[CAN_FRAME_DATA_MAX] = {8,7,6,5,4,3,2,1};
static int buf_to_can_data(char *info,struct CAN_DATA *pCan,bool full)
{
  char *lp = info;
  char index;
  unsigned long tmp;
  pCan->id = hex_buf_to_long(lp);
  index = index_of_no_num(lp);
  lp += (index + 1);
  pCan->tm = dec_buf_to_long(lp);
  index = index_of_no_num(lp);
  lp += (index + 1);

  if (is_num_ascii(*lp)) {
    tmp = dec_buf_to_long(lp);
    pCan->len = long_to_char(tmp);
    for(int i=0;i<CAN_FRAME_DATA_MAX && i<pCan->len; i++) {
      index = index_of_no_num(lp);
      lp += (index + 1);
      tmp = hex_buf_to_long(lp);
      pCan->buf[i] = long_to_char(tmp);
    }
  }
  else {
    pCan->len = can_fix_len;
    for (int i=0; i<can_fix_len; i++) {
      pCan->buf[i] = can_fix_buf[i];
    }
  }
  return 0;
}
