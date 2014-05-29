#include<vector>
#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<cstring>
#include<pthread.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include"xenbus_demo.h"

using namespace std;

int fd;

int modify_grant_table(int domid)
{
	int ret;
	ret = ioctl(fd, SVM_GRANT_RING, domid);
	return ret;
}

int command(void)
{
	string cmd;
	cout << "Please type the command:" << endl;
	cout << "SVM:";
	while(cin >> cmd){
		write(fd, cmd.c_str(), cmd.length());
		cout << "SVM:";
	}

	return 0;	
}

int server_init(void)
{
	int ret;

	fd = open("/dev/svm", O_RDWR);
	if(0 == fd){
		cerr << "can not open /dev/svm!\n";
		return -1;
	}

	ret = ioctl(fd, SVM_ALLOC_PAGE, NULL);
	if (-1 == ret){
		cerr << "alloc grant page failed!\n";
		return -1;
	}

	ret = ioctl(fd, SVM_GET_DOMID, NULL);
	if(-1 == ret){
		cerr << "set domid failed!\n";
		return -1;
	}

	return ret;
}

int get_domids(void)
{
	int domid_one, domid_two, ret;
	
	//register dom one
	cout << "type domid one:" ;
	cin >> domid_one;
	ret = modify_grant_table(domid_one);
	if(-1 == ret){
		cerr << "grant table failed with " << domid_one << endl;
	}
	cout << "dom:" << domid_one << " granted." << endl;

	cout << "type domid two:" ;
	cin >> domid_two;
	ret = modify_grant_table(domid_two);
	if(-1 == ret){
		cerr << "grant table failed with " << domid_two << endl;
	}
	cout << "dom:" << domid_two << " granted." << endl;
	
	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	
	ret = server_init();
	if(-1 == ret)
		return -1;

	get_domids();

	command();

	return 0;
}
