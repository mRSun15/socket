//
// Created by 孙占林 on 2018/6/21.
//

#ifndef EXP7_PACKET_H
#define EXP7_PACKET_H

#define TYPE_CHECK 0x30
#define FUNC_CHECK 0x0c
#define STATE_CHECK 0x03
#define MSG_SIZE 1024
#define BUFFER_SIZE (MSG_SIZE+20)
#define KIND(TYPE, FUNC, STATE) ((TYPE << 4) + (FUNC << 2) + (STATE))
#define GET_TYPE(a) ((a & TYPE_CHECK)>>4)
#define GET_FUNC(a) ((a & FUNC_CHECK)>>2)
#define GET_STATE(a) (a & STATE_CHECK)
#define TYPE_RES_REQ 0x00
#define TYPE_INFO 0x01
#define TYPE_SUCCESS 0x02
#define TYPE_ERROR 0x03
#define FUNC_TIME 0x00
#define FUNC_NAME 0x01
#define FUNC_LIST 0x02
#define FUNC_MESSAGE 0x03
#define STATE_SINGLE 0x00
#define STATE_BEGIN 0x01
#define STATE_MIDDLE 0x02
#define STATE_END 0x03


//server---cliennt
typedef struct respond{
    unsigned char kind;
    union {
        time_t time;
        char name[20];
        struct {
            int id;
            char ip[20];
            int port;
        } list;
        char error_msg[20];
    } u;
}Respond;


//client---server
typedef struct request{
    unsigned char kind;
    // every two bit as an check bits
    // undefined -- type -- function -- state
    // type: 00 -- respond/reques ; 01 -- info ; 10 -- success ; 11 -- error
    // function: 00 -- time ; 01 -- name; 10 -- list ; 11 -- message
    // state: 00 -- single; 01 -- begin; 10 -- middle; 11 -- end;

    int target_id;
    char buffer[MSG_SIZE+1];
}Request;

//command packet
typedef  struct info{
    unsigned char kind;
    int from_cli_id;
    char msg[MSG_SIZE];

}Info;
#endif //EXP7_PACKET_H
