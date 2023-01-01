#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

// P2 syscall: filesys_lock 구조체를 위한 헤더 추가 - unsure
#include "include/threads/synch.h"
// P2 syscall: filesys_lock 구조체를 위한 헤더 추가 끝 - unsure

void syscall_init(void);

/**************** P2 syscall: 프로토타입 선언 ****************/
typedef int tid_t;
struct lock filesys_lock; // P2 syscall: 파일에 대한 경쟁조건을 피하기 위한 글로벌 락


// P2 syscall: 시스템 콜 함수 구현
void halt(void);
void exit(int status);
int fork(const char *thread_name);
int exec(const char *cmd_line);
int wait(tid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned new_position);
unsigned tell(int fd);
void close(int fd);

/**************** P2 syscall: 프로토타입 선언 - 끝 ****************/

#endif /* userprog/syscall.h */