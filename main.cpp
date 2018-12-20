#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "libpq-fe.h"
#include "main.h"

#define FNAME5MINLOG "/usr/local/script/ng_stat/log/ng5min.log"
#define FNAMEINTNETS "/usr/local/script/ng_stat/intnets"
#define CONINFO "host=127.0.0.1 port=5432 dbname=iptrafic user=dba password=sql"

PGconn * conn;
PGresult * res;

LIST * initList(){
    LIST * pList = NULL;
    pList = (LIST*)calloc(1, sizeof(LIST));
	pList->counterElements = 0;
    return (pList);
}

//добавляем элемент в конец списка
int pushList(LIST * pList, void * buf){
    if(pList->counterElements == 0){
        ELEMENT * pElmt = (ELEMENT*)calloc(1, sizeof(ELEMENT));
        pElmt->pData = buf;
        pElmt->pNextElement = NULL;
        pList->FirstElement = pElmt;
        pList->LastElement = pElmt;
        pList->counterElements++;
        return 0;
    }
    if(pList->counterElements >= 1){
        ELEMENT * pElmt = (ELEMENT*)calloc(1, sizeof(ELEMENT));
        pElmt->pData = buf;
        pElmt->pNextElement = NULL;
        pList->LastElement->pNextElement = (void*)pElmt;
        pList->LastElement = pElmt ;
        pList->counterElements++;
        return 0;
    }
    return (0);
}

//вынимаем элемент с начала списка
void * popList(LIST * pList){
	if(pList->FirstElement != NULL){
		void * buf;
		buf = pList->FirstElement->pData;
		ELEMENT * pCurElmt = pList->FirstElement;
		if(pCurElmt->pNextElement != NULL){ //если НЕ единственный элемент в списке
			pList->FirstElement = (ELEMENT*)pCurElmt->pNextElement;
			free(pCurElmt);     //освобождаем память в которой хранился указатель
			pList->counterElements--;
		}else{
			pList->FirstElement->pNextElement = NULL;
			free(pList->FirstElement); 
			pList->FirstElement = NULL;
			free(pList->LastElement);
			pList->LastElement = NULL;
			pList->counterElements--;
		} 
    	return buf;
	}
	return NULL;
}

void printList(LIST * pList, int type){
	if(pList->counterElements > 0){
        ELEMENT * pCurElmt = pList->FirstElement;
		for(int i=0; i < pList->counterElements; i++){
			printElmt(pCurElmt->pData,type);
			printf("\n");
            pCurElmt = (ELEMENT*)pCurElmt->pNextElement;
		}
		printf("\nколичество элементов списка: %d\n",pList->counterElements);
	}else{
		printf("\nколичество элементов списка: %d\n",pList->counterElements);
	}
    return;    
}

void printElmt(void * buf, int type){
	if(type==1){
            CLIENT * c = (CLIENT*)buf ;
            printf("ip=%-15s",inet_ntoa(c->ip));
            printf("incall=%-15u",c->incall);
            printf("incext=%-15u",c->incext);
            printf("incint=%-15u",c->incint);
            printf("outall=%-15u",c->outall);
            printf("outext=%-15u",c->outext);
            printf("outint=%-15u",c->outint);
	}
	if(type==2){
		TRAFF * t = (TRAFF*)buf ;
		printf("ipsrc=%-15s\t",inet_ntoa(t->ipsrc));
		printf("ipdst=%-15s\t",inet_ntoa(t->ipdst));
		printf("размер пакета, байт=%-5d",t->bytes);
	}
	if(type==3){
		INTNETS * n = (INTNETS*)buf ;
		printf("net=%-15s\t",inet_ntoa(n->net));
		struct in_addr a;
		a.s_addr = n->mask;
		printf("mask=%-15s\t",inet_ntoa(a));
	}
	return;
}

//находит все элементы равные buf
//производит действие над элементом и удаляет его из списока
void removeListElemt(LIST * pList/*список с траффиком*/,LIST * pIntNEts, void * buf/*один клиент из таблицы клиенты*/){
	ELEMENT * pCurElmt = pList->FirstElement;
	ELEMENT * pPreElmt = pCurElmt;
	int x = 0;
	for(;;){
		//printf("%s",pCurElmt->pData);
		if(pList->counterElements == 0)	break;

		CLIENT * c = (CLIENT*)buf ;
		TRAFF * t = (TRAFF*)pCurElmt->pData ;
//=======================================================================================
//подсчет трафика
//=======================================================================================
		if((t->ipdst.s_addr == c->ip.s_addr)||(t->ipsrc.s_addr == c->ip.s_addr)){	
			//обработать данные
			if(t->ipdst.s_addr == c->ip.s_addr){  //подсчет входящего трафика
				c->incall = c->incall + t->bytes;//общий входящий трафик
				if(addr_in_net(pIntNEts,t->ipsrc)==1){//если трафик пришел из внутренней сети
					c->incint = c->incint + t->bytes;
				}else{
					c->incext = c->incext + t->bytes;//если трафик пришел из внешней сети
				}
			}
			if(t->ipsrc.s_addr == c->ip.s_addr){  //подсчет исходящего трафика
				c->outall = c->outall + t->bytes;
				if(addr_in_net(pIntNEts,t->ipdst)==1){
					c->outint = c->outint + t->bytes;//если трафик ушел во внутреннюю сеть
				}else{
					c->outext = c->outext + t->bytes;//если трафик ушел во внешнюю сеть
				}
			}
			//printf("trf=%d, sum=%d\n",t->bytes,c->incall);
			/*if(pCurElmt == pList->FirstElement){
				//execElemtList((CLIENT*)buf,(TRAFF*)pCurElmt->pData);
				//free(pCurElmt); pCurElmt == NULL;
				pList->FirstElement = (ELEMENT*)pList->FirstElement->pNextElement;
				pList->counterElements--;
			}else{
				//execElemtList((CLIENT*)buf,(TRAFF*)pCurElmt->pData);
				pPreElmt->pNextElement = pCurElmt->pNextElement;
				//free(pCurElmt); pCurElmt == NULL;
				pList->counterElements--;
			}*/
		}
//=======================================================================================
		if(x==1111) break;
		if(pList->counterElements == 1) break;
		pPreElmt = pCurElmt;
		pCurElmt = (ELEMENT *)pCurElmt->pNextElement;
		if(pList->counterElements == 0)	break;
		if(pCurElmt->pNextElement == NULL){
			x = 1111;
			continue;
		}
	}
	return;
}

int addr_in_net(LIST * pList, struct in_addr addr){
	ELEMENT * pCurElmt = pList->FirstElement;
	int x = 0;
	for(;;){
		if(pList->counterElements == 0) break;
		
		INTNETS * n = (INTNETS*)pCurElmt->pData;
		
		unsigned long net_addr = 0;
		net_addr = addr.s_addr & n->mask;
		struct in_addr a;
		a.s_addr = net_addr;

		struct in_addr netmask;
		netmask.s_addr = n->mask;
		//printf("адр сети=%s,",inet_ntoa(n->net));
		//printf("мск сети=%s,",inet_ntoa(netmask));
		//printf("вх IP=%s,",inet_ntoa(addr));
		//printf("мск сети вх IP=%s",inet_ntoa(a));
		//printf("\n");
		
		if(n->net.s_addr == net_addr){
			//printf("адр сети=%s,",inet_ntoa(n->net));
			//printf("мск сети=%s,",inet_ntoa(netmask));
			//printf("вх IP=%s,",inet_ntoa(addr));
			//printf("мск сети вх IP=%s",inet_ntoa(a));
			//printf("\n");
			return 1;
		}
		
		if(x==1111) break;
		pCurElmt = (ELEMENT *)pCurElmt->pNextElement;
		if(pCurElmt->pNextElement == NULL){ 
			x = 1111; 
			continue; 
		}
	}
	return 0;
}

void execElemtList(CLIENT * client, TRAFF * trafic){
	printElmt((void*)client,1);
	printf("\n");
	printElmt((void*)trafic,2);
	printf("\n");
	return;
}

void calculateMonthTrafic(LIST * clients, LIST * traf5min, LIST * IntNEts, LIST * trafmonthOld){
	ELEMENT * pCurClient = clients->FirstElement;
	int mark = 0;
//=======================================================================================
//результатом работы этого цикла 
//является список клиентов с заполненным 5-и минутным траффиком.
	for(;;){
		if(clients->counterElements == 0) break;
		//printf("%-32X\n",pCurClient->pData);
		removeListElemt(traf5min,IntNEts,pCurClient->pData);
		
		if(mark == 1111) break;
		if(clients->counterElements == 1) break;
		pCurClient = (ELEMENT *)pCurClient->pNextElement;
		if(pCurClient->pNextElement == NULL){
			mark = 1111;
			continue;
		}
	}
//=======================================================================================
//результат работы этого цикла - вычисленный месячный трафик клиентов
	ELEMENT * pOldTraf = trafmonthOld->FirstElement;
	mark = 0;
	for(;;){
		if(trafmonthOld->counterElements == 0) break;
		
		CLIENT * pOldClientTraf = (CLIENT*)pOldTraf->pData;
		//if((pClient->incall == 0)&&(pClient->incext == 0)&&(pClient->incint == 0)&&
		//   (pClient->outall == 0)&&(pClient->outext == 0)&&(pClient->outint == 0)){
		   	//не делаем ничего
		//}else{
			ELEMENT * pCurClientTraf = clients->FirstElement;
			int mark1 = 0;
			for(;;){
				if(clients->counterElements == 0) break;
				
				CLIENT * pCurTraf = (CLIENT*)pCurClientTraf->pData;
				
				if(pOldClientTraf->ip.s_addr == pCurTraf->ip.s_addr){
					pOldClientTraf->incall = pOldClientTraf->incall + pCurTraf->incall ; 
					pOldClientTraf->incext = pOldClientTraf->incext + pCurTraf->incext ;
					pOldClientTraf->incint = pOldClientTraf->incint + pCurTraf->incint ;
					pOldClientTraf->outall = pOldClientTraf->outall + pCurTraf->outall ;
					pOldClientTraf->outext = pOldClientTraf->outext + pCurTraf->outext ;
					pOldClientTraf->outint = pOldClientTraf->outint + pCurTraf->outint ;
				}
				
				if(mark1 == 1111) break;
				if(clients->counterElements == 1) break;
				pCurClientTraf = (ELEMENT *)pCurClientTraf->pNextElement;
				if(pCurClientTraf->pNextElement == NULL){
					mark1 = 1111;
					continue;
				}
			}
		//}
		
		if(mark == 1111) break;
		if(trafmonthOld->counterElements == 1) break;
		pOldTraf = (ELEMENT *)pOldTraf->pNextElement;
		if(pOldTraf->pNextElement == NULL){
			mark = 1111;
			continue;
		}
	}
//=======================================================================================
	return;
}

//=======================================================================================
//эта функция заносит результат вычисления трафика из списка с месячным трафиком в БД 
void updateMonthTrafINDatabase(LIST * pList/*список с вычисленным месячным трафиком клиентов*/){
	conn = PQconnectdb(CONINFO);
	if (PQstatus(conn) == CONNECTION_OK){
		res = PQexec(conn, "BEGIN");
	    if (PQresultStatus(res) != PGRES_COMMAND_OK){
			fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		PQclear(res);
		
		ELEMENT * pCurElmt = pList->FirstElement;
		int x = 0;
		for(;;){
			if(pList->counterElements == 0) break;

			CLIENT * pClientMontfTraf = (CLIENT*)pCurElmt->pData;
			if((pClientMontfTraf->incall == 0)&&(pClientMontfTraf->incext == 0)&&(pClientMontfTraf->incint == 0)&&
			   (pClientMontfTraf->outall == 0)&&(pClientMontfTraf->outext == 0)&&(pClientMontfTraf->outint == 0)){
				//не делаем ничего
			}else{
				char updtreq[256];updtreq[0]=0;
				strcpy(updtreq,"UPDATE trafmnf SET inctraf=");
				char buf[128];
				sprintf(buf,"%u,",pClientMontfTraf->incall);
				strcat(updtreq,buf);
				sprintf(buf,"outtraf=%u,",pClientMontfTraf->outall);
				strcat(updtreq,buf);
				sprintf(buf,"extinctrf=%u,",pClientMontfTraf->incext);
				strcat(updtreq,buf);
				sprintf(buf,"extouttrf=%u,",pClientMontfTraf->outext);
				strcat(updtreq,buf);
				sprintf(buf,"intinctrf=%u,",pClientMontfTraf->incint);
				strcat(updtreq,buf);
				sprintf(buf,"intouttrf=%u",pClientMontfTraf->outint);
				strcat(updtreq,buf);
				strcat(updtreq," WHERE ip='");
				sprintf(buf,"%s' ;",inet_ntoa(pClientMontfTraf->ip));
				strcat(updtreq,buf);

				printf("%s\n",updtreq);
				
				res = PQexec(conn, updtreq);
				if (PQresultStatus(res) != PGRES_COMMAND_OK){
					fprintf(stderr, "UPDATE failed: %s", PQerrorMessage(conn));
					PQclear(res);
				}
				PQclear(res);
			}
			
			if(x==1111) break;
			pCurElmt = (ELEMENT *)pCurElmt->pNextElement;
			if(pCurElmt->pNextElement == NULL){ 
				x = 1111; 
				continue; 
			}
		}

		res = PQexec(conn, "END");
		PQclear(res);
//=======================================================================================
	}else{
		fprintf(stderr,"Не удалось соединиться с базой данных: %s\n",PQerrorMessage(conn));
	}
	PQfinish(conn);
	return;
}

int main(){

//=======================================================================================
//создаем список с страфиком за последние 5 минут
    FILE * f = fopen(FNAME5MINLOG, "r");
    if(f==NULL){
        fprintf(stderr,"ERROR, file %s not open\n",FNAME5MINLOG);
        return -1;
    }
    LIST * l_traf5min = initList();
    char buf[128];
    while(!feof(f)){
        fgets(buf,127,f);
		if(strlen(buf) != 0){
			TRAFF * pBuf = (TRAFF*)malloc(1 * sizeof(TRAFF));
			//strcpy(pBuf,buf);
			char * b;
			b = strtok(buf,";");
			struct in_addr addr;
			inet_aton(b, &addr);
			pBuf->ipsrc = addr;
			
			b = strtok(NULL,";");
			b = strtok(NULL, ";");
			inet_aton(b, &addr);
			pBuf->ipdst = addr;
			
			b =strtok(NULL,";");
			b =strtok(NULL,";");
			b =strtok(NULL,";");
			b =strtok(NULL,";");
			pBuf->bytes = atoi(b);
			//printf("%d\n",atoi(b));
			
			pushList(l_traf5min,(void*)pBuf);
		}
        buf[0]=0;
    }
    fclose(f);
//=======================================================================================
//создаем список собственных сетей ЮТК
	f = fopen(FNAMEINTNETS, "r");
	if(f==NULL){
		fprintf(stderr,"ERROR, file %s not open\n",FNAMEINTNETS);
		return -1;
	}
	LIST * l_intnets = initList();
	while(!feof(f)){
		fgets(buf,127,f);
		if(strlen(buf) != 0){
			INTNETS * n = (INTNETS*)malloc(1 * sizeof(INTNETS));
			char * b;
			b = strtok(buf,"/");
			struct in_addr addr;
			inet_aton(b, &addr);
			n->net = addr;
			b = strtok(NULL,"/");
			if(atoi(b) == 8)  n->mask = 0x000000FF;
			if(atoi(b) == 16) n->mask = 0x0000FFFF;
			if(atoi(b) == 24) n->mask = 0x00FFFFFF;
			pushList(l_intnets,(void*)n);
		}
		buf[0]=0;
	}
	fclose(f);	
    //printList(l_intnets, 3);
	//return 0;
//=======================================================================================
//создаем список клиентов	
	LIST * l_clients = initList();
	LIST * l_trafmonth = initList();
	conn = PQconnectdb(CONINFO);
	if (PQstatus(conn) == CONNECTION_OK){
		res = PQexec(conn, "BEGIN");
	    if (PQresultStatus(res) != PGRES_COMMAND_OK){
			fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		PQclear(res);
		
		res = PQexec(conn, "DECLARE mycursor1 CURSOR FOR select ip from clients ORDER BY ip ASC");
		if (PQresultStatus(res) != PGRES_COMMAND_OK){
			fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		PQclear(res);

		res = PQexec(conn, "FETCH ALL in mycursor1");
		if (PQresultStatus(res) != PGRES_TUPLES_OK){
			fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		int nFields = PQnfields(res);

		for (int i=0; i < PQntuples(res); i++){
				CLIENT * pBuf = (CLIENT*) malloc(1 * sizeof(CLIENT)); 
				struct in_addr addr;
				inet_aton(PQgetvalue(res, i, 0), &addr);
				pBuf->ip = addr;
				pBuf->incall = 0;
				pBuf->incext = 0;
				pBuf->incint = 0;
				pBuf->outall = 0;
				pBuf->outext = 0;
				pBuf->outint = 0;

				pushList(l_clients,(void*)pBuf);
			//}
			//printf("\n");
		}
		PQclear(res);
		
		res = PQexec(conn, "CLOSE mycursor1");
	    PQclear(res);
									
		res = PQexec(conn, "END");
		PQclear(res);
//=======================================================================================
//создаем список с траффиком за последний месяц
		res = PQexec(conn, "BEGIN");
	    if (PQresultStatus(res) != PGRES_COMMAND_OK){
			fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		PQclear(res);
		
		res = PQexec(conn, "DECLARE mycursor1 CURSOR FOR select * from trafmnf ORDER BY ip ASC");
		if (PQresultStatus(res) != PGRES_COMMAND_OK){
			fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		PQclear(res);

		res = PQexec(conn, "FETCH ALL in mycursor1");
		if (PQresultStatus(res) != PGRES_TUPLES_OK){
			fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(conn));
			PQclear(res);
		}
		nFields = PQnfields(res);

		for (int i=0; i < PQntuples(res); i++){
				CLIENT * pBuf = (CLIENT*) malloc(1 * sizeof(CLIENT)); 
				struct in_addr addr;
				inet_aton(PQgetvalue(res, i, 0), &addr);
				pBuf->ip = addr;
				pBuf->incall = atoi(PQgetvalue(res, i, 1));
				pBuf->incext = atoi(PQgetvalue(res, i, 3));
				pBuf->incint = atoi(PQgetvalue(res, i, 5));
				pBuf->outall = atoi(PQgetvalue(res, i, 2));
				pBuf->outext = atoi(PQgetvalue(res, i, 4));
				pBuf->outint = atoi(PQgetvalue(res, i, 6));

				pushList(l_trafmonth,(void*)pBuf);
			//}
			//printf("\n");
		}
		PQclear(res);
		
		res = PQexec(conn, "CLOSE mycursor1");
	    PQclear(res);
									
		res = PQexec(conn, "END");
	    PQclear(res);
//=======================================================================================
	}else{
		fprintf(stderr,"Не удалось соединиться с базой данных: %s\n",PQerrorMessage(conn));
	}
	PQfinish(conn);

	//printList(l_trafmonth,1);
	
	calculateMonthTrafic(l_clients,l_traf5min,l_intnets,l_trafmonth);
	//printList(l_clients,1);
	//printf("+++++++++++++++++++++++\n");
	printList(l_trafmonth,1);
	//printList(l_traf5min,2);
	updateMonthTrafINDatabase(l_trafmonth);
	
    return 0;
}
