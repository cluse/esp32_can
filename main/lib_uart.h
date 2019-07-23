

void sys_uart_open();

void sys_uart_close();

void sys_uart_send(char* buf);

int sys_uart_rx_len();

int sys_uart_rx_bytes(char* buf,int len);

void sys_print(char *str);
void sys_print2(char *str1,char *str2);

void sys_print_code(char *str,int code);

void output_version();


