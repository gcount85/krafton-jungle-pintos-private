#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

// P2 syscall: filesys_lock 구조체를 위한 헤더 추가 - unsure
#include "include/threads/synch.h" 
// P2 syscall: filesys_lock 구조체를 위한 헤더 추가 끝 - unsure

void syscall_init(void);
void syscall_handler(struct intr_frame *f UNUSED);

/********* P2 syscall: 파일에 대한 경쟁조건을 피하기 위한 글로벌 락 *********/
struct lock filesys_lock;

/********* P2 syscall: 파일에 대한 경쟁조건을 피하기 위한 글로벌 락 - end *********/

#endif /* userprog/syscall.h */
