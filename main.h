typedef struct _client{
	struct in_addr ip;
	unsigned long incall;
	unsigned long incext;
	unsigned long incint;
	unsigned long outall;
	unsigned long outext;
	unsigned long outint;
} CLIENT;

typedef struct _traff{
	struct in_addr ipsrc;
	struct in_addr ipdst;
	unsigned long  bytes;
} TRAFF;

typedef struct _intnets{
	struct in_addr net;
	unsigned long mask;
} INTNETS;

typedef struct _element{
    void * pData;        //указатель на элемент списка
    void * pNextElement;    //указатель на следующий элемент списка
} ELEMENT;

typedef struct _list{
    ELEMENT * FirstElement;
	ELEMENT * LastElement;
	int counterElements;
} LIST;

void execElemtList(CLIENT * client, TRAFF * trafic);
void printElmt(void * buf, int type);
int addr_in_net(LIST * pList, struct in_addr addr);
