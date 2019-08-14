

void SysList_CoverSet(bool cover);

void SysList_TxInit();
void SysList_RxInit();

int SysList_TxAdd(struct CAN_DATA *can);
int SysList_RxAdd(struct CAN_DATA *can);

bool SysList_TxIsActive(int index);
bool SysList_RxIsActive(int index);

void SysList_TxUpdateTag(int index,long tm);
void SysList_TxSetNum(int index,int num);
void SysList_TxCutNum(int index,int num);
long SysList_TxRead(int index,struct CAN_DATA *can);
int SysList_RxRead(int index,struct CAN_DATA *can);

void SysList_TxDel(int index);
void SysList_RxDel(int index);

void SysList_TxDel_Id(int id);
void SysList_RxDel_Id(int id);

