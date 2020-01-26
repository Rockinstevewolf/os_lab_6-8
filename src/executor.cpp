#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <sys/types.h> 
#include "zmq.h"
using namespace std;

string address = "tcp://localhost:";

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

struct node{	//узел дерева
	pid_t pid;
	int id = 0;
	int port = 0;
	void* socket;

}me, next_nd;

pid_t add_node(node *s, int id, void* context);
Message *send_task(void* socket, Message m);

int main (int argc, char const *argv[]) 
{
	if(argc == 3){
		me.id = atoi(argv[2]);
	}
	else{
		cout << "Wrong input...\nyou should write "<< endl;
		return 1;
	}
	address += argv[1];
	me.port = atoi(argv[1]);

	void* context = zmq_ctx_new();
	void* controller = zmq_socket(context, ZMQ_REQ);

	if(zmq_connect(controller, address.c_str()) == 0){
		cout << me.id << "-connected to " << address.c_str() << " with [ ppid = " << getppid() << " ]" << endl;
	}
	zmq_msg_t request, reply;
	Message *m_back, m_send;

	me.pid = getpid();
	m_send.pid = me.pid;
	m_send.id = me.id;

	cout << me.id << "-my [ pid = " << me.pid << " ]" << endl;

	zmq_msg_init_size(&reply, sizeof(Message));
	memcpy(zmq_msg_data(&reply), &m_send, sizeof(Message));
	zmq_msg_send(&reply, controller, 0);
	zmq_msg_close(&reply);

	//cout << "No problems detected" << endl;
	pid_t pid;
	bool working = true;
	bool send = false;
	bool quit = false;
	while(working)
	{
		cout << "---request-" << me.id << "-waiting---" << endl;

		zmq_msg_init(&request);
		zmq_msg_recv(&request, controller, 0);
		m_back = (Message *) zmq_msg_data(&request); 
		zmq_msg_close(&request);

		cout << "--> " << m_back->command << endl;
		m_send.pid = getpid();
		m_send.id = me.id;

		if(m_back->command == -1){
			if(next_nd.id == 0){
				pid = add_node(&next_nd, m_back->id, context);
				if(pid > 0){
					m_send.pid = pid;
					m_send.status = 0;
					cout << me.id << "-created next node [ id = " << next_nd.id << " ] [ pid = " << next_nd.pid << " ]" << endl;
				}else{
					m_send.status = 1;
				}
			}
			else{
				cout << me.id << "-sends task to-" << next_nd.id << endl;
				Message *t = send_task(next_nd.socket, (*m_back));
				m_send = (*t);
			}
			send = true;
		}
		else if(m_back->command == 0){
			if(m_back->id ==  me.id){
				m_send.sum = 0;
				int n = m_back->amount;
				cout << "exec " << n << " ";
				for(int i = 0; i < n; i++){
					cout << m_back->list[i] << " ";
					m_send.sum = m_send.sum + m_back->list[i];
				}
				cout << endl;
				m_send.status = 0;
			}
			else{
				if(next_nd.id !=0){
					cout << me.id << "-sends task to-" << next_nd.id << endl;
					Message *t = send_task(next_nd.socket, (*m_back));
					m_send = (*t);
				}else{
					m_send.status = 1;
				}
			}
			send = true;
		}
		else if(m_back->command == 1){
			if(m_back->id ==  me.id){
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
				m_send.status = 0;
			}
			else{
				if(next_nd.id !=0){
					cout << me.id << "-sends task to-" << next_nd.id << endl;
					Message *t = send_task(next_nd.socket, (*m_back));
					m_send = (*t);
				}else{
					m_send.status = 1;
				}
			}
			send = true;
		}
		else if(m_back->command == 2){
			if(next_nd.id != 0){
				send_task(next_nd.socket, (*m_back));
			}
			working = false;
			send = true;
			quit = true;
			cout << "Closing calculator..." << endl;
			//break;
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
			if(quit){
				break;
			}
		}
	}
	//cout << "here" << endl;

	zmq_close(controller);
	zmq_ctx_destroy(context);
	exit(1);
	return 0;
}


pid_t add_node(node *s, int id, void* context){
	pid_t pid;
	s->id = id;
	s->port = me.port + s->id;
	string arg1 = to_string(s->id), arg2 = to_string(s->port);
	pid = fork();//создаем новый процесс
	if(pid == 0){
		char * args[] = {const_cast<char*>("child"),const_cast<char*>(arg2.c_str()), const_cast<char*>(arg1.c_str()), NULL}; 
		execv("./executor", args);	//и запускаем в нем программу заново с помощью execv (почитай доки)
		cout << "error setting up new executor" << endl;
		return -1;
	}else if(pid > 0){
		s->socket = zmq_socket(context, ZMQ_REP); //тут коннектимся с созданным узлом и запоминаем нужные данные
		string newarrd = "tcp://*:";
		newarrd+=arg2;
		zmq_bind(s->socket, newarrd.c_str());
		cout << me.id << "-connecting to address " << newarrd.c_str() << endl;
		zmq_msg_t rep;
		zmq_msg_init(&rep);
		zmq_msg_recv(&rep, s->socket, 0);
		Message *m = (Message *) zmq_msg_data(&rep);
		zmq_msg_close(&rep);
		pid = m->pid;	//получаем от него pid 
		//cout << me.id << "-connected with [ pid = " << pid << " ]" << endl;
		s->pid = pid;
		return pid;
	}else if(pid<0){
		return -1;
	}
	return -1;
}

Message *send_task(void* socket, Message t){
	zmq_msg_t q, a;

	zmq_msg_init_size(&q, sizeof(Message));
	memcpy(zmq_msg_data(&q), &t, sizeof(Message));
	zmq_msg_send(&q, socket, 0);
	zmq_msg_close(&q);

	zmq_msg_init(&a);
	zmq_msg_recv(&a, socket, 0);
	Message *m = (Message *) zmq_msg_data(&a);
	zmq_msg_close(&a);

	return m;
}	
