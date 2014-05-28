#include<string>
#include<fstream>
#include<iostream>
#include<cstdlib>
#include<cstdio>
#include<cstring>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include"xenbus_demo.h"

using namespace std;

int fd;

ofstream log("xa.log");

int client_net(char server_ip[20], int domid)
{
	int sockfd, status = -1;
	struct sockaddr_in serv_addr;
	

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket error!");
		return -1;
	}

	//initialize the serv_addr
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);

	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr)) == -1){
		perror("connect error");
		return -1;
	}
	
	send(sockfd, &domid, sizeof(int), NULL);
	cout << "send domid" << endl;
	log << "send domid" << endl;

	recv(sockfd, &status, sizeof(int), NULL);
	if(status != 0)//GRNIST_ok == 0
		return -1;

	return 0;
}

int client_init()
{
	int domid;
	fd = open("/dev/xa", O_RDWR);
	domid = ioctl(fd, XA_DOMAIN_NUMBER, NULL);
	cout << "domid is:" << domid << endl;
	log << "domid is:" << domid << endl;
	return domid;
}

int map_ring()
{
	cout << "call ioctl in map_ring" << endl;
	log << "call ioctl in map_ring" << endl;
	ioctl(fd, XA_MAP_RING, NULL);
	return 0;
}

int watch()
{
	cout << "call ioctl in watch" << endl;
	log << "call ioctl in watch" << endl;
	ioctl(fd, XA_WATCH, NULL);
	return 0;
}

int wait_for()
{
	string tmp;
	cout << "type \"go\" to continue." << endl;
	cin >> tmp;
	cout << "continued!" << endl;
	return 0;
}

int main(int argc, char * argv[])
{
	int ret, domid;
	
	domid = client_init();

	/*
	ret = client_net(argv[1], domid);
	if(0 != ret){
		return -1;
	}*/

	wait_for();
	
	map_ring();
	watch();

	return 0;
}
