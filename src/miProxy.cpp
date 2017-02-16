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
#include <cerrno>
#include <fstream>
#include <time.h>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>
#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"
#include "DNSResolve.h"

using namespace std;

#define packet_len 1000

class Len{
public:
    int header_length(string s){
        size_t found = s.find("\r\n\r\n");
        string sub = s.substr(0, found+4);
        return (int) sub.length();
    }
    
    int content(string s){
        size_t found = s.find("Content-Length: ");
        string sub = s.substr(found+16);
        size_t f = sub.find("\r\n");
        sub = sub.substr(0,f);
        int res = atoi(sub.c_str());
        return res;

    }
};

class Modify{
public:
    string findf4m(string s){
        size_t start = s.find(".f4m");
        string in = "_nolist";
        s.insert(start, in);
        return s;
    }
    
    string bitrate_modify(string s, int bitrate){
        size_t start = s.find("Seg");
        string sub = s.substr(0, start);
        int end = 0;
        string res = "";
        for(int i = sub.length() - 1; i != -1; i--){
            if(sub[i] == '/'){
                end = i;
                break;
            }
        }
        string bit = to_string(bitrate);
        s.replace(end+1, start-end-1, bit);
        return s;
    }
};


class Chunk{
	public:
		int seg_num(string s)
		{
			size_t found = s.find("Seg");
			if(found != string::npos) 
			{
				string sub = s.substr(found + 3);
				size_t f = sub.find("-");
				sub = sub.substr(0,f);
				int seg = atoi(sub.c_str());
				//cout << "Seg:" << seg <<endl;
				return seg;
			}
			else return 0;

			
		}

		int frag_num(string s)
		{
			size_t found = s.find("Frag");
			if(found != string::npos)
			{
				string sub = s.substr(found + 4);
				size_t f = sub.find(" ");
				sub = sub.substr(0,f);
				int frag = atoi(sub.c_str());
				//cout << "frag:" << frag <<endl;
				return frag;
			}
			else return 0;
		}

};

vector<int> getBitrate(){
    vector<int> res;
    ifstream myReadFile;
    myReadFile.open("f4m.txt");
    string output;
    if (myReadFile.is_open()) {
        while (!myReadFile.eof()) {
            myReadFile >> output;
            if(output.find("bitrate=") != string::npos){
                size_t start = output.find("bitrate=");
                output = output.substr(start+9);
                output = output.substr(0, output.size()-1);
                res.push_back(atoi(output.c_str()));
            }
        }
    }
    myReadFile.close();
    return res;
}

//get web server ip address 
char* DNSGet(int dnssd,ushort dns_id){
	char* ipserver;
	construct con(dns_id, "video.cse.umich.edu");
	Question query = con.data_send();
	int bytesSent = send(dnssd, &query, sizeof(query), 0);
	if(bytesSent <= 0)
	{
		cout << "Error sending stuff to server" << endl;
	}
	cout << "Send to server: " << bytesSent << endl;
	
	Response resp;
	int bytesRecv = recv(dnssd, &resp, sizeof(resp), 0);
	if(bytesRecv > 0)
	{
		cout << "Received from client: " << bytesRecv << endl;
	}
	else
	{
		exit(1);
	}
	if(resp.head.RCODE == '3'){
		cout<<"can not found hostname"<<endl;
		exit(1);
	}
	else{
		cout<<resp.body.RDATA<<endl;
		ipserver = resp.body.RDATA;
		//portNumServer = 80;
	}
	return ipserver;
}




int dns_usage = 0;

int main(int argc, char* argv[])
{
	char *log_path = argv[1];
	float alpha = atoi(argv[2]);
	string log_name = log_path;
	log_name = log_name+"log.txt";
	cout<<log_name<<endl;
	int portNum = atoi(argv[3]);
	vector<int> get_bitrate;
	ofstream logfile;
	logfile.open(log_name,fstream::app|fstream::out);
	char *ipserver;
	int portNumServer = 80;
	char *DNSip;
	int DNSportNum;

	if(argc == 7)
	{
		//use www-ip address
		ipserver = argv[6];
		portNumServer = 80;
		dns_usage = 0;
	}
	else if(argc == 6)
	{
		//use dns server
		DNSip = argv[4];
		DNSportNum = atoi(argv[5]);
		dns_usage = 1;
	}
	else
	{
		cout << "Error: Usage is ./miProxy <log> <alpha> <listen_port> <dns_ip> <dns_port> <www-ip>\n";
		return 1;
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

	int dnssd;
	// socket connect to dns
	if(dns_usage == 1){
		cout << "dns_usage = true" << endl;
		dnssd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(dnssd == -1)
		{
			std::cout << "Error creating dns server socket\n";
			exit(1);
		}

		struct sockaddr_in dns;
		memset(&dns, 0, sizeof(dns));
		dns.sin_family = AF_INET;
		dns.sin_port = htons((u_short) DNSportNum);
		struct hostent* sp = gethostbyname(DNSip);
		memcpy(&dns.sin_addr, sp->h_addr, sp->h_length);
		int err = connect(dnssd, (sockaddr*) &dns , sizeof(dns));
		if(err == -1)
		{
			cout << "Error on connect\n";
			exit(1);
		}
	}


	// Set of file descriptors to listen to
	fd_set readSet;
	// Keep track of each file descriptor accepted
	vector<int> fds;
	vector<int> fds_dns;
	vector<string> fds_ip;

	chrono::time_point<chrono::system_clock> start, end; 
	chrono::duration<double> elapsed_seconds;
	int seg =  0;
	int frag = 0;
	int pre_seg = 0;
	int chunk = 0;
	double T_cur = 0;
	double throughput = 0;
	double bitrate = 0;
	ushort dns_id=0;
	//create log file <duration> <tput> <avg-tput> <bitrate> <server-ip> <chunkname>

	while(true)
	{
		cout<<"enter while loop"<<endl;		
		
		FD_ZERO(&readSet); // Set up the readSet
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

		int server_sd;

		if(FD_ISSET(sd, &readSet))
		{
			int clientsd = accept(sd, NULL, NULL);
			if(clientsd == -1)
			{
				cout << "Error on accept" << endl;
			}
			else
			{
				// get web server ip address for each connection
				fds.push_back(clientsd);
				string dns_server_ip = DNSGet(dnssd,dns_id);
				fds_ip.push_back(dns_server_ip);

				dns_id++;
				if(dns_id > 65534) dns_id = 0;
				cout<<"DNS_ID = "<<dns_id<<endl;

				// connect to web server according to ip which dns gave
				server_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if(server_sd == -1)
				{
					std::cout << "Error creating server socket\n";
					exit(1);
				}
				struct sockaddr_in server;
				memset(&server, 0, sizeof(server));
				server.sin_family = AF_INET;
				server.sin_port = htons((u_short) portNumServer);
				string get_last = dns_server_ip;
				ipserver = new char[get_last.size() + 1];
				memcpy(ipserver, get_last.c_str(), get_last.size() + 1);
				cout<<"ipserver: "<<ipserver<<endl;

				server.sin_addr.s_addr = inet_addr(ipserver);
				int err1 = connect(server_sd, (sockaddr*) &server, sizeof(server));
				if(err1 == -1)
				{
					cout << "Error on connect to web server\n";
					exit(1);
				}
				fds_dns.push_back(server_sd);
			}
		}


		for(int i = 0; i < (int) fds.size(); ++i)
		{
			cout<<"enter for loop"<<endl;
			if(FD_ISSET(fds[i], &readSet))
			{	

					int serversd = fds_dns[i];
					string ipserver = fds_ip[i];

					char buf[packet_len] = "";
					//recv request from browser
					cout<<"i: "<<fds[i]<<endl;
					int bytesRecvd = recv(fds[i], &buf, packet_len, 0);
					if(bytesRecvd < 0)
					{
						cout << "Error recving bytes" << endl;
						exit(1);
					}
					else if(bytesRecvd == 0)
					{
						cout << "Connection closed" << endl;
						fds.erase(fds.begin() + i);
						close(serversd);
						fds_dns.erase(fds_dns.begin() + i);
						//break;
					}
					else{
						cout<< "Received from browser:\n"<<buf<<endl;
					}

					Chunk find_num;



					//send request to server
					string buff = buf;

					pre_seg = seg;
					cout << "pre-Seg:" << pre_seg <<endl;
					seg = find_num.seg_num(buff);
					cout << "Seg:" << seg <<endl;
					frag = find_num.frag_num(buff);
					cout << "frag:" << frag << endl;
					if(seg != 0 && frag != 0){
						if(seg == (pre_seg + 1)) {
							end = chrono::system_clock::now();
		        			elapsed_seconds = end-start;
		        			throughput = chunk * 8/(elapsed_seconds.count() * 1000);
		                    T_cur = alpha * throughput + (1-alpha) * T_cur;
		                    cout << "duration: " << elapsed_seconds.count() << "s" << endl;
		                    cout << "tput: " << throughput << "Kbps" << endl;
		                    cout << "avg-tput: " << T_cur <<"Kbps"<< endl;
		                    cout << "Chunk: " << chunk << endl;
		                    bitrate = T_cur/1.5;
		                    cout << "bitrate: " << bitrate << endl;
   							for(int i = get_bitrate.size()-1; i != -1; --i){
   								if(get_bitrate[i] < bitrate){
   									bitrate = get_bitrate[i];
   									break;
   								}
   							}
   							cout << "bitrate chosen:" << bitrate << endl;
                			start = chrono::system_clock::now();

						}
						logfile <<" "<< elapsed_seconds.count() <<" "<< throughput <<" "<<T_cur<<" "<<bitrate<<" "<<ipserver<<" "<<"Seg"<<seg<<"-Frag"<<frag<<endl;
						//modify header
						if(pre_seg != 0 && pre_seg != 1){
							Modify modify;
							cout << "mo-bitrate:" << bitrate << endl;
							buff = modify.bitrate_modify(buff, bitrate);
							cout << "after modify: \n" << buff << endl;
						}
					}

					// check if request .f4m file change to _nolist.f4m
					bool no_list = false;
					string s_old = "";
					
					if(buff.find(".f4m") != string::npos){
						cout<<"detect .f4m file"<<endl;
						Modify modify;
						s_old = buff;
						buff = modify.findf4m(buff);
						cout<<"f4mbuf:"<<buff<<endl;
					    	no_list = true;

					}





					//forward request to web server
					int bytesSent = send(serversd, buff.c_str(), buff.length(), 0);
					if(bytesSent <= 0){
						cout << "Error sending to web server" << endl;
						exit(1);
					}
					else{
						cout << "Send to web server:\n" << buff << endl;
					}




					// receive response from web server(header part)
					char buf_r[packet_len];
					Len len;
					int remain = 0;
					int bytesRecv = recv(serversd, &buf_r, packet_len, 0);
					string s = "";
					int total_bytes = 0;
					
					if(bytesRecv < 0){
						cout << "Error receiving from web server:\n" << endl;
						cout << "Something went wrong! errno " << errno << ": ";
        				cout << strerror(errno) << endl;
						exit(1);
					}
					else{
						chunk += bytesRecv;
						cout << "Received from web server:\n" << buf_r <<endl;

						//compute length
						s = buf_r;						

						int header = len.header_length(s);
						int content = len.content(s);
						remain = content - (bytesRecv - header);
						//cout<<"header length: "<<header<<"\nbody length: "<<(bytesRecv - header)<<"\ncontent length: "<<content<<"\nremain: "<<remain<<endl;
						//cout <<"bytesRecv: "<<bytesRecv<<endl;
						total_bytes = total_bytes+bytesRecv;
					}




					//send response to browser
					int bytesSend = send(fds[i], buf_r, bytesRecv, 0);
					if(bytesSend <= 0){
						cout << "Error sending to browser" << endl;
						exit(1);
					}
					else{
						total_bytes = total_bytes + bytesSend;
						cout << "Send back to browser: " << total_bytes << " bytes" << endl;
						//cout << "Send back to browser: "<<endl;
					}





					while(remain != 0){

						//recv response from webserver(body part)
						char buf_l[packet_len] = "";
 
						bytesRecv = recv(serversd, &buf_l, packet_len, 0);
						if(bytesRecv < 0){
							cout << "Error receiving from web server:\n" << endl;
							cout << "Something went wrong! errno " << errno << ": ";
	    				 	cout << strerror(errno) << endl;
							exit(1);
						}
						else{
							chunk += bytesRecv;
							remain = remain - bytesRecv;
							cout << "byte receive: " << bytesRecv << endl;
							cout << "remain: " << remain << endl;
							//cout << "Received from web server:\n" << buf_l << endl;

							//send response to browser
							bytesSend = send(fds[i], buf_l, bytesRecv, 0);
							if(bytesSend <= 0){
								cout << "Error sending to browser" << endl;
								exit(1);
							}
							else{
								total_bytes = total_bytes + bytesSend;
								cout << "Send back to browser: " << total_bytes << " bytes" << endl;
								//cout << "Send back to browser: "<<endl;
							}
						}
					}

					//cout<<"no list"<<no_list<<endl;
					//if request .f4m, proxy request .f4m and store it locally
					if(no_list == true){
						cout<<"no_list"<<endl;
						//send
						int bytesS = send(serversd, s_old.c_str(), s_old.length(), 0);
						cout<<"f4m"<<bytesS<<endl;
						if(bytesS <= 0){
							cout << "Error sending .f4m request to web server" << endl;
							exit(1);
						}
						else{
							//cout << "Send .f4m request to web server:\n" << s_old.c_str() << endl;
						}

						ofstream myfile;
						myfile.open ("f4m.txt");


						//revc header
						char buf_h[packet_len];
						int rem = 0;
						string s1 = "";
						int bytesR = recv(serversd, &buf_h, packet_len, 0);
						if(bytesR < 0){
							cout << "Error receiving .f4m from web server:\n" << endl;
							exit(1);
						}
						else{
							myfile << buf_h;
							//cout << "byte receive: " << bytesR << endl;
							s1 = buf_h;
							int header = len.header_length(s1);
							int content = len.content(s1);
							rem = content - (bytesR - header);
							cout << "remain: " << rem << endl;
						}

						//recv whole response
						while(rem > 0){
							bytesR = recv(serversd, &buf_h, packet_len, 0);
							if(bytesR < 0){
								cout << "Error receiving from web server:\n" << endl;
								exit(1);
							}
							else{
								rem = rem - bytesR;
								//cout << "byte receive: " << bytesR << endl;
								//cout << "remain: " << rem << endl;
								myfile << buf_h;
							}
						}
						myfile.close();
						get_bitrate = getBitrate();
						sort(get_bitrate.begin(), get_bitrate.end());
						//no_list = false;
					}
				//} 
			}
		}
	}
	logfile.close();
}
