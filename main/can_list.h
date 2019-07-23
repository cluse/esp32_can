

void SysList_CoverSet(bool cover);

void SysList_TxInit();
void SysList_RxInit();

bool SysList_TxAdd(struct CAN_DATA *can);
bool SysList_RxAdd(struct CAN_DATA *can);

bool SysList_TxIsActive(int index);
bool SysList_RxIsActive(int index);

long SysList_TxRead(int index,struct CAN_DATA *can);
int SysList_RxRead(int index,struct CAN_DATA *can);

void SysList_TxDel(int index);
void SysList_RxDel(int index);

void SysList_TxDel_Id(int id);
void SysList_RxDel_Id(int id);

