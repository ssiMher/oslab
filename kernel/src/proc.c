#include "klib.h"
#include "cte.h"
#include "proc.h"

#define PROC_NUM 64

static __attribute__((used)) int next_pid = 1;

proc_t pcb[PROC_NUM];
static proc_t *curr = &pcb[0];

void init_proc() {
  // Lab2-1, set status and pgdir
  pcb[0].status = RUNNING;
  pcb[0].pgdir = vm_curr();
  // Lab2-4, init zombie_sem
  sem_init(&(pcb[0].zombie_sem),0);
  // Lab3-2, set cwd
  pcb[0].cwd = iopen("/", TYPE_NONE);
}

proc_t *proc_alloc() {
  // Lab2-1: find a unused pcb from pcb[1..PROC_NUM-1], return NULL if no such one
  int i = 1;
  for(; i < PROC_NUM;i++){
    if(pcb[i].status==UNUSED)break;
  }
  if(i==PROC_NUM)return NULL;
  // init ALL attributes of the pcb
  pcb[i].pid = next_pid;
  next_pid++;
  assert(next_pid<32768);
  pcb[i].status=UNINIT;
  pcb[i].pgdir = vm_alloc();
  assert(pcb[i].pgdir!=NULL);
  pcb[i].brk = 0;
  pcb[i].kstack = kalloc();
  assert(pcb[i].kstack!=NULL);
  pcb[i].ctx = &(pcb[i].kstack->ctx);
  pcb[i].parent = NULL;
  pcb[i].child_num = 0;
  sem_init(&(pcb[i].zombie_sem),0);
  for(int j = 0; j < MAX_USEM;j++){
    pcb[i].usems[j] = NULL;
  }
  for(int j = 0; j < MAX_UFILE;j++){
    pcb[i].files[j] = NULL;
  }
  pcb[i].cwd = NULL;
  return &pcb[i];
}

void proc_free(proc_t *proc) {
  // Lab2-1: free proc's pgdir and kstack and mark it UNUSED
  if(proc!=proc_curr()){
    proc->status=UNUSED;
  }
}

proc_t *proc_curr() {
  return curr;
}

void proc_run(proc_t *proc) {
  //printf("proc_run:proc %d\n",proc->pid);
  assert(proc!=NULL);
  proc->status = RUNNING;
  curr = proc;
  set_cr3(proc->pgdir);
  set_tss(KSEL(SEG_KDATA), (uint32_t)STACK_TOP(proc->kstack));
  //printf("proc_run pid: %d\n",proc->pid);
  // if(proc->pid!=0){
  //   printf("proc %d: proc->parent= 0x%x\n",proc->pid, proc->parent);
  //   printf("proc %d: proc->ctx= 0x%x\n",proc->pid, proc->ctx);
  //   printf("proc %d: proc->ctx->eip= 0x%x\n", proc->pid, proc->ctx->eip);
  // }
  irq_iret(proc->ctx);
}

void proc_addready(proc_t *proc) {
  // Lab2-1: mark proc READY
  assert(proc!=NULL);
  proc->status = READY;
}

void proc_yield() {
  // Lab2-1: mark curr proc READY, then int $0x81
  //cli();
  assert(curr!=NULL);
  curr->status = READY;
  //printf("proc_yield\n");
  INT(0x81);
}

void proc_copycurr(proc_t *proc) {
  // Lab2-2: copy curr proc
  assert(proc!=NULL);
  vm_copycurr(proc->pgdir);
  
  proc->brk = curr->brk;
  assert(proc->kstack!=NULL);
  proc->kstack->ctx = curr->kstack->ctx;
//  memcpy((void*)&proc->kstack->ctx, curr->ctx, sizeof(curr->kstack->ctx));
  //memcpy((void*)&proc->kstack->ctx,(void*)&curr->kstack->ctx,sizeof(curr->kstack->ctx));
  //proc->ctx = &(proc->kstack->ctx);
  //printf("proc%d->ctx->eip = 0x%x\n",proc->pid, proc->ctx->eip);
  //printf("curr%d->ctx->eip = 0x%x\n",curr->pid, curr->kstack->ctx.eip);
  //printf("proc%d->ctx->irq = %d\n",proc->pid, proc->ctx->irq);
  //printf("curr%d->ctx->irq = %d\n",curr->pid, curr->kstack->ctx.irq);
  assert(proc->ctx!=NULL);
  proc->ctx->eax = 0;
  proc->parent = curr;
  //printf("111\n");
  assert(curr!=NULL);
  (curr->child_num)++;
  for(int i = 0; i < MAX_USEM;i++){
    if(curr->usems[i]!=NULL){
       proc->usems[i] = curr->usems[i];
       usem_dup(proc->usems[i]);
    }
  }
  for(int i = 0; i < MAX_UFILE;i++){
    if(curr->files[i]!=NULL){
       proc->files[i] = curr->files[i];
       fdup(proc->files[i]);
    }
  }
  // Lab2-5: dup opened usems
  // Lab3-1: dup opened files
  // Lab3-2: dup cwd
  proc->cwd = idup(curr->cwd);
  
}

void proc_makezombie(proc_t *proc, int exitcode) {
  assert(proc!=NULL);
  // Lab2-3: mark proc ZOMBIE and record exitcode, set children's parent to NULL
  proc->status = ZOMBIE;
  proc->exit_code = exitcode;
  for(int i = 1; i < PROC_NUM;i++){
    if(pcb[i].parent==proc){
      pcb[i].parent=NULL;
    }
  }
  if(proc->parent!=NULL){
    sem_v(&(proc->parent->zombie_sem));
  }
  for(int j = 0;j < MAX_USEM;j++){
    if(proc->usems[j]!=NULL){
      usem_close(proc->usems[j]);
    }
  }
  for(int j = 0;j < MAX_UFILE;j++){
    if(proc->files[j]!=NULL){
      fclose(proc->files[j]);
    }
  }
  // Lab2-5: close opened usem
  // Lab3-1: close opened files
  // Lab3-2: close cwd
  iclose(proc->cwd);
  
}

proc_t *proc_findzombie(proc_t *proc) {
  assert(proc!=NULL);
  // Lab2-3: find a ZOMBIE whose parent is proc, return NULL if none
  for(int i = 0;i < PROC_NUM;i++){
    if(pcb[i].parent==proc && pcb[i].status==ZOMBIE){
      return &pcb[i];
    }
  }
  return NULL;
}

void proc_block() {
  // Lab2-4: mark curr proc BLOCKED, then int $0x81
  curr->status = BLOCKED;
  INT(0x81);
}

int proc_allocusem(proc_t *proc) {
  // Lab2-5: find a free slot in proc->usems, return its index, or -1 if none
  for(int i = 0; i < MAX_USEM;i++){
    if(proc->usems[i]==NULL)return i;
  }
  return -1;
}

usem_t *proc_getusem(proc_t *proc, int sem_id) {
  // Lab2-5: return proc->usems[sem_id], or NULL if sem_id out of bound
  if(sem_id<0||sem_id>=32)return NULL;
  return proc->usems[sem_id];
}

int proc_allocfile(proc_t *proc) {
  // Lab3-1: find a free slot in proc->files, return its index, or -1 if none
  for(int i = 0; i < MAX_UFILE;i++){
    if(proc->files[i]==NULL)return i;
  }
  return -1;
}

file_t *proc_getfile(proc_t *proc, int fd) {
  // Lab3-1: return proc->files[fd], or NULL if fd out of bound
  if(fd<0||fd>=32)return NULL;
  return proc->files[fd];
}

void schedule(Context *ctx) {
  assert(curr!=NULL);
  // Lab2-1: save ctx to curr->ctx, then find a READY proc and run it
  curr->ctx = ctx;
  int i = 0;
  for(;&pcb[i]!=curr;i++);
  i++;
  
  //i = (i + 1) % PROC_NUM;
  for(;;i++){
    if(i==PROC_NUM) i = 0;
    if(pcb[i].status==READY)break;
  }
  //printf("schedule proc: = %d\n",pcb[i].pid);
  proc_run(&pcb[i]);
}
