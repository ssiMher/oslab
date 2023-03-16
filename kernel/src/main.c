#include "klib.h"
#include "serial.h"
#include "vme.h"
#include "cte.h"
#include "loader.h"
#include "fs.h"
#include "proc.h"
#include "timer.h"
#include "dev.h"

void init_user_and_go();

int main() {
  init_gdt();
  init_serial();
  init_fs();
  init_page(); // uncomment me at Lab1-4
  init_cte(); // uncomment me at Lab1-5
  init_timer(); // uncomment me at Lab1-7
  //init_proc(); // uncomment me at Lab2-1
  //init_dev(); // uncomment me at Lab3-1
  printf("Hello from OS!\n");
  init_user_and_go();
  panic("should never come back");
}

void init_user_and_go() {
  // Lab1-2: ((void(*)())eip)();
  // Lab1-4: pdgir, stack_switch_call
  // Lab1-6: ctx, irq_iret
  // Lab1-8: argv
  // Lab2-1: proc
  // Lab3-2: add cwd
  // PD *pgdir = vm_alloc();
  // //uint32_t eip = load_elf(pgdir, "loaduser");
  // uint32_t eip = load_elf(pgdir, "brktest");
  // assert(eip != -1);
  // set_cr3(pgdir);
  // stack_switch_call((void*)(USR_MEM - 16), (void*)eip, 0);
  PD *pgdir = vm_alloc();
  Context ctx;
  assert(load_user(pgdir, &ctx, "loaduser", NULL) == 0);
  set_cr3(pgdir);
  set_tss(KSEL(SEG_KDATA), (uint32_t)kalloc() + PGSIZE);
  irq_iret(&ctx);

  // uint32_t eip = load_elf(NULL, "loaduser");
  // assert(eip != -1);
  // ((void(*)())eip)();
}
