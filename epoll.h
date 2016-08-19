/*
 * epoll.h
 *
 *  Created on: 02.08.2016
 *      Author: Dorwig
 */

#ifndef INCLUDE_EPOLL_H_
#define INCLUDE_EPOLL_H_

enum EPOLL_EVENTS {
    EPOLLIN      = 0x0001,
    EPOLLOUT     = 0x0002,
    EPOLLRDHUP   = 0x0004,
    EPOLLPRI     = 0x0008,
    EPOLLERR     = 0x0010,
    EPOLLHUP     = 0x0020,
    EPOLLET      = 0x0040,
    EPOLLONESHOT = 0x0080
};

enum EPOLL_OPCODES {
    EPOLL_CTL_ADD,
    EPOLL_CTL_DEL,
    EPOLL_CTL_MOD
};

typedef union epoll_data {
    void* ptr;
    int fd;
    unsigned int u32;
    unsigned long long u64;
} epoll_data_t;

struct epoll_event {
    unsigned int events;
    epoll_data_t data;
};

int epoll_ctl(int epfd, int opcode, int fd, struct epoll_event* event);
int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);


#endif /* INCLUDE_EPOLL_H_ */
