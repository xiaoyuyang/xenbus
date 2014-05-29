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

int client_init()
{
	int domid;
	fd = open("/dev/xa", O_RDWR);
	domid = ioctl(fd, XA_DOMAIN_NUMBER, NULL);
	cout << "domid is:" << domid << endl;
	return domid;
}

int map_ring()
{
	int ret;
	cout << "call ioctl in map_ring" << endl;
	return ioctl(fd, XA_MAP_RING, NULL);
}

int watch()
{
	cout << "call ioctl in watch" << endl;
	return ioctl(fd, XA_WATCH, NULL);
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

	if(wait_for() == -1){
		cerr << "wait_for failed.\n";
		return -1;	
	}
	
	ret = map_ring();
	if(ret == -1){
		cerr << "map_ring failed.\n";
		return -1;
	}

	if(watch() == -1){
		cerr << "watch failed.\n";
		return -1;
	}

	return 0;
}
