//
// Created by 孙占林 on 2018/6/18.
//

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/socket.h>
#include "../packet.h"

#define SERVER_PORT 5645
#define QUENE 20


int client_fd[QUENE];
bool client_connect[QUENE];
struct sockaddr_in client_addr[QUENE];
pthread_t pthreads[QUENE];
std::mutex m_lock;
int calculate_client_number(void){
    m_lock.lock();
    int count = 0;
    for(int i = 0; i < QUENE; i++){
        if(client_connect[i])
            count++;
    }
    m_lock.unlock();
    return count;
}

void* running_thread(void *for_client_fd){

    int server_client_fd = *((int *)for_client_fd);
    char receiveBuffer[BUFFER_SIZE];
    char sendBuffer[BUFFER_SIZE];
    int cli_idx = 0;
    m_lock.lock();
    for(cli_idx = 0; cli_idx < QUENE; cli_idx++)
        if(client_fd[cli_idx] == server_client_fd) break;
    m_lock.unlock();
    int receive_state;
    while(1){
        receive_state = static_cast<int>(recv(server_client_fd, receiveBuffer, BUFFER_SIZE, 0));
        if(receive_state < 0)
        {
            printf("Read error. ");
            break;
        }
        else if(receive_state == 0)
        {
            continue;
        }

        Request client_req;
        memset(&client_req, 0, sizeof(client_req));
        memcpy(&client_req, receiveBuffer, sizeof(client_req));
        std::cout << "Receive the request from the client: " << cli_idx << std::endl << client_req.kind << std::endl;

        int func = GET_FUNC(client_req.kind);
        int type = GET_TYPE(client_req.kind);
        int state = GET_STATE(client_req.kind);
        switch (func){
            case FUNC_NAME: {
                Respond name_res;
                std::cout << "client: " << cli_idx << " ask for name\n";
                name_res.kind = KIND(TYPE_RES_REQ, FUNC_NAME, STATE_SINGLE);
                gethostname(name_res.u.name, sizeof(name_res.u.name));
                bzero(sendBuffer, sizeof(sendBuffer));
                memcpy(sendBuffer, &name_res, sizeof(name_res));
                send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);
                break;
            }
            case FUNC_TIME: {
                Respond time_res;
                std::cout << "client: " << cli_idx << " ask for time\n";
                time_res.kind = KIND(TYPE_RES_REQ, FUNC_TIME, STATE_SINGLE);
                time_res.u.time = std::time(0);
                bzero(sendBuffer, sizeof(sendBuffer));
                memcpy(sendBuffer, &time_res, sizeof(time_res));
                send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);
                break;
            }
            case FUNC_LIST: {
                int client_count = calculate_client_number();
                Respond list_res;
                std::cout << "client: " << cli_idx << " ask for list\n";
                if (client_count == 1) {
                    bzero(sendBuffer, sizeof(sendBuffer));
                    list_res.kind = KIND(TYPE_RES_REQ, FUNC_LIST, STATE_SINGLE);
                    list_res.u.list.id = cli_idx;
                    std::string ip = inet_ntoa(client_addr[cli_idx].sin_addr);
                    ip.copy(list_res.u.list.ip, ip.length() + 1);
                    list_res.u.list.port = ntohs(client_addr[cli_idx].sin_port);
                    memcpy(sendBuffer, &list_res, sizeof(list_res));
                    send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);
                    break;
                }
                int count = 0;
                for (int i = 0; i < QUENE; i++) {
                    if (client_connect[i]) {
                        bzero(&list_res, sizeof(list_res));
                        if (count == 0) {
                            list_res.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_LIST, STATE_BEGIN);
                        } else if (count == (client_count - 1)) {
                            list_res.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_LIST, STATE_END);
                        } else {
                            list_res.kind = (unsigned char) KIND(TYPE_RES_REQ, FUNC_LIST, STATE_MIDDLE);
                        }

                        list_res.u.list.id = i;
                        std::string ip = inet_ntoa(client_addr[i].sin_addr);
                        ip.copy(list_res.u.list.ip, ip.length() + 1);
                        list_res.u.list.port = ntohs(client_addr[i].sin_port);
                        bzero(sendBuffer, sizeof(sendBuffer));
                        memcpy(sendBuffer, &list_res, sizeof(list_res));
                        send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);

                        count++;
                    }
                }
                break;
            }
            case FUNC_MESSAGE: {
                std::cout << "client: " << cli_idx << " ask for message\n";
                int target_id = client_req.target_id;
                if (!client_connect[target_id]) {
                    Respond error_res;
                    std::string erroe_msg = "The client isn't connected.\n";
                    erroe_msg.copy(error_res.u.error_msg, erroe_msg.length() + 1);
                    error_res.kind = (unsigned char) KIND(TYPE_ERROR, FUNC_MESSAGE, STATE_SINGLE);
                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &error_res, sizeof(error_res));
                    send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);

                    break;
                } else if (target_id >= QUENE) {
                    Respond error_res;
                    std::string erroe_msg = "The client doesn't exist.\n";
                    erroe_msg.copy(error_res.u.error_msg, erroe_msg.length() + 1);
                    error_res.kind = (unsigned char) KIND(TYPE_ERROR, FUNC_MESSAGE, STATE_SINGLE);
                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &error_res, sizeof(error_res));
                    send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);

                    break;
                } else {
                    Respond success_res;
                    success_res.kind = (unsigned char) KIND(TYPE_SUCCESS, FUNC_MESSAGE, STATE_SINGLE);
                    std::string succes_msg = "Send Message successful.\n";
                    succes_msg.copy(success_res.u.error_msg, succes_msg.length()+1);
                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &success_res, sizeof(success_res));
                    send(server_client_fd, sendBuffer, sizeof(sendBuffer), 0);
                }

                std::string msg = client_req.buffer;
                int msg_length = msg.length();
                Info info_packet;
                if (msg_length < MSG_SIZE) {
                    info_packet.from_cli_id = cli_idx;
                    info_packet.kind = (unsigned char) KIND(TYPE_INFO, FUNC_MESSAGE, STATE_SINGLE);
                    msg.copy(info_packet.msg, msg_length + 1);
                    bzero(sendBuffer, sizeof(sendBuffer));
                    memcpy(sendBuffer, &info_packet, sizeof(info_packet));
                    send(client_fd[target_id], sendBuffer, sizeof(sendBuffer), 0);

                } else {
                    int packet_num = msg_length / MSG_SIZE + 1;
                    int i = 0;
                    int msg_pos = 0;
                    int rest_size;
                    while (i < packet_num) {
                        bzero(&info_packet, sizeof(info_packet));
                        if (i == 0) {
                            info_packet.kind = (unsigned char) KIND(TYPE_INFO, FUNC_MESSAGE, STATE_BEGIN);
                            rest_size = MSG_SIZE;
                        } else if (i == packet_num - 1) {
                            info_packet.kind = (unsigned char) KIND(TYPE_INFO, FUNC_MESSAGE, STATE_END);
                            rest_size = msg_length - i * MSG_SIZE;
                        } else {
                            info_packet.kind = (unsigned char) KIND(TYPE_INFO, FUNC_MESSAGE, STATE_END);
                            rest_size = MSG_SIZE;
                        }
                        msg.substr(msg_pos, rest_size).copy(info_packet.msg, rest_size);
                        info_packet.msg[rest_size] = '\0';
                        bzero(sendBuffer, sizeof(sendBuffer));
                        memcpy(sendBuffer, &info_packet, sizeof(info_packet));
                        send(client_fd[target_id], sendBuffer, sizeof(sendBuffer), 0);
                        i++;
                        msg_pos++;
                    }
                }
                break;
            }
            default:
                break;
        }

    }

}



int main(void)
{
    int socket_fd = 0;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0){
        printf("socket created failed.");
        exit(1);
    }
    struct sockaddr_in server_sockaddr;
    char sendBuffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr[0]);
    memset(&server_sockaddr, '\0', sizeof(server_sockaddr));
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(SERVER_PORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    int soceket_bind = bind(socket_fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr));
    if (soceket_bind < 0){
        printf("binding error.");
        exit(1);
    }

    int socket_listen = listen(socket_fd, QUENE);
    if(socket_listen < 0){
        printf("listening error.");
        exit(1);
    }
    int client_index = 0;
    while (1){
        memset(sendBuffer, '\0', sizeof(sendBuffer));
        int client_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addr[client_index], &client_len);
        if(client_socket_fd < 0)
        {
            printf("connect error.");
            exit(1);
        }
        std::cout << "build connection with client" << client_index << std::endl;
        Respond msg_respond;
        msg_respond.kind = (unsigned char)KIND(TYPE_RES_REQ, FUNC_MESSAGE, STATE_SINGLE);
        std::string msg = "Hello!.";
        msg.copy(msg_respond.u.error_msg, msg.length()+1);
        memcpy(sendBuffer, &msg_respond, sizeof(msg_respond));
        send(client_socket_fd, sendBuffer, sizeof(sendBuffer), 0);
        client_connect[client_index] = true;
        client_fd[client_index] = client_socket_fd;
        client_index += 1;
        pthread_create(&pthreads[client_index], NULL, running_thread, (void *)&client_socket_fd);
    }
    close(socket_fd);
}

