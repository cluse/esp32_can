

int str_len(char *str);
int str_copy(char *dst,const char *src);
bool is_str_same(char *str1,const char *str2);
int index_of_char(char *str,char b);
int index_of_str(char *str1,const char *str2);

bool is_num_ascii(char ascii);
int index_of_num(char *str);
int index_of_no_num(char *str);

int ascii_hex_to_int(char ascii);
int ascii_dec_to_int(char ascii);
char char_to_ascii_hex(char val);
char char_to_ascii_dec(char val);

unsigned long hex_buf_to_long(char *buf);
unsigned long dec_buf_to_long(char *buf);
int long_to_hex_buf(char *buf,unsigned long val);
int long_to_dec_buf(char *buf,unsigned long val);

unsigned long char_to_long(char val);
char long_to_char(unsigned long val);
unsigned long int_to_long(int val);
int long_to_int(unsigned long val);

void can_data_copy(struct CAN_DATA *src,struct CAN_DATA *dst);
int can_data_to_buf(char *info,struct CAN_DATA *pCan);
int buf_to_can_data(char *info,struct CAN_DATA *pCan);


