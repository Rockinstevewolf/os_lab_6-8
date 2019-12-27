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
	int request;	//основной запрос
	int period;
	int amount;
	pid_t id;	//id процесса
} Message;

void Get_request(zmq_msg_t request, void* controller, Message data){
	zmq_msg_init_size(&request, sizeof(Message));
	memcpy(zmq_msg_data(&request), &data, sizeof(Message));
	zmq_msg_send(&request, controller, 0);
	zmq_msg_close(&request);
}

Message* Do_reply(zmq_msg_t reply, void* controller, Message *back){
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, controller, 0);
	back = (Message *) zmq_msg_data(&reply); 
	zmq_msg_close(&reply);
	return back;
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
	pid_t id = getpid();
	zmq_msg_t request, reply;
	Message *Get;

	cout << "my id is " << id << endl;
	zmq_msg_init_size(&reply, sizeof(pid_t));
	memcpy(zmq_msg_data(&reply), &id, sizeof(pid_t));
	zmq_msg_send(&reply, controller, 0);
	zmq_msg_close(&reply);
	cout << "No problems detected" << endl;

	bool working = true;
	bool send = false;
	while(working)
	{
		zmq_msg_init(&request);
		zmq_msg_recv(&request, controller, 0);
		Get = (Message *)zmq_msg_data(&request);
		zmq_msg_close(&request);

		cout << "--> " << Get->request << endl;

		if(Get->request == 1){
			int n = Get->amount;
			bool tell = true;
			for(int i=0; i<n; i++){
				cout << "Staying Alive" << endl;


				sleep(Get->period);

				zmq_msg_init_size(&request, sizeof(bool));
				memcpy(zmq_msg_data(&request), &tell, sizeof(bool));
				zmq_msg_send(&request, controller, 0);
				zmq_msg_close(&request);

				zmq_msg_init(&request);
				zmq_msg_recv(&request, controller, 0);
				Get = (Message *)zmq_msg_data(&request);
				zmq_msg_close(&request);
			}
			send = true;
		}
		else if(Get->request == 2){
			working = false;
			cout << "Closing calculator..." << endl;
			return 0;
			send = true;
		}
		else{
			cout << "Try again..." << endl;
		}
		if(send){
			zmq_msg_init_size(&request, sizeof(Message));
			memcpy(zmq_msg_data(&request), &Get, sizeof(Message));
			zmq_msg_send(&request, controller, 0);
			zmq_msg_close(&request);

		}
	}

	zmq_close(controller);
	zmq_ctx_destroy(context);
	
	return 0;
}