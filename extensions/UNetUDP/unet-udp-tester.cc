#include <cstdlib>
#include <errno.h>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <cc++/socket.h>
#include "UDPPacket.h"
// --------------------------------------------------------------------------
static struct option longopts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "send", required_argument, 0, 's' },
	{ "receive", required_argument, 0, 'r' },
	{ "proc-id", required_argument, 0, 'p' },
	{ "node-id", required_argument, 0, 'n' },
	{ "send-pause", required_argument, 0, 'x' },
	{ "timeout", required_argument, 0, 't' },
	{ "data-count", required_argument, 0, 'c' },
	{ "disable-broadcast", no_argument, 0, 'b' },
	{ "show-data", no_argument, 0, 'd' },
	{ "check-lost", no_argument, 0, 'l' },
	{ "verbode", required_argument, 0, 'v' },
	{ "num-cycles", required_argument, 0, 'z' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetUDP;
// --------------------------------------------------------------------------
enum Command
{
	cmdNOP,
	cmdSend,
	cmdReceive
};
// --------------------------------------------------------------------------
static bool split_addr( const string& addr, string& host, ost::tpport_t& port )
{
	string::size_type pos = addr.rfind(':');
	if(  pos != string::npos )
	{
		host = addr.substr(0,pos);
		string s_port(addr.substr(pos+1,addr.size()-1));
		port = UniSetTypes::uni_atoi(s_port.c_str());
		return true;
	}
	
	return false;
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	int optindex = 0;
	int opt = 0;
	Command cmd = cmdNOP;
	int verb = 0;
	std::string addr = "";
	ost::tpport_t port=0;
	int usecpause = 2000000;
	timeout_t tout = TIMEOUT_INF;
	bool broadcast = true;
	int procID = 1;
	int nodeID = 1;
	size_t count = 50;
	bool lost = false;
	bool show = false;
	int ncycles = -1;

	while( (opt = getopt_long(argc, argv, "hs:c:r:p:n:t:x:blvdz:",longopts,&optindex)) != -1 ) 
	{
		switch (opt) 
		{
			case 'h':	
				cout << "-h|--help                - this message" << endl;
				cout << "[-s|--send] host:port    - Send message." << endl;
				cout << "[-c|--data-count] num    - Send num count of value. Default: 50." << endl;
				cout << "[-r|--receive] host:port - Receive message." << endl;
				cout << "[-p|--proc-id] id        - Set packet header. From 'procID'. Default: 1" << endl;
				cout << "[-n|--node-id] id        - Set packet header. From 'nodeID'. Default: 1" << endl;
				cout << "[-t|--timeout] msec      - timeout for receive. Default: 0 msec (waitup)." << endl;
				cout << "[-x|--send-pause] msec   - pause for send packets. Default: 200 msec." << endl;
				cout << "[-b|--disable-broadcast] - Disable broadcast mode." << endl;
				cout << "[-l|--check-lost]        - Check the lost packets." << endl;
				cout << "[-v|--verbose]           - verbose mode." << endl;
				cout << "[-d|--show-data]         - show receive data." << endl;
				cout << "[-z|--num-cycles] num    - Number of cycles of exchange. Default: -1 - infinitely." << endl;
				cout << endl;
			return 0;

			case 'r':	
				cmd = cmdReceive;
				addr = string(optarg);
			break;

			case 's':
				addr = string(optarg);
				cmd = cmdSend;
			break;
			
			case 't':
				tout = UniSetTypes::uni_atoi(optarg);
			break;	

			case 'x':
				usecpause = UniSetTypes::uni_atoi(optarg)*1000;
			break;	
			
			case 'c':
				count = UniSetTypes::uni_atoi(optarg);
			break;	

			case 'p':
				procID = UniSetTypes::uni_atoi(optarg);
			break;	

			case 'n':
				nodeID = UniSetTypes::uni_atoi(optarg);
			break;	
			
			case 'b':
				broadcast = false;
			break;	

			case 'd':
				show = true;
			break;	
			
			case 'l':
				lost = true;
			break;	
			
			case 'v':	
				verb = 1;
			break;
			
			case 'z':
				ncycles = UniSetTypes::uni_atoi(optarg);
			break;
			
			case '?':
			default:
				cerr << "? argumnet" << endl;
				return 0;
		}
	}
	
	if( cmd == cmdNOP )
	{
		cerr << "No command... Use -h for help" << endl;
		return -1;
	}

	if( tout < 0 )
		tout = TIMEOUT_INF;

	ost::Thread::setException(ost::Thread::throwException);
	
	try
	{
		string s_host;
		if( !split_addr(addr,s_host,port) )
		{
			cerr << "(main): Unknown 'host:port' for '" << addr << "'" << endl;
			return 1;
		}
		
		if( verb )
		{
			cout << " host=" << s_host 
				<< " port=" << port
				<< " timeout=";
			if( tout == TIMEOUT_INF )
				cout << "Waitup";
			else
				cout << tout;

			cout << " msecpause=" << usecpause/1000 
				<< endl;
		}

		ost::IPV4Host host(s_host.c_str());
//		udp.UDPTransmit::setBroadcast(broadcast);

		switch( cmd )
		{
			case cmdReceive:
			{
				ost::UDPDuplex udp(host,port);
				
//				char buf[UniSetUDP::MaxDataLen];
				UniSetUDP::UDPMessage pack;
				UniSetUDP::UDPPacket buf;
				unsigned long prev_num=1;

				int nc = 1;
				if( ncycles > 0 )
					nc = ncycles;

				while( nc )
				{
					try
					{
						if( !udp.isInputReady(tout) )
						{
							cout << "(recv): Timeout.." << endl;
							continue;
						}
						
						size_t ret = udp.UDPReceive::receive( &(buf.data), sizeof(buf.data) );
						size_t sz = UniSetUDP::UDPMessage::getMessage(pack,buf);
						if( sz == 0 )
						{
							cerr << "(recv): FAILED header ret=" << ret 
								<< " sizeof=" << sz<< endl;
							continue;
						}
		
						if( lost )
						{
							if( prev_num != (pack.num-1) )
								cerr << "WARNING! Incorrect sequence of packets! current=" << pack.num
									<< " prev=" << prev_num << endl;
							
							prev_num = pack.num;
						}

						if( verb )
							cout << "receive OK:  "
								 << " bytes: " << ret << endl;

						if( show )
							cout << "receive data: " << pack << endl;
					}
					catch( ost::SockException& e )
					{
						cerr << "(recv): " << e.getString() << " (" << addr << ")" << endl;
					}
					catch( ... )
					{
						cerr << "(recv): catch ..." << endl;
					}
			
					if( ncycles > 0 )
					{
						nc--;
						if( nc <=0 )
							break;
					}
				}
			}
			break;
	
			case cmdSend:
			{	
				ost::UDPSocket* udp;
      			if( !broadcast )
            		udp = new ost::UDPSocket();
        		else
			        udp = new ost::UDPBroadcast(host,port); 

				UniSetUDP::UDPMessage mypack;
				mypack.nodeID = nodeID;
				mypack.procID = procID;

				for( size_t i=0; i < count; i++ )
				{
					UDPAData d(i,i);
					mypack.addAData(d);
				}
				
				for( unsigned int i=0; i < count; i++ )
					mypack.addDData(i,i);

				udp->setPeer(host,port);
				unsigned long packetnum = 0;

				UniSetUDP::UDPPacket s_buf;
				
				int nc = 1;
				if( ncycles > 0 )
					nc = ncycles;

				while( nc )
				{
					mypack.num = packetnum++;
					if( packetnum > UniSetUDP::MaxPacketNum )
						packetnum = 1;

					try
					{
						if( udp->isPending(ost::Socket::pendingOutput,tout) )
						{
							mypack.transport_msg(s_buf);

							if( verb )
								cout << "(send): to addr=" << addr << " d_count=" << mypack.dcount 
									<< " a_count=" << mypack.acount << " bytes=" << s_buf.len << endl;
 							
							size_t ret = udp->send((char*)&s_buf.data, s_buf.len);
							if( ret < s_buf.len )
        						cerr << "(send): FAILED ret=" << ret << " < sizeof=" << s_buf.len << endl;
						}
					}
					catch( ost::SockException& e )
					{
						cerr << "(send): " << e.getString() << " (" << addr << ")" << endl;
					}
					catch( ... )
					{
						cerr << "(send): catch ..." << endl;
					}

					if( ncycles > 0 )
					{
						nc--;
						if( nc <=0 )
							break;
					}
					
					usleep(usecpause);
				}
			}
			break;

			default:
				cerr << endl << "Unknown command: '" << cmd << "'. Use -h for help" << endl;
				return -1;
			break;
		}
	}
	catch( std::exception& e )
	{
		cerr << "(main): " << e.what() << endl;
	}
	catch( ... )
	{
		cerr << "(main): catch ..." << endl;
		return 1;
	}
	
	return 0;
}
// --------------------------------------------------------------------------
