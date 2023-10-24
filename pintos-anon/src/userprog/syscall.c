#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/thread.h"
#include "process.h"
#include "threads/interrupt.h"
#include "threads/vaddr.h"


#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/directory.h"
#include "filesys/inode.h"
#include "devices/input.h"
#include "devices/shutdown.h"


#include "userprog/exception.h"
#include "userprog/pagedir.h"

/* UTILS */
static bool validate_address(const void *vaddr);
struct file_descriptor *get_file_descriptor(fid_t fid);

/* SYSTEM CALL FUNCTIONS */
/* WAIT AND HALT IS IMPLEMETED ON THE syscall_handler */
static int  create_fd (struct file *);
static void sys_exec (struct intr_frame *,  char *);
static void sys_exit (struct intr_frame *,  int status);
static void sys_open (struct intr_frame *f, const char *file_);
static void sys_close(struct intr_frame *f, fid_t);



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
    case SYS_EXIT:  sys_exit(f,-1);
                    break;
    case SYS_HALT:  shutdown_power_off();
                    break;
    case SYS_WAIT:  f->eax = process_wait((tid_t) args[1]);
                    break;
    case SYS_EXEC:  sys_exec(f,(char *)args[1]);
                    break;
    case SYS_OPEN:  sys_open(f,(char *)args[1]);
                    break;
    case SYS_CLOSE: sys_close(f, (fid_t)args[1]);
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

static void
sys_open (struct intr_frame *f,const char *file_name)
{
  struct file *file_ = filesys_open(file_name);
  f->eax = -1;
  if(file_)
  {
    fid_t fid = create_fd (file_);
    struct inode *inode = file_get_inode(file_);
    if(inode)
      get_file_descriptor(fid)->dir = dir_open (inode_reopen (inode));
    f->eax=fid;
  }
}

static void
sys_close (struct intr_frame *f, fid_t fid)
{
  if (fid < 2)
    sys_exit(f,-1);
  else{
    struct file_descriptor *fd = get_file_descriptor(fid);
    f->eax = -1; //RETURN -1 if close failed
    if (fd)
    {
      file_close(fd->file); /* file_close from src/filesys/file.c */
      if (fd->dir)
        dir_close(fd->dir);
      list_remove(&fd->fd_elem);
      free(fd);
      f->eax = 0;
    }
  }
}

//FILE MANIPULATORS


// FILE DESCRIPTORS
static int create_fd(struct file *file_)
{
  #ifdef USERPROG
    struct thread* t = thread_current();
    struct file_descriptor *fd = malloc(sizeof(struct file_descriptor));
    fd->file = file_;
    fd->dir = NULL;
    fd->fid = t->next_fd++;
    list_push_back(&t->fd_list,&fd->fd_elem);
    return fd->fid;
  #endif
}

struct file_descriptor *get_file_descriptor(fid_t fid)
{
  #ifdef USERPROG
    struct list *list_ = &thread_current()->fd_list;
    if(list_empty(list_))
      return NULL;

    struct list_elem *elem = list_begin(list_);
    for(; elem != list_end(list_) ; elem = list_next(elem))
    {
      struct file_descriptor *fd = list_entry(elem, struct file_descriptor, fd_elem);
      if (fd->fid == fid)
        return fd;
    }
    return NULL;
  #endif
}
