#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <vector>
#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <chrono>
#include <ctime>
#include <sstream>

using namespace std;

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		cout << "Error: Usage is ./server <listen_port> <alpha>\n";
		return 1;
	}

	int portNum = atoi(argv[1]);
	float alpha = atof(argv[2]);
	// Bind server to sd and set up listen server
	int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in self;
	self.sin_family = AF_INET;
	self.sin_addr.s_addr = INADDR_ANY;
	self.sin_port = htons((u_short) portNum);
	int err = ::bind(sd, (struct sockaddr*) &self, sizeof(self));
	if(err == -1)
	{
		cout << "Error binding the socket to the port number\n";
		return 1;
	}

	err = listen(sd, 10);
	if(err == -1)
	{
		cout << "Error setting up listen queue\n";
		return 1;
	}

	// Set of file descriptors to listen to
	fd_set readSet;
	// Keep track of each file descriptor accepted
	vector<int> fds;

	while(true)
	{
		// Set up the readSet
		FD_ZERO(&readSet);
		FD_SET(sd, &readSet);
		for(int i = 0; i < (int) fds.size(); ++i)
		{
			FD_SET(fds[i], &readSet);
		}


		int maxfd = 0;
		if(fds.size() > 0)
		{
			maxfd = *max_element(fds.begin(), fds.end());
		}
		maxfd = max(maxfd, sd);

		// maxfd + 1 is important
		int err = select(maxfd + 1, &readSet, NULL, NULL, NULL);
		assert(err != -1);

		if(FD_ISSET(sd, &readSet))
		{
			int clientsd = accept(sd, NULL, NULL);
			if(clientsd == -1)
			{
				cout << "Error on accept" << endl;
			}
			else
			{
				fds.push_back(clientsd);
			}
		}

		double T_cur;
		int chunk = 0;
		int bytesRecvd;
		chrono::time_point<chrono::system_clock> start, end; 
        chrono::duration<double> elapsed_seconds;

		for(int i = 0; i < (int) fds.size(); ++i)
		{
			if(FD_ISSET(fds[i], &readSet))
			{
				char buf[1000]= "";
				
				while(1){
					if (chunk == 0){
						start = chrono::system_clock::now();
					}
					cout<<"ss"<<endl;

					bytesRecvd = recv(fds[i], &buf, 1000, 0);
					cout<<buf<<endl;
					cout<<bytesRecvd<<endl;

					if(bytesRecvd < 0){
						cout << "Error recving bytes" << endl;
						cout << strerror(errno) << endl;
						exit(1);
					}
					else if(bytesRecvd == 0){
						cout << "Connection closed" << endl;
						fds.erase(fds.begin() + i);
						end = chrono::system_clock::now();
                		elapsed_seconds = end-start;
						break;
					}
					else
						chunk += bytesRecvd;
				}

                T_cur = chunk/elapsed_seconds.count();
                T_cur = alpha*chunk/elapsed_seconds.count() + (1-alpha)*T_cur; 
                chunk = 0;

			}
		}
	}

}