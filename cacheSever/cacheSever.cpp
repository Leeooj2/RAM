#include <cstdio>
#include<iostream>
#include<stdio.h>
#include <WS2tcpip.h>
#include<string>
#include<WinSock2.h>
#include<ctime>
#include<windows.h>
#include<map>

#pragma comment(lib,"ws2_32.lib")
using namespace std;
const int PORT = 8000;

#define MaxClient 10
#define MaxBufSize 256
#define _CRT_SECURE_NO_WARINGS

void InitializeRAM();

#define FILE_NAME "D:\\vs项目\\RamBufServer\\test.txt"//读写文件名
#define BUTTONID		10000
#define FF_RAM	0

int  AllocateAlgorithm;
#define MAXRAMSIZE		1073741824
#define MAXAREANO		15
#define MAXPROCESSNO	4194304//1GB/256B个数据对象
#define RAMSIZE			16


//int f = 0;
// 定义空闲分区表
struct {
	int Address;
	int Length;
} FreeAreaList[MAXAREANO];

// 定义进程内存分配表
struct {
	int Flag;
	int Address;
	int Length;//定义每个数据对象的大小为256
	char* id;
	int len;//数据长度
	char* content;//数据内容
} ProcessList[MAXPROCESSNO];


//** 链表节点结构体

struct ListNode
{
	char* m_key;                //key,value 形式方便map存储。
	char* m_value;
	ListNode* pPre;
	ListNode* pNext;

	ListNode(char* key, char* value)
	{
		m_key = key;
		m_value = value;
		pPre = NULL;
		pNext = NULL;
	}
};

//*  LRU缓存实现类  双向链表。
class LRUCache
{
private:
	int m_capacity;    //缓存容量
	ListNode* pHead;   //头节点
	ListNode* pTail;   //尾节点
	map<char*, ListNode*>  mp;   //mp用来存数据，达到find为o(1)级别。

public:
	//** 构造函数初始化缓存大小
	LRUCache(int size)
	{
		m_capacity = size;
		pHead == NULL;
		pTail == NULL;
	}

	~LRUCache()
	{
		//**  一定要注意，map释放内存时，先释放内部new的内存，在释放map的内存
		map<char*, ListNode*>::iterator it = mp.begin();
		for (; it != mp.end();)
		{
			delete it->second;
			it->second = NULL;
			mp.erase(it++);    //** 注意：一定要这样写，it++ 放在其他任何一个地方都会导致其迭代器失效。
		}
		delete pHead;
		pHead == NULL;
		delete pTail;
		pTail == NULL;

	}
	//** 这里只是移除，并不删除节点
	void Remove(ListNode* pNode)
	{
		// 如果是头节点
		if (pNode->pPre == NULL)
		{
			pHead = pNode->pNext;
			pHead->pPre = NULL;
		}

		// 如果是尾节点
		if (pNode->pNext == NULL)
		{
			pTail = pNode->pPre;
			pTail->pNext = NULL;
		}

		else
		{
			pNode->pPre->pNext = pNode->pNext;
			pNode->pNext->pPre = pNode->pPre;
		}

	}
	//  将节点放到头部，最近用过的数据要放在队头。
	void SetHead(ListNode* pNode)
	{
		pNode->pNext = pHead;
		pNode->pPre = NULL;
		if (pHead == NULL)
		{
			pHead = pNode;
		}
		else
		{
			pHead->pPre = pNode;
			pHead = pNode;

		}
		if (pTail == NULL)
		{
			pTail = pHead;
		}
	}
	// * 插入数据，如果存在就只更新数据
	void Set(char* key, char* value)
	{
		map<char*, ListNode*>::iterator it = mp.find(key);
		if (it != mp.end())
		{
			ListNode* Node = it->second;
			Node->m_value = value;
			Remove(Node);
			SetHead(Node);
		}
		else
		{
			ListNode* NewNode = new ListNode(key, value);
			if (mp.size() >= m_capacity)
			{
				map<char*, ListNode*>::iterator it = mp.find(pTail->m_key);
				//从链表移除
				Remove(pTail);
				//删除指针指向的内存
				delete it->second;
				//删除map元素
				mp.erase(it);
			}
			//放到头部
			SetHead(NewNode);
			mp[key] = NewNode;

		}
	}

	//获取缓存里的数据
	char* Get(char* key)
	{
		map<char*, ListNode*>::iterator it = mp.find(key);
		if (it != mp.end())
		{
			ListNode* Node = it->second;
			Remove(Node);
			SetHead(Node);
			return Node->m_value;
		}
		else
		{
			return (char*)"FFFFFFFF";       //返回FFFFFFFF，表明数据id不存在缓存中
		}
	}

	int GetSize()
	{
		return mp.size();
	}

};


int FreeAreaMaxNo;								// 空闲分区最大数目
int ProcessMaxNo;								// 进程最大数目
int usedflag = 0;									//当前操作栈中优先级位数

//---------------------------------------------------------------------------
//     动态分区内存分配初始化函数
//---------------------------------------------------------------------------
void InitializeRAM()
{
	FreeAreaMaxNo = 1;							// 空闲分区数目1个
	ProcessMaxNo = 0;							// 进程数目0个
	FreeAreaList[0].Address = 0;				// 第0空闲分区首址
	FreeAreaList[0].Length = MAXRAMSIZE;		// 第0空闲分区大小
	AllocateAlgorithm = FF_RAM;					// 内存分区分配算法置为首次适应算法

}

//---------------------------------------------------------------------------
//     内存分配函数
//---------------------------------------------------------------------------
void AllocateRAM(int AllocatRamSize, char* id, int len,char* content)
{
	int i, j;

	if (AllocateAlgorithm == FF_RAM) {		// 首次适应算法
		for (i = 0; i < FreeAreaMaxNo; i++) {
			if (FreeAreaList[i].Length >= AllocatRamSize) break;
		}
	}

	if (i >= FreeAreaMaxNo) {
		cout << "数据对象没有足够内存可供分配！" << endl;
		return;
	}

	ProcessList[ProcessMaxNo].Flag = usedflag++;
	ProcessList[ProcessMaxNo].Address = FreeAreaList[i].Address; // 进程内存首址
	ProcessList[ProcessMaxNo].Length = AllocatRamSize;
	ProcessList[ProcessMaxNo].id = id;
	ProcessList[ProcessMaxNo].len = len;
	ProcessList[ProcessMaxNo].content = content;
	ProcessMaxNo++;

	FreeAreaList[i].Address += AllocatRamSize;					// 空闲分区首址
	FreeAreaList[i].Length -= AllocatRamSize;					// 空闲分区大小
	if (FreeAreaList[i].Length == 0) {							// 如果空闲分区大小为0，撤消，则覆盖掉空闲分区表的内容为空
		for (j = i; j < FreeAreaMaxNo - 1; j++) {
			FreeAreaList[j].Address = FreeAreaList[j + 1].Address;
			FreeAreaList[j].Length = FreeAreaList[j + 1].Length;
		}
		FreeAreaMaxNo--;
	}
}

void LRU(char* recvbuf)//对缓冲区满的对象进行LRU替换写入
{
	LRUCache* cache = new LRUCache(ProcessMaxNo);
	int used = 0;
	int processNo = 0;
	for (int v = 0; v < ProcessList[v].Flag; v++)//根据used操作优先级的升序序列进行set赋值
	{
		if (ProcessList[v].Flag == used)
		{
			cache->Set(ProcessList[v].id, ProcessList[v].content);
		}
		used++;

	}
	int i = 0, flag = 0;
	while (recvbuf[i++] != '\0')
	{
		if (recvbuf[i] == ' ')
		{
			flag = i;
			break;//包含空格，表明输入内容中为合法写入请求，但要排除情况

		}
	}
	char* l = new char[flag + 1];//l为数据id
	for (int i = 0; i < flag; i++)
		l[i] = recvbuf[i];
	l[flag] = '\0';
	int w = 0;
	char* tempcontent = new char[strlen(recvbuf) - flag + 1];
	while (recvbuf[i] != '\0')
	{
		tempcontent[w] = recvbuf[i];
		w++;
		i++;
	}
	tempcontent[w] = '\0';

	cache->Set(l, tempcontent);//此时替换了优先度最低的数据对象，实现了LRU替换算法

	//此时替换的数据进行更新数据
	for (int h = 0; h < ProcessMaxNo; h++)
	{
		if (!strcmp(cache->Get(ProcessList[h].id) , "FFFFFFFF"))//表明了此时数据进行了替换，则将数据id改为FFFFFFFF，用于旧数据丢弃
		{
			//开辟文件保存替换出来的数据
			FILE *outfile = fopen(FILE_NAME, "w");
	
			fprintf(outfile, "%s ", ProcessList[h].id);//输出被替换的数据id
			fprintf(outfile, "%d ", ProcessList[h].len);//输出被替换的数据长度
			fprintf(outfile, "%s ", ProcessList[h].content);//输出被替换的数据内容
			fprintf(outfile, "\n");//换行
		
			ProcessList[h].Flag = usedflag++;
			ProcessList[h].Length = MaxBufSize;
			ProcessList[h].id = l;
			ProcessList[h].len = strlen(recvbuf)-(flag+1);
			ProcessList[h].content = tempcontent;
			break;
		}
		
	}
	cout << "数据替换成功" << endl;

}

int servercount = 0;//服务测试数
int datanum = 0;	//服务数据总量

//服务线程
DWORD WINAPI ServerThread(LPVOID lpParameter) {
	SOCKET *ClientSocket = (SOCKET*)lpParameter;
	int recval = 0; 
	int w = 0;//缓冲区下标计数器
	char RecvBuf[MaxBufSize];//接收缓冲区
	char SendBuf[MaxBufSize];//发送缓冲区
	while (1) {
		recval = recv(*ClientSocket, RecvBuf, sizeof(RecvBuf), 0);
		servercount++;
		//测试服务带宽，读写各占5次
		srand((unsigned)time(NULL));//设置种子
		double time3;//计时器
		LARGE_INTEGER nFreq;
		LARGE_INTEGER nBeginTime;
		LARGE_INTEGER nEndTime;

		QueryPerformanceFrequency(&nFreq);
		QueryPerformanceCounter(&nBeginTime);

		if (RecvBuf) {

			cout << "接收到的消息是：" << RecvBuf << "            来自客户端:" << *ClientSocket << endl;
		
			int i = 0, flag = 0;
			while (RecvBuf[i++] != '\0')
			{
				if (RecvBuf[i] == ' ')
				{
					flag = i;
					break;//包含空格，表明输入内容中为合法写入请求，但要排除情况

				}
			}

			if (flag > 0)//写入请求
			{
				//写入缓冲区都被占用返回数据id,FULL
				if (ProcessMaxNo == MAXPROCESSNO - 1)
				{
					char* l = new char[flag + 1];//l为数据id
					for (int i = 0; i < flag; i++)
						l[i] = RecvBuf[i];
					l[flag] = '\0';

					const char* fulltemp = " FULL";
					int k = 0, p = 0, pp = 0;
					while (l[p] != '\0')
					{
						SendBuf[p] = l[p];
						p++;
					}
					while (fulltemp[pp] != '\0')
					{
						SendBuf[p] = fulltemp[pp];
						pp++;
						p++;
					}
					SendBuf[p] = '\0';
					//strcpy(SendBuf, fulltemp);
					k = send(*ClientSocket, SendBuf, sizeof(SendBuf), 0);//发送内容到发送缓冲区
					if (k < 0) {
						cout << "发送失败" << endl;
					}
					datanum += strlen(RecvBuf);//发送数据量加入到datanum计数器中
					datanum += strlen(SendBuf);//接收数据量加入到datanum计数器中

					memset(SendBuf, 0, sizeof(SendBuf));
					LRU(RecvBuf);//实现了LRU替换和文件读取
					goto end;
				}

				w = 0;
				char* m = new char[flag + 1];//当前的数据id,FFFFFFFF不用于数据id
				for (int i = 0; i < flag; i++)
					m[i] = RecvBuf[i];
				m[flag] = '\0';
				int i = flag + 1;
				
				char* tempcontent = new char[strlen(RecvBuf) - flag+1];
				while (RecvBuf[i] != '\0')
				{
					tempcontent[w] = RecvBuf[i];
					w++;
					i++;
				}
				tempcontent[w] = '\0';
				
				int len = strlen(RecvBuf) - (flag + 1);
				for (int q = 0; q < ProcessMaxNo; q++)//寻找内存中数据id是否有相同的数据id
				{
					if (!strcmp(m, ProcessList[q].id))//找到对应的数据id
					{
						ProcessList[q].id = m;
						ProcessList[q].len = len;
						ProcessList[q].Flag = usedflag++;//操作栈中优先级最高，在LRU中替换栈中位于栈顶
						ProcessList[q].content = tempcontent;
						goto send;
					}
				}
				AllocateRAM(MaxBufSize, m,len,tempcontent);			
				
send:			const char* sendtemp = "数据id为";
				const char* sendtemp2 = "保存完成！";

				int qq = 0, qqq = 0, qqqq = 0;
				while (sendtemp[qq] != '\0')
				{
					SendBuf[qq] = sendtemp[qq];
					qq++;
				}
				while (m[qqq] != '\0')
				{
					SendBuf[qq] = m[qqq];
					qqq++;
					qq++;
				}
				while (sendtemp2[qqqq] != '\0')
				{
					SendBuf[qq] = sendtemp2[qqqq];
					qqqq++;
					qq++;
				}
				SendBuf[qq] = '\0';

				int k = 0;
				k = send(*ClientSocket, SendBuf, sizeof(SendBuf), 0);//发送内容到发送缓冲区
				if (k < 0) {
					cout << "发送失败" << endl;
				}
				memset(SendBuf, 0, sizeof(SendBuf));
				cout << "保存数据完成！" << endl;
			}
			else//读入请求
			{
				int q, cun = 0;
				for (q = 0; q < ProcessMaxNo; q++)//寻找内存中数据id
				{
					if (!strcmp(RecvBuf, ProcessList[q].id))//找到对应的数据id
					{
						cun = 1;
						ProcessList[q].Flag = usedflag++;//操作栈中优先级最高，在LRU中替换栈中位于栈顶
						break;//找到q下标对应的数据id
					}
				}

				int qq = 0, qqq = 0, qqqq = 0, qqqqq = 0, qqqqqq = 0, wwwww = 0;
				if (cun == 1)
				{
					char num[64];
					_itoa(ProcessList[q].len, num, 10);//数据长度
					//cout << num << endl;
					const char* sendtemp = "id:";
					const char* sendtemp2 = ",len:";
					const char* sendtemp3 = ",cont:";
					while (sendtemp[qq] != '\0')
					{
						SendBuf[qq] = sendtemp[qq];
						qq++;
					}
					while (RecvBuf[qqq] != '\0')
					{
						SendBuf[qq] = RecvBuf[qqq];
						qq++;
						qqq++;
					}
					while (sendtemp2[qqqq] != '\0')
					{
						SendBuf[qq] = sendtemp2[qqqq];
						qq++;
						qqqq++;
					}
					while (num[qqqqq] != '\0')
					{
						SendBuf[qq] = num[qqqqq];
						qq++;
						qqqqq++;
					}
					while (sendtemp3[qqqqqq] != '\0')
					{
						SendBuf[qq] = sendtemp3[qqqqqq];
						qq++;
						qqqqqq++;
					}
					//cout << ProcessList[q].content << endl;
					while (ProcessList[q].content[wwwww] != '\0')
					{
						SendBuf[qq] = ProcessList[q].content[wwwww];
						qq++;
						wwwww++;
					}
					SendBuf[qq] = '\0';

					int k = 0;
					k = send(*ClientSocket, SendBuf, sizeof(SendBuf), 0);//发送内容到发送缓冲区
					if (k < 0) {
						cout << "发送失败" << endl;
					}
					datanum += strlen(RecvBuf);//发送数据量加入到datanum计数器中
					datanum += strlen(SendBuf);//接收数据量加入到datanum计数器中

					memset(SendBuf, 0, sizeof(SendBuf));
					cout << "发送数据完成！" << endl;
	
				}
				else//不存在数据id
				{
					int k = 0;
					const char* sendtemp = "FFFFFFFF";
					strcpy(SendBuf, sendtemp);
					k = send(*ClientSocket, SendBuf, sizeof(SendBuf), 0);//发送内容到发送缓冲区
					if (k < 0) {
						cout << "发送失败" << endl;
					}
					datanum += strlen(RecvBuf);//发送数据量加入到datanum计数器中
					datanum += strlen(SendBuf);//接收数据量加入到datanum计数器中

					memset(SendBuf, 0, sizeof(SendBuf));
					cout << "发送数据完成！" << endl;
				}
			}
		}
		else
		{
			cout << "接收消息结束！" << endl;
			break;
		}
end:	memset(RecvBuf, 0, sizeof(RecvBuf));//初始化接收缓冲区为0

		if (servercount == 10)
		{
			QueryPerformanceCounter(&nEndTime);
			time3 = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;

			cout << "测试5次读与5次写下的服务带宽：" << datanum / time3 << endl;
		}
		
	}//while
	closesocket(*ClientSocket);
	free(ClientSocket);
	return 0;
}

int main() {
	WSAData wsd;
	WSAStartup(MAKEWORD(2, 2), &wsd);
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN ListenAddr;
	ListenAddr.sin_family = AF_INET;
	ListenAddr.sin_addr.S_un.S_addr = INADDR_ANY;//表示填入本机ip
	ListenAddr.sin_port = htons(PORT);
	int n;
	n = bind(ListenSocket, (LPSOCKADDR)&ListenAddr, sizeof(ListenAddr));
	if (n == SOCKET_ERROR) {
		cout << "端口绑定失败！" << endl;
		return -1;
	}
	else {
		cout << "端口绑定成功：" << PORT << endl;
	}
	int l = listen(ListenSocket, 20);
	cout << "服务端准备就绪，等待连接请求" << endl;

	InitializeRAM();

	while (1) {
		//循环接收客户端连接请求并创建服务线程
		SOCKET *ClientSocket = new SOCKET;
		ClientSocket = (SOCKET*)malloc(sizeof(SOCKET));
		//接收客户端连接请求
		int SockAddrlen = sizeof(sockaddr);
		*ClientSocket = accept(ListenSocket, 0, 0);
		cout << "一个客户端已连接到服务器，socket是：" << *ClientSocket << endl;
		//InitializeRAM();

		CreateThread(NULL, 0, &ServerThread, ClientSocket, 0, NULL);

	}//while
	closesocket(ListenSocket);		//关闭服务端套接字
	WSACleanup();					//释放套接字资源
	return(0);
}//main
