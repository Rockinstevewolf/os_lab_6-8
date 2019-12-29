#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h> 
#include <chrono>
#include <ctime>
#include <vector>
#include <sys/types.h> 
#include "zmq.h"
using namespace std;
using namespace chrono;

string address = "tcp://localhost:";

typedef struct Message{ //структура для общения узлов
	int command;	//основной запрос
	int period;
	int amount;
	int list[100];
	int sum;
	pid_t id;	//pid процесса созданного calculator
} Message;


Message m_null = {0,0,0,0,0,0};


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
		m_send.id = getpid();
		if(m_back->command == 0){
			m_send.sum = 0;
			int n = m_back->amount;
			cout << "exec " << n << " ";
			for(int i = 0; i < n; i++){
				cout << m_back->list[i] << " ";
				m_send.sum = m_send.sum + m_back->list[i];
			}
			cout << endl;
			send = true;
		}
		else if(m_back->command == 1){
			cout << "heartbeat " << m_back->period << " " << m_back->amount << endl;
			int n = m_back->amount;
			int per = m_back->period;
			for(int i=0; i < n; i++){

				zmq_msg_init_size(&reply, sizeof(Message));
				memcpy(zmq_msg_data(&reply), &m_send, sizeof(Message));
				zmq_msg_send(&reply, controller, 0);
				zmq_msg_close(&reply);

				cout << "Staying Alive" << endl;
				sleep(per);

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
			memcpy(zmq_msg_data(&reply), &m_send, sizeof(Message));
			zmq_msg_send(&reply, controller, 0);
			zmq_msg_close(&reply);
			send = false;
		}
	}

	zmq_close(controller);
	zmq_ctx_destroy(context);
	
	return 0;
}