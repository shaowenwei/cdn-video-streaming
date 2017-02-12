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
#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"
#include "DNSResolve.h"

using namespace std;

int main(int argc, char* argv[])
{
	if(argc <= 2)
	{
		cout << "Error: Usage is ./nameserver <listen_port> <ip address 1> <ip address 2> <ip address 3>...\n";
		return 1;
	}

	int portNum = atoi(argv[1]);
	vector<string> ip_list;
	int index = 0;
	for(int i = 2; i != argc; i++){
		string ip = argv[i];
		ip_list.push_back(ip);
	}
	int len = ip_list.size();


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

		for(int i = 0; i < (int) fds.size(); ++i)
		{
			if(FD_ISSET(fds[i], &readSet))
			{
				while(1){

					//recv the DNS request
					Question query;
					int bytesRecvd = recv(fds[i], &query, sizeof(query), 0);
					if(bytesRecvd < 0)
					{
						cout << "Error recving request" << endl;
						cout << strerror(errno) << endl;
						exit(1);
					}
					else if(bytesRecvd == 0)
					{
						cout << "Connection closed" << endl;
						fds.erase(fds.begin() + i);
						break;
					}
					else{
						cout << "recv DNS request from proxy: " << bytesRecvd << endl;
					}
					string hostname = query.body.QNAME;
					response res(1, hostname, ip_list[index%len]);
					cout<<res.hostname<<endl;
					Response res_data = res.data_send();
					index++;

					//send back the response
					int bytesSend = send(fds[i], &res_data, sizeof(res_data), 0);
					if(bytesSend <= 0){
						cout << "Error sending DNS response to proxy" << endl;
						exit(1);
					}
					else{
						cout << "Send DNS response to proxy:\n" << bytesSend << endl;
					}

				}
			}
		}
	}
}