#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/thread.h"
#include "process.h"
#include "threads/interrupt.h"
#include "threads/vaddr.h"

static bool validate_address(const void *vaddr);
static void sys_exec (struct intr_frame *, char *);
/* WAIT AND HALT IS IMPLEMETED ON THE syscall_handler */
static void sys_exit (struct intr_frame *,int status);



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool validate_address(const void *vaddr)
{
  /* CHECKS IF THE POINTERS ARE VALID */
  uint32_t *ptr = (uint32_t *) vaddr;
  return is_user_vaddr(ptr) && vaddr != NULL
  #ifdef USERPROG
    && pagedir_get_page(thread_current()->pagedir,ptr) != NULL
  #endif 
    ;
}

void
syscall_handler (struct intr_frame *f) 
{
  uint32_t* args = ((uint32_t*) f->esp);

  if(! (validate_address(args)) ) {
    //sys_exit(0); //0 for failed execution
  }

  uint32_t sys_code = args[0];
  switch (sys_code) {
    case SYS_EXIT: sys_exit(f,-1);
                   break;
    case SYS_HALT: shutdown_power_off();
                   break;
    case SYS_WAIT: f->eax = process_wait((tid_t) args[1]);
                   break;
    case SYS_EXEC: sys_exec(f,(char *)args[1]);
                   break;
    default: sys_exit(f,args[1]);
  }

}

static void
sys_exec (struct intr_frame *f, char *command)
{
    f->eax = process_execute (command);
}

static void
sys_exit (struct intr_frame *f, int status)
{
  thread_exit();
  f->eax = status;
}

