#ifndef __DNS_RESOLVE_H__
#define __DNS_RESOLVE_H__

#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"
#include <netinet/in.h>

using namespace std;

// resolve the response
struct Response{
	DNSHeader head;
	DNSRecord body;
};

class response{
public:
	DNSHeader header;
	DNSRecord record;
	Response buff;
	string hostname;
	string ip;
	ushort ID;
	response(ushort ID, string hostname){
		this->hostname = hostname;
		this->ID = ID;
	}
	Response data_send(){
        send_response();
        return buff;
    }
private:
	void send_response(){
		header.ID = htons(ID);
		header.QR = true; //response(1)
		header.OPCODE = 0; //standard query(0)
		header.AA = true; // 1 in response
		header.TC = false; //truncate or not
		header.RD = false; //copy from qery
		header.RA = false; //in response
		header.Z = '0';
		if(hostname == "video.cse.umich.edu")
			header.RCODE = '0'; //in response
		else
			header.RCODE = '3'; // not found hostname
		header.QDCOUNT = 0;
		header.ANCOUNT = htons(1);
		header.NSCOUNT = 0; // 0 in all
		header.ARCOUNT = 0; // 0 in all
		if(hostname == "video.cse.umich.edu")
			strcpy(record.NAME, hostname.c_str()); // hostname
		else
			strcpy(record.NAME, "");
		record.TYPE = htons(1); // return A record
		record.CLASS = htons(1); // return ip address
		record.TTL = 0; // no caching
		if(hostname == "video.cse.umich.edu")
			strcpy(record.RDATA, ip.c_str()); // ip address
		else
			strcpy(record.NAME, "");
		record.RDLENGTH = htons(ip.length()); //length of RDATA
		//response
		buff.head = header;
		buff.body = record;
	}
};



// query DNS server
struct Question{
	DNSHeader head;
	DNSQuestion body;
};

class construct{
public:
    DNSHeader header;
    DNSQuestion question;
    Question buff;
    ushort ID;
    string hostname;
    construct(ushort ID, string hostname){
        this->ID = ID;
        this->hostname = hostname;
    }
    Question data_send(){
        query();
        return buff;
    }
    
private:
    void query(){
        header.ID = htons(ID);
        header.QR = false; //query(0)
        header.OPCODE = 0; //standard query(0)
        header.AA = false; // 0 in query
        header.TC = false; //truncate or not
        header.RD = false; //recursive desire
        header.RA = false; //in response
        header.Z = '0';
        header.RCODE = '0'; //in response
        header.QDCOUNT = htons(1);
        header.ANCOUNT = 0;
        header.NSCOUNT = 0; //0 in all
        header.ARCOUNT = 0; // 0 in all
        strcpy(question.QNAME, hostname.c_str());
        question.QTYPE = htons(1); //1 in all request
        question.QCLASS = htons(1); //1 in all request
        // send
        buff.head = header;
        buff.body = question;
    }   
};


#endif