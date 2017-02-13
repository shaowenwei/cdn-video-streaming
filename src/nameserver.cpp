#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <fstream>
#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"
#include "DNSResolve.h"
#include "Dijkstra.h"

using namespace std;

//read config
class readfile{
public:
    vector< pair<int, string> > CLIENT;
    vector< pair<int, string> > SERVER;
    vector< vector<int> > LINKS;
    int num_nodes;
    int num_links;
    void read_network_config(string filename);
};

void readfile::read_network_config(string filename){
    ifstream tfile;
    tfile.open(filename);
    string output;
    int line = 0;
    num_nodes = 0;
    num_links = 0;
    vector<int> nn1;
    int n1;
    string n2, n3;
    if (tfile.is_open()) {
        while (getline(tfile, output)) {
            line++;
            if(output.find("NUM_NODES: ") != string::npos){
                size_t start1 = output.find("NUM_NODES: ");
                string o = output.substr(start1+11);
                num_nodes = atoi(o.c_str());
            }
            if(output.find("NUM_LINKS: ") != string::npos){
                size_t start2 = output.find("NUM_LINKS: ");
                string n = output.substr(start2+11);
                num_links = atoi(n.c_str());
            }
            if(line > 1 && line < num_nodes + 2){
                char space = ' ';
                int s1 = 0;
                int num_space = 0;
                for(int i = 0; i != output.size(); i++)
                {
                    char c = output[i];
                    if(c == space){
                        num_space++;
                        if (num_space == 1)
                        {
                            n1 = atoi(output.substr(0, i).c_str());
                            s1 = i;
                        }
                        if (num_space == 2){
                            n2 = output.substr(i-6, 6).c_str();
                            n3 = output.substr(i+1, output.size()-i);
                        }
                    }
                    
                }
                if(n2 == "CLIENT"){
                    CLIENT.push_back(make_pair(n1, n3));
                }
                if(n2 == "SERVER"){
                    SERVER.push_back(make_pair(n1, n3));
                }
                
            }
            if(line > num_nodes+2 && line < num_links+num_nodes+3){
                char space = ' ';
                int num_space = 0;
                int s1 = 0;
                nn1.clear();
                for(int i = 0; i < output.size(); i++)
                {
                    
                    char c = output[i];
                    if(c == space){
                        num_space++;
                        if (num_space == 1){
                            nn1.push_back(atoi(output.substr(0, i).c_str()));
                            s1 = i;
                        }
                        if (num_space==2){
                            nn1.push_back(atoi(output.substr(s1+1, i-2).c_str()));
                            nn1.push_back(atoi(output.substr(i+1, output.size()-i).c_str()));
                        }
                    }
                }
                LINKS.push_back(nn1);
            }
        }
    }
    tfile.close();
}

// using Dijkstra to find server's ip
string distance(string client_ip, string filename){
    readfile read;
    read.read_network_config(filename);
    
    
    int V = read.num_nodes;
    Graph g(V);
    int client = -1;
    
    vector<vector<int>> vec = read.LINKS;
    for(int i = 0; i != vec.size(); i++)
    {
        g.addEdge(vec[i][0], vec[i][1], vec[i][2]);
    }
    
    vector<int> servers;
    vector< pair<int, string> > tmp = read.SERVER;
    for(int i = 0; i != tmp.size(); i++){
        servers.push_back(tmp[i].first);
    }
    
    vector< pair<int, string> > tm = read.CLIENT;
    for(int i = 0; i != tm.size(); i++){
        if(tm[i].second == client_ip){
            client = tm[i].first;
        }
    }
    
    int s = g.shortestPath(client, servers);
    string ip = "";
    for(int i = 0; i != tmp.size(); i++){
        if(tmp[i].first == s){
            ip = tmp[i].second;
        }
    }
    return ip;
}





int main(int argc, char* argv[])
{
	if(argc <= 2)
	{
		cout << "Error: Usage is ./nameserver <log> <port> <geography_based> <servers>\n";
		return 1;
	}

	char *log_path = argv[1];
	string log_name = log_path;
	log_name = log_name + "DNS_log.txt";
	ofstream logfile;
	logfile.open(log_name,fstream::app|fstream::out);

	int portNum = atoi(argv[2]);
	int geo = atoi(argv[3]); // 1 distance

	char *servers_name = argv[4];
	string server_filename = servers_name;

	int len = 0;
	int index = 0;
	vector<string> ip_list;

	if(geo == 0)
	{
		readfile read;
		read.read_network_config(server_filename);
		vector< pair<int, string> > server_list = read.SERVER;
		index = 0;
		for(int i = 0; i != server_list.size(); ++i){
			string ip = server_list[i].second;
			ip_list.push_back(ip);
		}
		len = ip_list.size();
	}


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
	vector<string> fds_ip;

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
			struct sockaddr_in their_addr;
			socklen_t addr_size = sizeof(their_addr);
			int clientsd = accept(sd, (struct sockaddr *)&their_addr, &addr_size);
			if(clientsd == -1)
			{
				cout << "Error on accept" << endl;
			}
			else
			{
				fds.push_back(clientsd);
				char s[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, (struct sockaddr *)&their_addr.sin_addr.s_addr, s, sizeof s);
				fds_ip.push_back(s);
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

					response res(1, hostname);
					if(geo == 0)
					{
						res.ip = ip_list[index%len];
						cout << res.hostname << endl;
						logfile << fds_ip[i] << " " << res.hostname << " " << ip_list[index%len] << " " <<endl;
						index++;
					}
					else
					{
    					string sip = distance(fds_ip[i], server_filename);
    					res.ip = sip;
						cout << res.hostname << endl;
						logfile << fds_ip[i] << " " << res.hostname << " " << sip << " " <<endl;
					}
					Response res_data = res.data_send();

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
	logfile.close();
}