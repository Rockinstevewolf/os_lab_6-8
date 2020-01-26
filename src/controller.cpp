#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "zmq.h"
using namespace std;

typedef struct Message{ //структура для общения узлов
	int status;
	int command;	//основной запрос
	int period;
	int amount;
	int list[100];
	int sum;
	pid_t pid;	//pid процесса созданного calculator
	int id;
} Message;

Message m_null = {0,0,0,0,0,0,0};

string address = "tcp://*:";

void print_menu(){
	cout << "------------------------------------" << endl
		 << "-	create [id]" << endl
		 << "-	exec [id] [amount] [a, b, c, d, ...]" << endl
		 << "-	heartbeat [id] [time] [amount]" << endl
		 << "-	menu" << endl
		 << "-	q" << endl
		 << "------------------------------------" << endl;
}


int main (int argc, char const *argv[]) 
{
	if(argc != 2){
		cout<<"Wrong input...\nyou have to write port"<<endl;
		return 1;
	}
	void* context = zmq_ctx_new();
	void* executor = zmq_socket(context, ZMQ_REP);
	zmq_msg_t request, reply;
	Message m_send, *m_back;
	string choice;
	int el;
	vector <int> ids;
	int cid;

	bool working = true, send = false;
	bool heartbeat = false, quit = false, exec = false, create = false;

	address += argv[1];
	zmq_bind(executor, address.c_str());
	cout << "Starting with address " << address.c_str() << endl;

	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, executor, 0);
	m_back = (Message *) zmq_msg_data(&reply); 
	zmq_msg_close(&reply);
	int mid = m_back->id;
	cout << "Connected id: " << mid << endl;
	//system("cat connected.txt");
	ids.push_back(mid);
	print_menu();
	m_send = m_null;
	while(working)
	{
		cout << "> ";
		cin >> choice;
		if(choice == "create"){
			cin >> cid;
			for(int i : ids){
				if(cid == i){
					create = false;
					break;
				}
				else{
					create = true;
				}
			}
			if(cid > 0){
				if(create){
					m_send.id = cid;
					m_send.command = -1;
					send = true;
					create = true;
				}
				else{
					cout << "Id already exists..." << endl;
				}
			}
			else{
				cout << "Wrong id input..." << endl;
			}
		}
		else if(choice == "exec"){
			cin >> cid;
			for(int i : ids){
				if(i == cid){
					m_send.command = 0;
					m_send.id = cid;
					cin >> m_send.amount;
					for(int i = 0; i < m_send.amount; i++){
						cin >> m_send.list[i];
						//cout << m_send.list[i] << endl;
					}
					send = true;
					exec = true;
				}
			}
			if(!exec){
				cout << "Id does not exist..." << endl;
			}
		}
		else if(choice == "heartbeat"){
			cin >> cid;
			for(int i : ids){
				if(i == cid){
					m_send.command = 1;
					m_send.id = cid;
					cin >> m_send.period;
					cin >> m_send.amount;
					send = true;
					heartbeat = true;
				}
			}
			if(!heartbeat){
				cout << "Id does not exist..." << endl;
			}
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
			zmq_msg_init_size(&request, sizeof(Message));
			memcpy(zmq_msg_data(&request), &m_send, sizeof(Message));
			zmq_msg_send(&request, executor, 0);
			zmq_msg_close(&request);

			if(heartbeat){
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
			}

			zmq_msg_init(&reply);
			zmq_msg_recv(&reply, executor, 0);
			m_back = (Message *) zmq_msg_data(&reply); 
			zmq_msg_close(&reply);

			if(quit){
				cout << "Closing controller..." << endl;
				break;
			}

			if(create){
				if(m_back->status == 0){
					cout << "Created: " << m_back->pid << endl;
					ids.push_back(cid);
				}
				else{
					cout << "Could not create node: " << m_send.id << endl;
				}
				create = false;
			}

			if(exec){
				if(m_back->status == 0){
					cout << "Ok--" << m_back->id << ": " << m_back->sum << endl;
					exec = false;
				}
				else{
					cout << "Could find node: " << m_send.id << endl;
				}
			}

			m_send = m_null;
			send = false;
		}
	}
	zmq_close(executor);
	zmq_ctx_destroy(context);
	return 0;
}