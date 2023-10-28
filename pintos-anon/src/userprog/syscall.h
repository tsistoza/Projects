#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/interrupt.h"

struct lock filesys_lock;

void syscall_init (void);
void syscall_handler (struct intr_frame *f);

#endif /* userprog/syscall.h */
