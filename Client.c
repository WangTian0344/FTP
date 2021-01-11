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
#define SERVER_PORT 2121

/*
连接到服务器后，会不停循环，等待输入，
输入quit后，断开与服务器的连接
*/
struct sockaddr_in InitSocket(int port);//初始化socket
void ClientProcess(int clientSocket);//服务器处理主程序
void Help();//帮助信息
void Modehelp();//模式帮助信息
void Quit(int clientSocket,char sendbuf[]);//退出服务器
int Login(int clientSocket);//登录服务器
int Active_Mode(int clientSocket,char sendbuf[]);//设置主动模式
int Passive_Mode(int clientSocket,char sendbuf[]);//设置被动模式
void Pwd(int clientSocket,char sendbuf[]);//显示当前目录
void Dir(int clientSocket,char sendbuf[]);//显示当前目录文件列表
void Cd_re(int clientSocket,char sendbuf[]);//返回上级目录
void Cd(int clientSocket,char sendbuf[]);//进入子目录
void Put(int clientSocket,char sendbuf[],int server);//上传文件
void Get(int clientSocket,char sendbuf[],int server);//下载文件
void Mkdir(int clientSocket,char sendbuf[]);//创建文件夹
void Rm(int clientSocket,char sendbuf[]);//删除 文件或空文件夹

struct sockaddr_in InitSocket(int port)//初始化socket
{
	struct sockaddr_in socket;
	socket.sin_family = AF_INET;//设置协议族，IPv4
	socket.sin_port = htons(port);//设置端口号
	socket.sin_addr.s_addr = inet_addr("127.0.0.1");//指定服务器端的ip，本地测试：127.0.0.1
	return socket;
}

void ClientProcess(int clientSocket)//服务器处理主程序
{

	char sendbuf[200];//发送缓存
	char recvbuf[200];//接受缓存
	int iDataNum;
	int active_server=0;//主动模式套接字描述
	int passive_server=0;//被动模式套接字模式
	int active_set=0;//主动模式设置与否
	int passive_set=0;//被动模式设置与否

	while(1)
	{
		while((c=getchar())!='\n'&&c!=EOF);//清除缓存区
		memset(sendbuf,0,200);
		printf("输入命令(输入'?'或'help'来获取帮助):");
		scanf("%[^\n]", sendbuf);//输入命令

		if(strcmp(sendbuf, "quit") == 0)//退出
 		{
			Quit(clientSocket,sendbuf);
			break;
		}

		if(strcmp(sendbuf, "?")==0||strcmp(sendbuf,"help")==0)//帮助
		{
			Help();
			continue;
		}

		if(strcmp(sendbuf,"port")==0)//主动模式
		{
			if(active_set==1)
			{
				printf("主动模式已设置!\n");
			}
			else
			{
				if(passive_set==1)//如果已设置被动模式，关闭
				{
					close(passive_server);
					passive_set=0;
				}
				active_server=Active_Mode(clientSocket,sendbuf);
				active_set=1;
			}
			continue;

		}
		if(strcmp(sendbuf,"pasv")==0)//被动模式
		{
			if(passive_set==1)
			{
				printf("被动模式已设置!\n");
			}
			else
			{
				if(active_set==1)//如果已设置主动模式，关闭
				{
					close(active_server);
					active_set=0;
				}
				passive_server=Passive_Mode(clientSocket,sendbuf);
				passive_set=1;
			}
			continue;

		}
		if(strcmp(sendbuf,"pwd")==0)//显示当前目录
		{
			Pwd(clientSocket,sendbuf);
			continue;
		}
		if(strcmp(sendbuf,"dir")==0)//显示文件列表
		{
			Dir(clientSocket,sendbuf);
			continue;
		}
		if(strcmp(sendbuf,"cd ..")==0)//返回上级目录
		{
			Cd_re(clientSocket,sendbuf);
			continue;
		}
		else if(strncmp(sendbuf,"cd ",3)==0)//进入子目录
		{
			Cd(clientSocket,sendbuf);
			continue;
		}
		if(strncmp(sendbuf,"put ",4)==0)//上传文件
		{
			if(active_set!=1&&passive_set!=1)//先设置模式
			{
				Modehelp();
				continue;
			}
			else if(active_set==1)//选择模式
				Put(clientSocket,sendbuf,active_server);
			else if(passive_set==1)
				Put(clientSocket,sendbuf,passive_server);
			continue;
		}
		if(strncmp(sendbuf,"get ",4)==0)//下载文件
		{
			if(active_set!=1&&passive_set!=1)
			{
				Modehelp();
				continue;
			}
			else if(active_set==1)
				Get(clientSocket,sendbuf,active_server);
			else if(passive_set==1)
				Get(clientSocket,sendbuf,passive_server);
			continue;
		}
		if(strncmp(sendbuf,"mkdir ",6)==0)//创建文件夹
		{
			Mkdir(clientSocket,sendbuf);
			continue;
		}
		if(strncmp(sendbuf,"rm ",3)==0)//删除文件或空文件夹
		{
			Rm(clientSocket,sendbuf);
			continue;
		}
		printf("输入命令无效!\n");
	}
	if(active_set==1)//关闭数据连接
		close(active_server);
	if(passive_set==1)
		close(passive_server);
}

void Help()//帮助信息
{
	printf("\n");
	printf("port --------------------将数据传输设为主动模式\n");
	printf("pasv --------------------将数据传输设为被动模式\n");
	printf("get filename ------------下载文件(filename 文件名)\n");
	printf("put filename ------------上传文件(filename 文件名)\n");
	printf("pwd ---------------------显示当前目录\n");
	printf("dir ---------------------列出服务器当前文件列表下的文件和子目录\n");
	printf("cd foldername -----------进入指定目录(foldername 文件夹名)\n");
	printf("cd .. -------------------返回上级目录\n");
    printf("mkdir foldername --------创建文件夹(foldername 文件夹名)\n");
    printf("rm filename/foldername --删除文件或空文件夹(同上)\n");
	printf("?/help ------------------获取帮助\n");
	printf("quit --------------------退出ftp服务器\n");
	printf("\n");

}

void Modehelp()//模式帮助信息
{
	printf("\n");
	printf("请设置数据传输模式！\n");
	printf("port ----------------将数据传输设为主动模式\n");
	printf("pasv ----------------将数据传输设为被动模式\n");
	printf("\n");
}

void Quit(int clientSocket,char sendbuf[])//退出服务器
{
	send(clientSocket, sendbuf, strlen(sendbuf), 0);//发送退出信息
	printf("已退出FTP服务器!\n");
}
int Login(int clientSocket)//登录服务器
{
	char user[20];//账号
	char pass[20];//密码
	char sendbuf[200];//发送缓存
	char recvbuf[200];//接受缓存
	int iDataNum=0;
	memset(user,0,20);
	memset(pass,0,20);
	memset(sendbuf,0,200);
	memset(recvbuf,0,200);
	printf("user:");//接受账号和密码
	scanf("%s",user);
	printf("pass:");
	scanf("%s",pass);
	strcat(sendbuf,user);//编辑发送信息
	strcat(sendbuf," ");
	strcat(sendbuf,pass);
	send(clientSocket, sendbuf, strlen(sendbuf), 0);//发送账号密码
	recvbuf[0]='\0';
	iDataNum = recv(clientSocket, recvbuf, 200, 0);//接收登录信息
	recvbuf[iDataNum]='\0';
	if(atoi(recvbuf)==220)
	{
		printf("登录FTP服务器成功!\n");
		return 1;
	}
	else
		return 0;

}

int Active_Mode(int clientSocket,char sendbuf[])//设置主动模式
{
	char recvbuf[200];//接收缓存
	int num=0;
	int portnumber;
	send(clientSocket, sendbuf, strlen(sendbuf), 0);//发送主动模式信息
	recvbuf[0]='\0';
	num=recv(clientSocket, recvbuf, 200, 0);//接受端口号
	recvbuf[num]='\0';
	portnumber=atoi(recvbuf);

	int DataSocket;
	struct sockaddr_in client_data_addr;
	int addr_len=sizeof(client_data_addr);
	int server;
	if((DataSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	bzero(&client_data_addr, sizeof(client_data_addr));//初始化套接字
	client_data_addr.sin_family = AF_INET;
	client_data_addr.sin_port = htons(portnumber);
	client_data_addr.sin_addr.s_addr = htonl(INADDR_ANY);

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

	server = accept(DataSocket, (struct sockaddr*)&client_data_addr, 	(socklen_t*)&addr_len);//接收请求

	if(server < 0)
	{
		perror("accept");
	}

	recvbuf[0] = '\0';
	num = recv(server, recvbuf, 200, 0);//接收验证信息
	recvbuf[num] = '\0';
	if(strcmp(recvbuf,"125")==0)
	printf("主动模式设置成功!\n");
	return server;
}

int Passive_Mode(int clientSocket,char sendbuf[])//设置被动模式
{
	char recvbuf[200];//接收缓存
	int num=0;
	int portnumber;
	send(clientSocket, sendbuf, strlen(sendbuf), 0);//发送被动模式信息
	recvbuf[0]='\0';
	num=recv(clientSocket, recvbuf, 200, 0);//接收端口
	recvbuf[num]='\0';
	portnumber=atoi(recvbuf);

	sleep(1);//休眠一秒，等待服务器端socket的建立，防止无法连接

	int DataSocket;
	struct sockaddr_in server_data_Addr;

	if((DataSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	server_data_Addr.sin_family = AF_INET;//初始化socket
	server_data_Addr.sin_port = htons(portnumber);
	server_data_Addr.sin_addr.s_addr =inet_addr("127.0.0.1");

	if(connect(DataSocket, (struct sockaddr *)&server_data_Addr, sizeof(server_data_Addr)) < 0)//连接
	{
		perror("connect");
		return 1;
	}

	recvbuf[0] = '\0';
	num = recv(DataSocket, recvbuf, 200, 0);//接收验证信息
	recvbuf[num] = '\0';
	if(strcmp(recvbuf,"227")==0)
	printf("被动模式设置成功!\n");

	return DataSocket;


}

void Pwd(int clientSocket,char sendbuf[])//显示当前目录
{
	char recvbuf[200];//目录缓存
	int num=0;
	memset(recvbuf,0,200);
	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,200,0);//接收目录
	recvbuf[num]='\0';
	printf("当前地址:%s\n",recvbuf);
}

void Dir(int clientSocket,char sendbuf[])//显示当前目录文件列表
{
	char recvbuf[1024];//列表缓存
	int num=0;
	memset(recvbuf,0,1024);

	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,1024,0);//接收目录信息
	recvbuf[num]='\0';
	if(strcmp(recvbuf,"-1")==0)
	{
		printf("文件夹为空!\n");
	}
	else
	printf("文件列表:\n%s\n",recvbuf);
}

void Cd_re(int clientSocket,char sendbuf[])//返回上级目录
{
	char recvbuf[200];
	int num=0;
	memset(recvbuf,0,200);
	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,200,0);//接收验证信息
	recvbuf[num]='\0';
    if(strcmp(recvbuf,"-1")==0)
        printf("返回失败!\n");
    else
    {
       printf("返回成功!\n当前地址:%s\n",recvbuf);
    }
}

void Cd(int clientSocket,char sendbuf[])//进入子目录
{
	char recvbuf[200];//目录缓存
	int num=0;
	memset(recvbuf,0,200);

	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,200,0);//接收验证信息
	recvbuf[num]='\0';
	if(strcmp(recvbuf,"-1")==0)
		printf("该文件夹不存在!\n",recvbuf);
	else
		printf("当前地址:%s\n",recvbuf);
}

void Put(int clientSocket,char sendbuf[],int server)//上传文件
{
	char recvbuf[200];//接收缓存
	char filename[200];//文件名
	char filebuf[200];//文件内容
	FILE *fp;
	char *c;
	int file_empty=0;//判断文件为空
	memset(recvbuf,0,200);
	memset(filename,0,200);
	memset(filebuf,0,200);
	strcpy(filename,sendbuf+4);//获取文件路径
	if((fp=fopen(filename,"r"))==NULL)//打卡文件
	{
		printf("文件不存在!\n");
		return;
	}
	c=filename+strlen(filename)-1;
	while(c-filename!=0&&*c!='/')//获取文件名
		c--;
	strcpy(sendbuf,"put ");
	strcat(sendbuf,c+1);

	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送命令
	while(fgets(filebuf,200,fp)!=NULL)//发送文件内容
	{
		file_empty=1;
		send(server,filebuf,strlen(filebuf),0);
	}
	if(file_empty==0)//文件为空
	{
		strcpy(filebuf,"-1");
		send(server,filebuf,strlen(filebuf),0);
	}
	fclose(fp);//关闭文件
	printf("上传成功!\n");
}

void Get(int clientSocket,char sendbuf[],int server)//下载文件
{
	char recvbuf[200];//接收缓存
	char filename[200];//文件名
	char filebuf[200];//文件内容
	FILE *fp;
	int num=0;
	memset(recvbuf,0,200);
	memset(filename,0,200);
	memset(filebuf,0,200);

	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送命令
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,200,0);//接收验证信息
	recvbuf[num]='\0';

	if(strcmp(recvbuf,"-1")==0)
	{
		printf("文件不存在!\n");
		return;
	}

	strcpy(filename,"/home/oscreader/Desktop/Network/ClientFile");
	strcat(filename,"/");
	strcat(filename,sendbuf+4);//确定路径

	if((fp=fopen(filename,"w"))==NULL)//打开文件
	{
		printf("文件打开失败!\n");
		return;
	}

	while((num=recv(server,filebuf,200,0))>0)//接收文件内容
	{
		if(strcmp(filebuf,"-1")==0)
			break;
		fputs(filebuf,fp);
		if(num<200)
			break;
	}
	printf("下载成功!\n");
	fclose(fp);

}

void Mkdir(int clientSocket,char sendbuf[])//创建文件夹
{
	char recvbuf[200];
	int num=0;
	memset(recvbuf,0,200);
	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送命令
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,200,0);//接收验证信息
	recvbuf[num]='\0';
	if(strcmp(recvbuf,"0")==0)
		printf("创建文件夹成功!\n");
	else
		printf("创建文件夹失败!\n");
}


void Rm(int clientSocket,char sendbuf[])//删除 文件或空文件夹
{
	char recvbuf[200];
	int num=0;
	memset(recvbuf,0,200);
	send(clientSocket,sendbuf,strlen(sendbuf),0);//发送命令
	recvbuf[0]='\0';
	num=recv(clientSocket,recvbuf,200,0);//接收验证信息
	recvbuf[num]='\0';
	if(strcmp(recvbuf,"0")==0)
		printf("删除文件/空文件夹成功!\n");
	else
		printf("删除文件/空文件夹失败!\n");
}

int main()
{
	int clientSocket;//客户端只需要一个套接字文件描述符，用于和服务器通信

	struct sockaddr_in serverAddr;//描述服务器的socket

	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	serverAddr=InitSocket(SERVER_PORT);//初始化套接字

	if(connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)//连接
	{
		perror("connect");
		return 1;
	}

	printf("连接到FTP服务器...\n");

	while(Login(clientSocket)==0)//登录
	{
		printf("登录失败，请重新输入账号和密码!\n");
	}

	ClientProcess(clientSocket);//客户端主程序

	close(clientSocket);//关闭套接字

	return 0;

}

