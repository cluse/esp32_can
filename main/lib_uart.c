

#include "def.h"
#include "lib_str.h"

#include "driver/gpio.h"
#include "driver/uart.h"

//void sys_rx_add_byte(char val);
//#define TX_GPIO_NUM GPIO_NUM_21
//#define RX_GPIO_NUM GPIO_NUM_22

#define SYS_UART_BUF_LEN 1024
void sys_uart_open()
{
    uart_config_t uart_config = {        
        .baud_rate = 115200,        
        .data_bits = UART_DATA_8_BITS,        
        .parity    = UART_PARITY_DISABLE,        
        .stop_bits = UART_STOP_BITS_1,        
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE    };    
        
    uart_param_config(UART_NUM_0, &uart_config);    
    uart_driver_install(UART_NUM_0, SYS_UART_BUF_LEN, SYS_UART_BUF_LEN, 0, NULL, 0);
}

void sys_uart_close()
{
    uart_driver_delete(UART_NUM_0);
}

void sys_uart_send(char* buf)
{
    int len,ret;
    len = str_len(buf);
    ret = uart_write_bytes(UART_NUM_0,buf,len);
    if (ret != len) {
        printf("err-> uart_send %d %d \n",len,ret);
    }
    //for (i=0;i<len;i++)
    //    sys_rx_add_byte(buf[i]);
}

int sys_uart_rx_len()
{
    int len;
    if (uart_get_buffered_data_len(UART_NUM_0,(size_t*)(&len)) == ESP_OK) {
        return len;
    }
    return 0;
}

int sys_uart_rx_bytes(char* buf,int len)
{
    int ret;
    ret = uart_read_bytes(UART_NUM_0,(uint8_t*)buf,len,0);
    return ret;
}

static char print_buf1[100];
static char print_buf2[100];
static char print_buf3[100];
void sys_print(char *str)
{
    int len;
    char *lp = print_buf1;
    len = str_copy(str,lp);
    lp[len++] = '\n';
    lp[len] = 0;
    sys_uart_send(lp);
}

void sys_print2(char *str1,char *str2)
{
    int len;
    char *lp = print_buf2;
    len = str_copy(str1,lp);
    lp[len++] = ' ';
    str_copy(str2,&lp[len]);
    len = str_len(lp);
    lp[len++] = '\n';
    lp[len] = 0;
    sys_uart_send(lp);
}

void sys_print_code(char *str,int code)
{
    int len;
    char *lp = print_buf3;
    char tmp[10];
    long_to_hex_buf(tmp,code);
    len = str_copy(str,lp);
    str_copy(tmp,&lp[len]);
    len = str_len(lp);
    lp[len++] = '\n';
    lp[len] = 0;
    sys_uart_send(lp);
}

void output_version()
{
    sys_print(VERSION);
}




