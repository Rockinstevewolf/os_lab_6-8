#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "zmq.h"
using namespace std;

typedef struct Message{ //структура для общения узлов
	int request;	//основной запрос
	int period;
	int amount;
	pid_t id;	//pid процесса созданного calculator
} Message;

string address = "tcp://*:";

void print_menu(){
	cout << "--------------------" << endl
		 << "-	heartbeat [time] [amount]" << endl
		 << "-	menu" << endl
		 << "-	q" << endl
		 << "--------------------" << endl;
}

void Do_request(zmq_msg_t request, void* executor, Message data){
	zmq_msg_init_size(&request, sizeof(Message));
	memcpy(zmq_msg_data(&request), &data, sizeof(Message));
	zmq_msg_send(&request, executor, 0);
	zmq_msg_close(&request);
}

Message* Get_reply(zmq_msg_t reply, void* executor, Message *back){
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, executor, 0);
	back = (Message *) zmq_msg_data(&reply); 
	zmq_msg_close(&reply);
	return back;
}

int main (int argc, char const *argv[]) 
{
	if(argc != 2){
		cout<<"Wrong input...\nyou should write "<<endl;
		return 1;
	}
	void* context = zmq_ctx_new();
	void* executor = zmq_socket(context, ZMQ_REP);

	zmq_msg_t request, reply;

	address += argv[1];
	zmq_bind(executor, address.c_str());
	//system("cat doge.txt");
	cout << "Starting with address " << address.c_str() << endl;

	Message data, *back;

	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, executor, 0);
	pid_t *id = (pid_t *) zmq_msg_data(&reply);
	pid_t main_id = *id;
	zmq_msg_close(&request);

	cout << "Connected id: " << main_id << endl;
	system("cat connected.txt");

	string choice;
	bool working = true;
	bool send = false;
	print_menu();
	while(working)
	{
		cout << "> ";
		cin >> choice;

		if(choice == "heartbeat"){
			cin >> data.period;
			cin >> data.amount;
			send = true;
			data.request = 1;
			for(int i = 0; i < data.amount; i++){
				zmq_msg_init(&reply);
				zmq_msg_recv(&reply, executor, 0);
				bool *alive = (bool *) zmq_msg_data(&reply); 
				zmq_msg_close(&reply);

				Do_request(request, executor, data);
				if(alive){
					cout << "Id: " << main_id << " is Staying Alive..." << endl;
				}
			}

		}
		else if(choice == "menu"){
			print_menu();
		}
		else if(choice == "q"){
			data.request = 2;
			working = false;
			Do_request(request, executor, data);
			cout << "Closing controller..." << endl;
			return 0;
		}
		else{
			cout << "Try again..." << endl;
		}
		if(send){
			Do_request(request, executor, data);
			Get_reply(reply, executor, back);
			send = false;
		}
	}
	zmq_close(executor);
	zmq_ctx_destroy(context);
	return 0;
}