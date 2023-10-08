#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

typedef int pid_t;

struct process_hierarchy {
    pid_t pid;
    int exit_code;
    bool is_exit;
    int numThreads;

    struct semaphore waitLock;
    struct lock      cvLock;
    struct list_elem children;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void init_process_hierarchy(struct process_hierarchy *ph);
struct process_hierarchy *find_childtid(struct thread *t,tid_t child_tid);
void decreaseThreads(struct process_hierarchy *);

#endif /* userprog/process.h */
