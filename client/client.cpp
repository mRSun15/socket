//
// Created by 孙占林 on 2018/6/18.
//

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include "../packet.h"

#define INST_TIME 3
#define INST_CONNECT 1
#define INST_CLOSE 2
#define INST_NAME 4
#define INST_LIST 5
#define INST_MESSAGE 6
#define INST_EXIT 7

using namespace std;

bool flag;
sem_t *client_sem;

void *receive(void *socket_fd) {
    int client_socket = *((int *) socket_fd);
    char receiveBuffer[BUFFER_SIZE];
    while (1) {
        bzero(receiveBuffer, sizeof(receiveBuffer));
        int receive_state = recv(client_socket, receiveBuffer, BUFFER_SIZE, 0);
        if (receive_state < 0) {
            break;
        } else if (receive_state == 0) {
            continue;
        }
        Respond server_res;

        memcpy(&server_res, receiveBuffer, sizeof(server_res));
        int func = GET_FUNC(server_res.kind);
        int type = GET_TYPE(server_res.kind);
        int state = GET_STATE(server_res.kind);
        if (type == TYPE_ERROR) {
            cout << "send message failed: \n";
            cout << server_res.u.error_msg;
            sem_post(client_sem);
        } else if (type == TYPE_SUCCESS) {
            cout << "send message success. \n";
            sem_post(client_sem);
        } else if (type == TYPE_RES_REQ) {
            switch (func) {
                case FUNC_TIME:
                    char *time;
                    time = ctime(&server_res.u.time);
                    cout << time;
                    sem_post(client_sem);
                    break;
                case FUNC_NAME:
                    cout << server_res.u.name << endl;
                    sem_post(client_sem);
                    break;
                case FUNC_LIST:
                    cout << "client'msg id: " << server_res.u.list.id << ", ip: "
                         << server_res.u.list.ip << ":" << server_res.u.list.port << endl;
                    if (state == STATE_END)
                        sem_post(client_sem);
                    break;
                case FUNC_MESSAGE:;
                    cout << server_res.u.error_msg << endl;
//                    sem_post(client_sem);
                default:
                    break;
            }
        } else if (type == TYPE_INFO) {
            Info server_info;
            memcpy(&server_info, receiveBuffer, BUFFER_SIZE);

            if (state == STATE_BEGIN || state == STATE_SINGLE) {
//                sem_wait(&client_socket);
                cout << "The message is from " << server_info.from_cli_id << ": " << endl;
            }
            cout << server_info.msg;
        }

        if (type == TYPE_INFO && (state == STATE_END || state == STATE_SINGLE)) {
            cout << endl;
        }
    }
}

int main(void) {

    int choice = 0;
    int server_port = 0;
    struct sockaddr_in server_addr;
    string server_ip = "";
    int socket_fd = 0;
    char sendBuffer[BUFFER_SIZE];

    flag = false;
    pthread_t thread_id;

    while (1) {
        cout << "1.connect\t2.close\t3.get time\t4.get client name\t5.get client list\t6.send message\t7.exit\n";
        cin >> choice;
        if (!flag && (choice != INST_CONNECT && choice != INST_EXIT)) {
            cout << "Error! No connection." << endl;
        } else {

            switch (choice) {
                case INST_CONNECT : {

                    if (!flag) {
                        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (socket_fd < 0) {
                            cout << "Fail to create a socket" << endl;
                            break;
                        }

                        cout << "Input the server'msg IP: ";
                        cin >> server_ip;
                        cout << "Input the server'msg port: ";
                        cin >> server_port;
                        bzero(&server_addr, sizeof(server_addr));
                        server_addr.sin_family = PF_INET;
                        server_addr.sin_port = htons(server_port);
                        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
                        int connect_state = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
                        if (connect_state < 0) {
                            cout << "connection error " << endl;
                            break;
                        }
                        flag = true;
                        pthread_create(&thread_id, NULL, receive, (void *) &socket_fd);

                    } else if (flag) {
                        cout << "Error! The server has been connected,!" << endl;
                        break;
                    }
                    sem_unlink("x");
                    client_sem = sem_open("x", O_CREAT, 0644, 0);
                    cout << "test ..." <<endl;
//                    cout << "client_sem: " << *client_sem << endl;
//                    if(state < 0){
//                        cout << state << endl;
//                        cout << "semphore error !" << endl;
//                        exit(1);
//                    }
                    break;
                }
                case INST_CLOSE: {
                    cout << "close the connection." << endl;
//                    sem_wait(client_sem);
                    sem_close(client_sem);
                    pthread_detach(thread_id);
                    close(socket_fd);
                    flag = false;
                    break;
                }
                case INST_TIME: {

                    Request time_request;

                    time_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_TIME, STATE_SINGLE);


                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &time_request, sizeof(time_request));
                    send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                    sem_wait(client_sem);
                    break;
                }
                case INST_NAME: {
                    Request name_request;

                    name_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_NAME, STATE_SINGLE);

                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &name_request, sizeof(name_request));
                    send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
//                    cout << "client_sem: " << *client_sem << endl;
                    sem_wait(client_sem);


                    break;
                }
                case INST_LIST: {
                    Request list_request;

                    list_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_LIST, STATE_SINGLE);
                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &list_request, sizeof(list_request));
                    send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                    cout << "client list: " << endl;
//                    cout << "client_sem" << *client_sem << endl;
                    sem_wait(client_sem);


                    break;
                }
                case INST_MESSAGE: {
                    Request msg_request;

                    msg_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_LIST, STATE_SINGLE);

                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &msg_request, sizeof(msg_request));
                    send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                    sem_wait(client_sem);
//                    cout << *client_sem << endl;
                    int client_id;
                    string client_msg;
                    cout << "Choose the client'msg id: ";
                    cin >> client_id;
                    getchar();
                    cout << "Input the message: ";
                    getline(cin, client_msg);
//                    client_msg = client_msg + '\n';
                    bzero(sendBuffer, sizeof(sendBuffer));

                    Request info_request;
                    int msg_length = client_msg.length();
                    if (msg_length < MSG_SIZE) {
                        info_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_MESSAGE, STATE_SINGLE);
                        info_request.target_id = client_id;
                        client_msg.copy(info_request.buffer, msg_length + 1);
                        memcpy(sendBuffer, &info_request, sizeof(msg_request));
                        send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                    } else {
                        int pack_num = (msg_length / MSG_SIZE) + 1;
                        int pack_count = 0;
                        int pos = 0;
                        while (1) {
                            bzero(sendBuffer, sizeof(sendBuffer));
                            bzero(info_request.buffer, MSG_SIZE + 1);
                            if (pack_count == 0) {
                                info_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_MESSAGE, STATE_BEGIN);
                                info_request.target_id = client_id;
                                client_msg.substr(pos, MSG_SIZE).copy(info_request.buffer, MSG_SIZE);
                                info_request.buffer[MSG_SIZE] = '\0';
                                memcpy(sendBuffer, &info_request, sizeof(info_request));
                                send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                            } else if (pack_count == pack_num - 1) {
                                int rest_size = msg_length - pack_count * MSG_SIZE;
                                bzero(info_request.buffer, MSG_SIZE + 1);
                                info_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_MESSAGE, STATE_END);
                                info_request.target_id = client_id;
                                client_msg.substr(pos, rest_size).copy(info_request.buffer, rest_size);
                                info_request.buffer[rest_size] = '\0';
                                memcpy(sendBuffer, &info_request, sizeof(info_request));
                                send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                                break;
                            } else {
                                info_request.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_MESSAGE, STATE_MIDDLE);
                                info_request.target_id = client_id;
                                client_msg.substr(pos, MSG_SIZE).copy(info_request.buffer, MSG_SIZE);
                                info_request.buffer[MSG_SIZE] = '\0';
                                memcpy(sendBuffer, &info_request, sizeof(info_request));
                                send(socket_fd, sendBuffer, sizeof(sendBuffer), 0);
                            }
                            pack_count++;
                            pos += MSG_SIZE;
                        }
                    }
                    sem_wait(client_sem);
                    break;

                }
                case INST_EXIT: {
                    printf("exit");
                    if (flag) {
                        sem_close(client_sem);
                        close(socket_fd);

                        flag = false;
                    }
                    cout << "Exit the program." << endl;
                    exit(0);
                }
                default:
                    break;
            }
        }
    }
}
