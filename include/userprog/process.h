#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd(const char* file_name);
tid_t process_fork(const char* name, struct intr_frame* if_);
int process_exec(void* f_name);
int process_wait(tid_t);
void process_exit(void);
void process_activate(struct thread* next);

/****************** P2: added ******************/
void stack_arguments(int argc, char** argv, struct intr_frame* if_);
/****************** P2: added - end ******************/

/****************** P3: added ******************/
bool install_page(void* upage, void* kpage, bool writable);

/****************** P3: added - end ******************/

#endif /* userprog/process.h */