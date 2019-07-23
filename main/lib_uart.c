

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

void sys_print(char *str)
{
    sys_uart_send(str);
    sys_uart_send("\n");
}

void sys_print2(char *str1,char *str2)
{
    sys_uart_send(str1);
    sys_uart_send(" ");
    sys_uart_send(str2);
    sys_uart_send("\n");
}

void sys_print_code(char *str,int code)
{
    char tmp[10];
    sys_uart_send(str);
    long_to_hex_buf(tmp,code);
    sys_uart_send(tmp);
    sys_uart_send("\n");
}

void output_version()
{
    sys_print(VERSION);
}

//--------------------------------------------
#if 0
#define SYS_RX_BUF_LEN 1024*2
#define SYS_ONE_LEN 64
int sys_in = 0;
int sys_out = 0;
char sys_rx_buf[SYS_RX_BUF_LEN];

int get_real_index(int index,int offset)
{
    index += offset;
    if (index >= SYS_RX_BUF_LEN)
        index -= SYS_RX_BUF_LEN;
    return index;
}

int get_send_bytes()
{
    if (sys_in >= sys_out)
        return sys_in - sys_out;
    else
        return SYS_RX_BUF_LEN - sys_out + sys_in;
}

void sys_rx_add_byte(char val)
{
    sys_rx_buf[sys_in] = val;
    sys_in = get_real_index(sys_in,1);
}

bool sys_rx_event()
{
    int len;
    len = get_send_bytes();
    if (len > SYS_ONE_LEN) {
        len = SYS_ONE_LEN;
    }
    if (len > 0) {
        if (uart_wait_tx_done(UART_NUM_0, 10/portTICK_RATE_MS) == ESP_OK) {
            uart_tx_chars(UART_NUM_0,buf,len);
        }
    }
    else {
        return false;
    }
}

//ret = uart_tx_chars(UART_NUM_0,buf,len);
//if (ret != len) {
//    printf("err-> uart_send %d %d \n",len,ret);
//}
#endif











