#include<vector>
#include<cstdio>
#include<fstream>
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

class client {
	public:
	client(int _domid, int _sockfd):domid(_domid), sockfd(_sockfd){}

	public:
	int domid;//remote domid
	int sockfd;
	int remote_id;
};

int fd;

vector<client> clients;

fstream log("server.log");

int modify_grant_table(int domid)
{
	int ret;
	ret = ioctl(fd, SVM_GRANT_RING, &domid);
	return ret;
}

void *server_worker(void *arg)
{
	int ret, sockfd, domid;
	pthread_detach(pthread_self());
	
	sockfd = *((int *)arg);
	recv(sockfd, &domid, sizeof(int), NULL);
	cout << "client domid is:" << domid;
	log << "client domid is:" << domid <<endl;
	
	ret = modify_grant_table(domid);
	if(-1 == ret){
		cerr << "grant_table failed!";
		log << "grant_table failed!";
	}

	client tmp(domid, sockfd);
	clients.push_back(tmp);

	int message = ret;
	send(sockfd, &message, sizeof(int), NULL);
	
	return ((void *)0);
}



//network part
void *server_net(void *arg)
{
	int sockfd, client_fd;
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
	int flag;
	size_t length;
	string informaiton;
	
	//creat socket
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket creat failed!");
		exit(1);
	}else{
	}

	//init local_addr
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(SERVER_PORT);
	local_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(local_addr.sin_zero), 0, 8);

	//bind socket with local_addr
	if(bind(sockfd,(struct sockaddr*) &local_addr, sizeof(struct sockaddr)) == -1){
		perror("bind error!");
		exit(1);
	}

	if(listen(sockfd, BACKLOG) == -1){
		perror("Listen error!");
		exit(1);
	}


	while(true){
		socklen_t sin_size = sizeof(struct sockaddr_in);
		if((client_fd = accept(sockfd, (struct sockaddr *) &remote_addr, &sin_size)) == -1){
			perror("Accpet error!");
			continue;
		}

		pthread_t tid_worker;
		pthread_create(&tid_worker, NULL, server_worker, &client_fd);

	}
	
	return ((void *)0);
}

int command(void)
{
	string cmd;
	cout << "Please type the command:" << endl;
	cout << "SVM:";
	while(cin >> cmd){
		write(fd, cmd.c_str(), cmd.length());
		cout << endl << "svm:";
	}

	return 0;	
}

int server_init(void)
{
	int ret;
	//1
	fd = open("/dev/svm", O_RDWR);
	if(0 == fd){
		cerr << "can not open /dev/svm!\n";
		log << "can not open /dev/svm!" << endl;
		return -1;
	}

	//2
	ret = ioctl(fd, SVM_ALLOC_PAGE, NULL);
	if (-1 == ret){
		cerr << "alloc grant page failed!\n";
		log << "alloc grant page failed!" << endl;
		return -1;
	}

	//3
	ret = ioctl(fd, SVM_GET_DOMID, NULL);
	if(-1 == ret){
		cerr << "set domid failed!\n";
		log << "set domid failed!" << endl;
		return -1;
	}


	return ret;
}

int get_domids(void)
{
	int domid_one, domid_two, ret;
	cout << "type domid one:" ;
	cin >> domid_one;
	cout << "type domid two:" ;
	cin >> domid_two;

	cout << "\ndomid is :" << domid_one << " " <<  domid_two << endl;

	ret = modify_grant_table(domid_one);
	if(-1 == ret){
		cerr << "grant table failed with " << domid_one << endl;
		log << "grant table failed with " << domid_one << endl;
	}
	ret = modify_grant_table(domid_two);
	if(-1 == ret){
		cerr << "grant table failed with " << domid_two << endl;
		log << "grant table failed with " << domid_two << endl;
	}
}

int main(int argc, char *argv[])
{
	int ret;
	/* 1 open /dev/svm
	 * 2 alloc grant page
	 * 3 write domid to /vrv/svm
	 */
	ret = server_init();
	if(-1 == ret)
		return -1;

	/*
	pthread_t tid_net; 
	pthread_create(&tid_net, NULL, server_net, NULL);
	*/

	get_domids();

	command();

	return 0;
}
