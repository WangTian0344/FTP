#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#define SERVER_PORT 2121

/*
监听后，一直处于accept阻塞状态，
直到有客户端连接，
当客户端如数quit后，断开与客户端的连接
*/

typedef struct//线程函数参数结构体
{
	int clientSocket;//套接字描述符
	int port;//端口号
}Serverparam;


struct sockaddr_in InitSocket(int port);//初始化套接字
void *ServerProcess(void *param);//服务器处理主程序
int  LoginProcess(int clientSocket);//登录处理
int Active_Mode(int clientSocket,int port);//设置主动模式
int Passive_Mode(int clientSocket,int port);//设置被动模式
void PwdProcess(int clientSocket,char file_addr[]);//显示当前目录
void DirProcess(int clientSocket,char file_addr[]);//显示当前目录文件列表
void CdreProcess(int clientSocket,char file_addr[]);//返回上级目录
void CdProcess(int clientSocket,char file_addr[],char recvbuf[]);//进入子目录
void PutProcess(int clientSocket,char file_addr[],char buffer[],int client);//上传文件
void GetProcess(int clientSocket,char file_addr[],char buffer[],int client);//下载文件
int Is_file_exist(char file_addr[],char filename[]);//判断文件是否在当前目录中
void MkdirProcess(int clientSocket,char file_addr[],char buffer[]);//创建文件夹
void RmProcess(int clientSocket,char file_addr[],char buffer[]);//删除文件或空文件夹

pthread_mutex_t login_mutex;//登录文件锁
pthread_mutex_t file_mutex;//文件操作锁

struct sockaddr_in InitSocket(int port)//初始化套接字
{
	struct sockaddr_in socket;
	bzero(&socket, sizeof(socket));
	socket.sin_family = AF_INET;//IPv4
	socket.sin_port = htons(port);//端口
	socket.sin_addr.s_addr =inet_addr("127.0.0.1");//IP
	return socket;
}

void *ServerProcess(void *param)//服务器处理主程序
{
	char buffer[200];//接收缓存
	int iDataNum;
	int clientSocket;//连接套接字描述符
	int port;//端口
	int active_client=0;//主动模式套接字描述
	int passive_client=0;//被动模式套接字模式
	int login_set=0;//登录与否
	int active_set=0;//主动模式设置与否
	int passive_set=0;//被动模式设置与否
	char file_addr[200]="/home/oscreader/Desktop/Network/ServerFile";//默认文件目录

	Serverparam *p=(Serverparam*)param;//初始化端口和套接字描述符
	clientSocket=p->clientSocket;
	port=p->port;

	while(1)
	{
		while(login_set==0&&LoginProcess(clientSocket)==0);//登录
		if(login_set==0)
		{
			printf("登录成功\n");
			login_set=1;
		}
		memset(buffer,0,200);
		printf("读取消息:");
		buffer[0] = '\0';
		iDataNum = recv(clientSocket, buffer, 1024, 0);//接收命令
		if(iDataNum < 0)
		{
			perror("recv null");
            continue;
		}
		buffer[iDataNum] = '\0';

		if(strcmp(buffer, "quit") == 0)//退出
			break;

		printf("%s\n", buffer);

 		if(strcmp(buffer, "port") == 0)//主动模式
		{
			if(passive_set==1)//如果已设置被动模式，关闭
			{
				close(passive_client);
				passive_set=0;
			}
 			active_client=Active_Mode(clientSocket,port);
			active_set=1;
			continue;
		}
		if(strcmp(buffer, "pasv") == 0)//被动模式
		{
			if(active_set==1)//如果已设置主动模式，关闭
			{
				close(active_client);
				active_set=0;
			}
 			passive_client=Passive_Mode(clientSocket,port+1);
			passive_set=1;
			continue;
		}
		if(strcmp(buffer,"pwd")==0)//显示当前目录
		{
			PwdProcess(clientSocket,file_addr);
			continue;
		}
		if(strcmp(buffer,"dir")==0)//显示文件列表
		{
			DirProcess(clientSocket,file_addr);
			continue;
		}
		if(strcmp(buffer,"cd ..")==0)//返回上级目录
		{
			CdreProcess(clientSocket,file_addr);
			continue;
		}
		else if(strncmp(buffer,"cd ",3)==0)//进入子目录
		{
			CdProcess(clientSocket,file_addr,buffer);
			continue;
		}
		if(strncmp(buffer,"put ",4)==0)//上传文件
		{
			pthread_mutex_lock(&file_mutex);//文件操作上锁
			if(active_set==1)//选择模式
				PutProcess(clientSocket,file_addr,buffer,active_client);
			else if(passive_set==1)
				PutProcess(clientSocket,file_addr,buffer,passive_client);
			pthread_mutex_unlock(&file_mutex);//文件操作解锁
			continue;
		}
		if(strncmp(buffer,"get ",4)==0)//下载文件
		{
			pthread_mutex_lock(&file_mutex);//文件操作上锁
			if(active_set==1)//选择模式
				GetProcess(clientSocket,file_addr,buffer,active_client);
			else if(passive_set==1)
				GetProcess(clientSocket,file_addr,buffer,passive_client);
			pthread_mutex_unlock(&file_mutex);//文件操作解锁
			continue;
		}
		if(strncmp(buffer,"mkdir ",6)==0)//创建文件夹
		{
			pthread_mutex_lock(&file_mutex);//文件操作上锁
			MkdirProcess(clientSocket,file_addr,buffer);
			pthread_mutex_unlock(&file_mutex);//文件操作解锁
			continue;
		}
		if(strncmp(buffer,"rm ",3)==0)//删除文件或空文件夹
		{
			pthread_mutex_lock(&file_mutex);//文件操作上锁
			RmProcess(clientSocket,file_addr,buffer);
			pthread_mutex_unlock(&file_mutex);//文件操作解锁
			continue;
		}
	}
	if(active_set==1)//关闭数据连接
		close(active_client);
	if(passive_set==1)
		close(passive_client);
}

int LoginProcess(int clientSocket)//登录处理
{
	pthread_mutex_lock(&login_mutex);//登录文件上锁
	char sendbuf[200];//发送缓存
	char recvbuf[200];//接受缓存
	char loginbuf[200];//登录信息
	int num=0;
	FILE *loginfile;
	memset(sendbuf,0,200);
	memset(recvbuf,0,200);
	memset(loginbuf,0,200);
	recvbuf[0]='\0';
	num = recv(clientSocket, recvbuf, 200, 0);//接收登录信息
	recvbuf[num]='\0';

	if((loginfile=fopen("/home/oscreader/Desktop/Network/login.txt","r"))==NULL)//打开登录文件
	{
		strcpy(sendbuf,"221");
		perror("open file fail!\n");
	}
	else
	{
		while(fgets(loginbuf,200,loginfile)!=NULL)//查询
		{
			loginbuf[strlen(loginbuf)-1]='\0';
			if(strcmp(loginbuf,recvbuf)==0)
			{
				strcpy(sendbuf,"220");
				break;
			}
		}
	}
	if(strcmp(sendbuf,"")==0)//发送验证信息
		strcpy(sendbuf,"221");
	send(clientSocket, sendbuf, strlen(sendbuf), 0);
	fclose(loginfile);
	pthread_mutex_unlock(&login_mutex);//登录文件解锁
	if(strcmp(sendbuf,"220")==0)
		return 1;
	else
		return 0;
}

int Active_Mode(int clientSocket,int port)//设置主动模式
{
	char sendbuf[200];//发送缓存
	char recvbuf[200];//接收缓存
	sprintf(sendbuf,"%d",port);
	send(clientSocket, sendbuf, strlen(sendbuf), 0);//接收命令

	sleep(1);//休眠一秒，等待客户端socket的建立，防止无法连接

	int DataSocket;
	struct sockaddr_in server_data_Addr;

	if((DataSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	server_data_Addr.sin_family = AF_INET;//IPv4
	server_data_Addr.sin_port = htons(port);//端口
	server_data_Addr.sin_addr.s_addr =inet_addr("127.0.0.1");//IP

	if(connect(DataSocket, (struct sockaddr *)&server_data_Addr, sizeof(server_data_Addr)) < 0)//连接
	{
		perror("connect");
		return 1;
	}
 	printf("主动模式设置成功!\n");
	strcpy(sendbuf,"125");
	send(DataSocket, sendbuf, strlen(sendbuf), 0);//发送验证信息
	return DataSocket;
}

int Passive_Mode(int clientSocket,int port)//设置被动模式
{
	char sendbuf[200];//发送缓存
	char recvbuf[200];//接收缓存
	sprintf(sendbuf,"%d",port);
	send(clientSocket, sendbuf, strlen(sendbuf), 0);//发送端口

	int DataSocket;
	struct sockaddr_in client_data_addr;
	int addr_len=sizeof(client_data_addr);
	int client;

	if((DataSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	bzero(&client_data_addr, sizeof(client_data_addr));
	client_data_addr.sin_family = AF_INET;//IPv4
	client_data_addr.sin_port = htons(port);//端口
	client_data_addr.sin_addr.s_addr = htonl(INADDR_ANY);//IP

	if(bind(DataSocket, (struct sockaddr *)&client_data_addr, sizeof(client_data_addr)) < 0)//绑定端口
	{
		perror("connect");
		return 1;
	}
	if(listen(DataSocket, 5) < 0)//监听
	{
		perror("listen");
		return 1;
	}

	client = accept(DataSocket, (struct sockaddr*)&client_data_addr, 	(socklen_t*)&addr_len);//接收连接

	if(client < 0)
	{
		perror("accept");
	}

	printf("被动模式设置成功!\n");
	strcpy(sendbuf,"227");
	send(client, sendbuf, strlen(sendbuf), 0);//发送验证信息
	return client;
}

void PwdProcess(int clientSocket,char file_addr[])//显示当前目录
{
	send(clientSocket,file_addr,strlen(file_addr),0);//发送目录
}
void DirProcess(int clientSocket,char file_addr[])//显示当前目录文件列表
{
	char sendbuf[200];//发送缓存
	int empty=0;//目录为空否
	memset(sendbuf,0,200);
	DIR *dir;
	struct dirent *ptr;
	dir=opendir(file_addr);
	while((ptr=readdir(dir))!=NULL)//遍历目录中所有文件
	{
		if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
			continue;
		strcat(sendbuf,ptr->d_name);
		strcat(sendbuf,"	");
		empty=1;
	}
	closedir(dir);
	if(empty==0)//目录为空
		strcpy(sendbuf,"-1");
	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息
}

void CdreProcess(int clientSocket,char file_addr[])//返回上级目录
{
	char sendbuf[200];//发送缓存
	char *c;
	memset(sendbuf,0,200);
	c=file_addr+strlen(file_addr)-1;
	while(c-file_addr!=0&&*c!='/')//获得目录名
		c--;

	if(strcmp("ServerFile",c+1)==0)//是否为根目录
	{
		strcat(sendbuf,"-1");
	}
	else
	{
		*c='\0';
		strcpy(sendbuf,file_addr);
	}

	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息
}

void CdProcess(int clientSocket,char file_addr[],char recvbuf[])//进入子目录
{
	char sendbuf[200];//发送信息
	char folderbuf[200];//文件夹
	int find=0;
	memset(sendbuf,0,200);
	memset(folderbuf,0,200);
	strcpy(folderbuf,recvbuf+3);

	DIR *dir;
	struct dirent *ptr;
	dir=opendir(file_addr);
	while((ptr=readdir(dir))!=NULL)//遍历文件，查看是否为文件夹
	{
		if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
			continue;
		else if(strcmp(ptr->d_name,folderbuf)==0&&ptr->d_type==4)
		{
			strcat(file_addr,"/");
			strcat(file_addr,folderbuf);
			find=1;
			break;
		}
	}
	closedir(dir);
	if(find==1)
		strcpy(sendbuf,file_addr);
	else
		strcpy(sendbuf,"-1");
	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送验证信息
}

void PutProcess(int clientSocket,char file_addr[],char buffer[],int client)//上传文件
{
	char filename[200];//文件名
	char recvbuf[200];//接收缓存
	int num=0;
	FILE *fp;

	memset(recvbuf,0,200);
	strcpy(filename,file_addr);
	strcat(filename,"/");
	strcat(filename,buffer+4);//获取文件名

	fp=fopen(filename,"w");//打开文件

	recvbuf[0]='\0';
	while((num=recv(client,recvbuf,200,0))>0)//接收文件内容
	{
		if(strcmp(recvbuf,"-1")==0)
			break;
		fputs(recvbuf,fp);
		if(num<200)
			break;
	}
	printf("上传成功!\n");
	fclose(fp);
}

void GetProcess(int clientSocket,char file_addr[],char buffer[],int client)//下载文件
{
	char filename[200];//文件名
	char recvbuf[200];//接收缓存
	char filebuf[200];//文件内容
	int num=0;
	int file_empty=0;
	FILE *fp;
	memset(filename,0,200);
	memset(recvbuf,0,200);
	memset(filebuf,0,200);
	if(Is_file_exist(file_addr,buffer+4)==0)//判断文件是否存在
	{
		strcpy(buffer,"-1");
		send(clientSocket,buffer,strlen(buffer),0);
		return;
	}
	strcpy(filename,file_addr);
	strcat(filename,"/");
	strcat(filename,buffer+4);//获取文件名
	printf("filename:%s\n",filename);
	fp=fopen(filename,"r");

	strcpy(buffer,"1");
	send(clientSocket,buffer,strlen(buffer),0);//发送验证

	while(fgets(filebuf,200,fp)!=NULL)//发送文件内容
	{
		file_empty=1;
		send(client,filebuf,strlen(filebuf),0);
	}
	if(file_empty==0)
	{
		strcpy(filebuf,"-1");
		send(client,filebuf,strlen(filebuf),0);
	}
	printf("下载成功!\n");
	fclose(fp);
}
int Is_file_exist(char file_addr[],char filename[])//判断文件是否在当前目录中
{
	DIR *dir;
	struct dirent *ptr;
	dir=opendir(file_addr);
	while((ptr=readdir(dir))!=NULL)
	{
		if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
			continue;
		else if(strcmp(ptr->d_name,filename)==0&&ptr->d_type!=4)
		{
			return 1;
		}
	}
	closedir(dir);
	return 0;
}

void MkdirProcess(int clientSocket,char file_addr[],char buffer[])//创建文件夹
{
	char foldername[200];
	int mk;
	memset(foldername,0,200);
	strcpy(foldername,file_addr);
	strcat(foldername,"/");
	strcat(foldername,buffer+6);//获取文件夹名
	printf("%s\n",foldername);
	mk=mkdir(foldername,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);//创建
	if(mk==0)
		strcpy(buffer,"0");
	else
		strcpy(buffer,"-1");
	send(clientSocket,buffer,strlen(buffer),0);//发送验证
}

void RmProcess(int clientSocket,char file_addr[],char buffer[])//删除文件或空文件夹
{
	char filename[200];
	int rm;
	memset(filename,0,200);
	strcpy(filename,file_addr);
	strcat(filename,"/");
	strcat(filename,buffer+3);//获取文件名
	printf("%s\n",filename);
	rm=remove(filename);//删除
	if(rm==0)
		strcpy(buffer,"0");
	else
		strcpy(buffer,"-1");
	send(clientSocket,buffer,strlen(buffer),0);//发送验证
}
int main()

{
	int serverSocket;//调用socket函数返回的文件描述符
	struct sockaddr_in server_addr;//声明两个套接字sockaddr_in结构体变量，分别表示客户端和服务器
	struct sockaddr_in clientAddr;
	int addr_len = sizeof(clientAddr);
	int client;

	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	server_addr=InitSocket(SERVER_PORT);//初始化套接字

	pthread_attr_t attr;//创建线程参数
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_t pth;

	if(bind(serverSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)//绑定端口
	{
		perror("connect");
		return 1;
	}

	if(listen(serverSocket, 5) < 0)//监听
	{
		perror("listen");
		return 1;
	}

	while(1)
	{

		printf("监听端口: %d\n", SERVER_PORT);

		client = accept(serverSocket, (struct sockaddr*)&clientAddr, 	(socklen_t*)&addr_len);//接收连接

		if(client < 0)
		{
			perror("accept");
			continue;
		}

		printf("等待消息...\n");

		printf("IP地址:%s\n", inet_ntoa(clientAddr.sin_addr));

		printf("端口号:%d\n", htons(clientAddr.sin_port));

		Serverparam *param=malloc(sizeof(Serverparam));//初始化线程函数参数
		param->clientSocket=client;
		param->port=htons(clientAddr.sin_port)+1;

 		pthread_create(&pth,&attr,&ServerProcess,(void*)param);//创建啊线程


	}

	close(serverSocket);//关闭套接字

	return 0;

}
