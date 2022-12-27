#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);
void syscall_handler(struct intr_frame *f UNUSED);
void halt();
void exit();

#endif /* userprog/syscall.h */
