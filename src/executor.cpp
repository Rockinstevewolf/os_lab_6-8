#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h> 
#include <chrono>
#include <ctime>
#include <sys/types.h> 
#include "zmq.h"
using namespace std;
using namespace chrono;

string address = "tcp://localhost:";

typedef struct Message{ //структура для общения узлов
	int command;	//основной запрос
	int period;
	int amount;
	pid_t id;	//id процесса
} Message;

/*
Message* Get_request(zmq_msg_t request, void* controller){
	zmq_msg_init(&request);
	zmq_msg_recv(&request, controller, 0);
	Message *m_back = (Message *) zmq_msg_data(&request); 
	zmq_msg_close(&request);
	return m_back;
}*/

void Do_reply(zmq_msg_t reply, void* controller, Message m_send){
	zmq_msg_init_size(&reply, sizeof(Message));
	memcpy(zmq_msg_data(&reply), &reply, sizeof(Message));
	zmq_msg_send(&reply, controller, 0);
	zmq_msg_close(&reply);
}

int main (int argc, char const *argv[]) 
{
	if(argc != 2){
		cout << "Wrong input...\nyou should write "<< endl;
		return 1;
	}
	address += argv[1];
	void* context = zmq_ctx_new();
	void* controller = zmq_socket(context, ZMQ_REQ);

	if(zmq_connect(controller, address.c_str()) == 0){
		cout << "Connecting to " << address.c_str() << endl;
	}
	zmq_msg_t request, reply;
	Message *m_back, m_send;

	m_send.id = getpid();

	cout << "my id is " << m_send.id << endl;

	zmq_msg_init_size(&reply, sizeof(Message));
	memcpy(zmq_msg_data(&reply), &m_send, sizeof(Message));
	zmq_msg_send(&reply, controller, 0);
	zmq_msg_close(&reply);

	cout << "No problems detected" << endl;

	bool working = true;
	bool send = false;
	while(working)
	{
		cout << "---request--waiting---" << endl;

		zmq_msg_init(&request);
		zmq_msg_recv(&request, controller, 0);
		m_back = (Message *) zmq_msg_data(&request); 
		zmq_msg_close(&request);

		cout << "--> " << m_back->command << endl;
		if(m_back->command == 1){
			cout << "heartbeat " << m_back->period << " " << m_back->amount << endl;
			int n = m_back->amount;
			for(int i=0; i < n; i++){

				zmq_msg_init_size(&reply, sizeof(Message));
				memcpy(zmq_msg_data(&reply), &reply, sizeof(Message));
				zmq_msg_send(&reply, controller, 0);
				zmq_msg_close(&reply);

				cout << "Staying Alive" << endl;
				sleep(m_back->period);

				zmq_msg_init(&request);
				zmq_msg_recv(&request, controller, 0);
				m_back = (Message *) zmq_msg_data(&request); 
				zmq_msg_close(&request);
			}
			send = true;
		}
		else if(m_back->command == 2){
			working = false;
			cout << "Closing calculator..." << endl;
			break;
		}
		else{
			cout << "Try again..." << endl;
		}
		if(send){
			zmq_msg_init_size(&reply, sizeof(Message));
			memcpy(zmq_msg_data(&reply), &reply, sizeof(Message));
			zmq_msg_send(&reply, controller, 0);
			zmq_msg_close(&reply);
			send = false;
		}
	}

	zmq_close(controller);
	zmq_ctx_destroy(context);
	
	return 0;
}