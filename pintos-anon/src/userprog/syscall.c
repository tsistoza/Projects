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
static void validate_address(struct intr_frame *f, const uint8_t *addr);
struct file_descriptor *get_file_descriptor(fid_t fid);
static unsigned inputbuf (char* buffer, unsigned size); /* used for sys_read */

/* SYSTEM CALL FUNCTIONS */
/* WAIT AND HALT IS IMPLEMETED ON THE syscall_handler */
static int  create_fd (struct file *);
static void sys_exec (struct intr_frame *,  char *);
static void sys_exit (struct intr_frame *,  int status);
static void sys_open (struct intr_frame *f, const char *file_);
static void sys_close(struct intr_frame *f, fid_t);
static void sys_create(struct intr_frame *f,const char*, off_t);
static void sys_remove(struct intr_frame *f,const char*);
static void sys_filesize(struct intr_frame *f, fid_t);
static void sys_read(struct intr_frame *f, fid_t fid, char* arg2, unsigned arg3);
static void sys_write(struct intr_frame *f, fid_t fid, const char* arg2, unsigned arg3);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void validate_address(struct intr_frame *f, const uint8_t *addr)
{
  if(!is_user_vaddr(addr)){
    printf("INVALID ADDRESS EXITING...\n");
    sys_exit(f, -1);
  }
  if(!pagedir_get_page(thread_current()->pagedir,addr))
  {
    printf("INVALID ADDRESS EXITING...\n");
    sys_exit(f,-1);
  }
}

void
syscall_handler (struct intr_frame *f) 
{
  int *args = f->esp;

  validate_address(f, (uint8_t *)args);

  int sys_code = args[0];
  //printf("sys_code = %"PRIu32"\n",sys_code);
  //printf("arguments = %s\n", (char *) args[85]); //arguments are at 85,86
  

  switch (sys_code) {
    case SYS_EXIT:     validate_address(f, (uint8_t *) (args+1));
                       sys_exit(f,-1);
                       break;
    case SYS_HALT:     shutdown_power_off();
                       break;
    case SYS_WAIT:     validate_address(f, (uint8_t *) (args+1));
                       f->eax = process_wait((tid_t) args[1]);
                       break;
    case SYS_EXEC:     validate_address(f, (uint8_t *) (args+1));
                       validate_address(f, (uint8_t *) (args[1]));
                       sys_exec(f,(char *)args[1]);
                       break;
    case SYS_OPEN:     validate_address(f, (uint8_t *) (args+1));
                       validate_address(f, (uint8_t *) args[1]);
                       sys_open(f, (char *)args[1]);
                       break;
    case SYS_CLOSE:    validate_address(f, (uint8_t *) (args+1));
                       sys_close(f, (fid_t)args[1]);
                       break;
    case SYS_CREATE:   validate_address(f, (uint8_t *) (args+1));
                       validate_address(f, (uint8_t *) (args+2));
                       validate_address(f, (uint8_t *) args[1]);
                       sys_create(f, (char *)args[1],args[2]);
                       break;
    case SYS_REMOVE:   validate_address(f, (uint8_t *) args[1]);
                       sys_remove(f, (char *)args[1]);
                       break;
    case SYS_FILESIZE: validate_address(f, (uint8_t *) args[1]);
                       sys_filesize(f, (fid_t)args[1]);
                       break;
    case SYS_READ:     validate_address(f, (uint8_t *) args[1]);
                       validate_address(f, (uint8_t *) args[3]);
                       sys_read(f,(fid_t) args[1], (char *) args[2], (unsigned) args[3]);
                       break;
    case SYS_WRITE:    validate_address(f, (uint8_t *) args+1);
                       validate_address(f, (uint8_t *) args+2);
                       validate_address(f, (uint8_t *) args+3);
                       sys_write(f,(fid_t) args[1], (char *) args[2], (unsigned) args[3]);
                       break;
    default:           printf("DEFAULT SYS EXIT SYSCALL\n"); 
                       sys_exit(f,args[1]);
  }

}

/* MAIN SYSTEM CALLS */

static void
sys_exec (struct intr_frame *f, char *command)
{
    f->eax = process_execute (command);
}

static void
sys_exit (struct intr_frame *f, int status)
{
  struct thread *curr = thread_current();
  f->eax = status;
  printf("%s: exit(%d)", curr->name, status);
  thread_exit(); /* process_exit kills all child processes */
}

static void
sys_open (struct intr_frame *f,const char *file_name)
{
  lock_acquire(&filesys_lock);
  struct file *file_ = filesys_open(file_name);
  lock_release(&filesys_lock);
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
      lock_acquire(&filesys_lock);
      file_close(fd->file); /* file_close from src/filesys/file.c */
      lock_release(&filesys_lock);
      if (fd->dir)
        dir_close(fd->dir);
      list_remove(&fd->fd_elem);
      free(fd);
      f->eax = 0;
    }
  }
}

//FILE MANIPULATORS
static void
sys_create(struct intr_frame *f,const char* file_name,off_t initial_size)
{
  lock_acquire(&filesys_lock);
  f->eax = filesys_create (file_name, initial_size);
  lock_release(&filesys_lock);
}

static void
sys_remove(struct intr_frame *f, const char* file_name)
{
  lock_acquire(&filesys_lock);
  f->eax = filesys_remove (file_name);
  lock_release(&filesys_lock);
}

static void
sys_filesize(struct intr_frame *f, fid_t fid)
{
  if (fid < 2)
    sys_exit(f,-1);
  else
  {
    struct file_descriptor *fd = get_file_descriptor(fid);
    lock_acquire(&filesys_lock);
    f->eax = fd ? file_length(fd->file) : -1;
    lock_release(&filesys_lock);
  }
}

static unsigned
inputbuf (char* buffer, unsigned size)
{
  size_t i = 0;
  while(i < size) {
    buffer[i] = input_getc();
    if(buffer[i++] == '\n') /* enter key */
    {
      buffer[i-1] = '\0'; /* end the string */
      break;
    }
  }
  return i;
}

static void
sys_read(struct intr_frame *f, fid_t fid, char *buffer, unsigned size)
{
  if (fid == 1) /* STDOUT */
    sys_exit(f, -1);
  
  if (size < 1) /* Check if size is ok */
    f->eax=0;
  
  if (fid == 0){ /* if STDIN read from keyboard*/
    f->eax = inputbuf(buffer, size);
  }
  else { /* if not STDIN just read from file from file descriptor */
    struct file_descriptor * fd = get_file_descriptor(fid);
    lock_acquire(&filesys_lock);
    f->eax = fd ? file_read(fd->file, buffer, size): -1;
    lock_release(&filesys_lock);
  }
}

static void
sys_write(struct intr_frame *f, fid_t fid, const char* buffer, unsigned size) 
{
  if (fid == 0) /* STDIN */
    sys_exit(f,-1);

  if (size < 1) /* CHECK SIZE */
    f->eax = 0;

  if (fid == 1) {
    putbuf(buffer, size);
    f->eax =size;
  } else {
    struct file_descriptor *fd = get_file_descriptor(fid);
    lock_acquire(&filesys_lock);
    f->eax = fd ? file_write(fd->file, buffer, size): -1;
    lock_release(&filesys_lock);
  }
}

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
