#ifndef DEFS_HPP
#define DEFS_HPP

#include <stdint.h>  
#include <memory>
#include <functional>
  
#define MAX_PAYLOAD 1024  
#define NICKNAME_MAX_SIZE 31
  
;
#pragma pack(push, 1)
typedef struct  
{  
uint32_t length;  // длина поля type + payload  
uint8_t  type;  // тип сообщения  
char  payload[MAX_PAYLOAD]; // данные  
} Message;  
#pragma pack(pop)

/**
 * Структура ClientData
 *
 * Эта структура нигде не используется, но концептуально она представлена в
 * Server::conns_. Там хранятся обертки StreamSocket (shared_ptr на нее) и
 * сопоставление с никнеймом. При аутентификации учитывается размер
 * NICKNAME_MAX_SIZE. Также в conns_ хранятся только авторизованные
 * пользователи, что делает хранение поля _authenticated_ излишним
 */
#pragma pack(push, 1)
typedef struct {
    int sock;
    char nickname[NICKNAME_MAX_SIZE];
    int authenticated;
} ClientData;
#pragma pack(pop)

enum MessageType 
{  
MSG_HELLO  = 1, // клиент -> сервер (ник)  
MSG_WELCOME = 2, // сервер -> клиент  
MSG_TEXT  = 3, // текст  
MSG_PING  = 4,  
MSG_PONG  = 5,  
MSG_BYE  = 6,

MSG_AUTH = 7,        // аутентификация
MSG_PRIVATE = 8,     // личное сообщение
MSG_ERROR = 9,       // ошибка
MSG_SERVER_INFO = 10 // системные сообщения
};

class StreamSocket;

using SharedSocket = std::shared_ptr<StreamSocket>;

using Handler = std::function<void(SharedSocket &&)>;


#endif
