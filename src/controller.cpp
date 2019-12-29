#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "zmq.h"
using namespace std;

typedef struct Message{ //структура для общения узлов
	int command;	//основной запрос
	int period;
	int amount;
	pid_t id;	//pid процесса созданного calculator
} Message;

Message m_null = {0,0,0,0};

string address = "tcp://*:";

void print_menu(){
	cout << "------------------------------" << endl
		 << "-	heartbeat [time] [amount]" << endl
		 << "-	menu" << endl
		 << "-	q" << endl
		 << "------------------------------" << endl;
}

void Do_request(zmq_msg_t request, void* executor, Message m_send){
	zmq_msg_init_size(&request, sizeof(Message));
	memcpy(zmq_msg_data(&request), &m_send, sizeof(Message));
	zmq_msg_send(&request, executor, 0);
	zmq_msg_close(&request);
}
/*
Message *Get_reply(zmq_msg_t reply, void* executor){
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, executor, 0);
	Message *m_back = (Message *) zmq_msg_data(&reply); 
	zmq_msg_close(&reply);
	return m_back;
}*/

int main (int argc, char const *argv[]) 
{
	if(argc != 2){
		cout<<"Wrong input...\nyou should write "<<endl;
		return 1;
	}
	void* context = zmq_ctx_new();
	void* executor = zmq_socket(context, ZMQ_REP);
	zmq_msg_t request, reply;
	Message m_send, *m_back;
	string choice;
	bool working = true, send = false;
	bool heartbeat = false, quit = false;

	address += argv[1];
	zmq_bind(executor, address.c_str());
	//system("cat doge.txt");
	cout << "Starting with address " << address.c_str() << endl;

	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, executor, 0);
	m_back = (Message *) zmq_msg_data(&reply); 
	zmq_msg_close(&reply);
	pid_t id = m_back->id;
	cout << "Connected id: " << id << endl;
	system("cat connected.txt");
	print_menu();
	while(working)
	{
		cout << "> ";
		cin >> choice;

		if(choice == "heartbeat"){
			m_send.command = 1;
			cin >> m_send.period;
			cin >> m_send.amount;
			send = true;
			heartbeat = true;
		}
		else if(choice == "menu"){
			print_menu();
		}
		else if(choice == "q"){
			m_send.command = 2;
			working = false;
			send = true;
			quit = true;
		}
		else{
			cout << "Try again..." << endl;
		}
		if(send){
			if(heartbeat){
				zmq_msg_init_size(&request, sizeof(Message));
				memcpy(zmq_msg_data(&request), &m_send, sizeof(Message));
				zmq_msg_send(&request, executor, 0);
				zmq_msg_close(&request);
				for(int i = 0; i < m_send.amount; i++){

					zmq_msg_init(&reply);
					zmq_msg_recv(&reply, executor, 0);
					m_back = (Message *) zmq_msg_data(&reply); 
					zmq_msg_close(&reply);

					cout << "Id: " << m_back->id << " is Staying Alive..." << endl;

					zmq_msg_init_size(&request, sizeof(Message));
					memcpy(zmq_msg_data(&request), &m_send, sizeof(Message));
					zmq_msg_send(&request, executor, 0);
					zmq_msg_close(&request);
				}
				heartbeat = false;
				m_send = m_null;
			}
			//else if()
			else if(quit){

				zmq_msg_init_size(&request, sizeof(Message));
				memcpy(zmq_msg_data(&request), &m_send, sizeof(Message));
				zmq_msg_send(&request, executor, 0);
				zmq_msg_close(&request);

				cout << "Closing controller..." << endl;
				break;
			}

			zmq_msg_init(&reply);
			zmq_msg_recv(&reply, executor, 0);
			m_back = (Message *) zmq_msg_data(&reply); 
			zmq_msg_close(&reply);

			send = false;
		}
	}
	zmq_close(executor);
	zmq_ctx_destroy(context);
	return 0;
}