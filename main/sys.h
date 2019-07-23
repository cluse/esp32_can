

void Sys_Init();

void com_event();

void output_can_rx(struct CAN_DATA *can);

void output_can_monitor(struct CAN_DATA *can,int num);

void tx_process(struct CAN_DATA *pCan,long tm);

void rx_process(struct CAN_DATA *pCan,long tm);

void monitor_ss_process(long tm);


