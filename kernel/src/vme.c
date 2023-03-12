#include "klib.h"
#include "vme.h"
#include "proc.h"

static TSS32 tss;

void init_gdt()
{
  static SegDesc gdt[NR_SEG];
  gdt[SEG_KCODE] = SEG32(STA_X | STA_R, 0, 0xffffffff, DPL_KERN);
  gdt[SEG_KDATA] = SEG32(STA_W, 0, 0xffffffff, DPL_KERN);
  gdt[SEG_UCODE] = SEG32(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
  gdt[SEG_UDATA] = SEG32(STA_W, 0, 0xffffffff, DPL_USER);
  gdt[SEG_TSS] = SEG16(STS_T32A, &tss, sizeof(tss) - 1, DPL_KERN);
  set_gdt(gdt, sizeof(gdt[0]) * NR_SEG);
  set_tr(KSEL(SEG_TSS));
}

void set_tss(uint32_t ss0, uint32_t esp0)
{
  tss.ss0 = ss0;
  tss.esp0 = esp0;
}

static PD kpd;
static PT kpt[PHY_MEM / PT_SIZE] __attribute__((used));
typedef union free_page {
  union free_page *next;
  char buf[PGSIZE];
} page_t;

page_t *free_page_list;

void init_page()
{
  extern char end;
  panic_on((size_t)(&end) >= KER_MEM - PGSIZE, "Kernel too big (MLE)");
  static_assert(sizeof(PTE) == 4, "PTE must be 4 bytes");
  static_assert(sizeof(PDE) == 4, "PDE must be 4 bytes");
  static_assert(sizeof(PT) == PGSIZE, "PT must be one page");
  static_assert(sizeof(PD) == PGSIZE, "PD must be one page");
  // Lab1-4: init kpd and kpt, identity mapping of [0 (or 4096), PHY_MEM)
  for (int i = 0; i < (PHY_MEM / PT_SIZE); i++)
  {
    kpd.pde[i].val = MAKE_PDE(&kpt[i], 1);
    for (int j = 0; j < NR_PTE; j++)
    {
      void *va = (void*)((i << DIR_SHIFT) | (j << TBL_SHIFT));
      kpt[i].pte[j].val = MAKE_PTE(va, 1);
    }
  }

  // TODO();
  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);
  // Lab1-4: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
  free_page_list = (page_t*)KER_MEM;
  page_t* temppage = free_page_list;
  while((int)free_page_list<PHY_MEM){
    free_page_list->next = (page_t*)((int)free_page_list+PGSIZE);
    free_page_list = free_page_list->next;
  }
  free_page_list = temppage;

//  TODO();
}

void *kalloc()
{
  if((int)free_page_list<PHY_MEM){
    void * va = free_page_list;
    free_page_list = free_page_list->next;
    memset(va,0,4096);
  //  Log("kalloc:%x  ",va);
    
  //  int index = (int)va >> 22 & 0x3ff;
  //  int jndex = ((int)va << 10) >> 22 & 0x3ff;
  //  kpt[index].pte[jndex].present = 1;
    return va;
  }
  else{
    assert(0);
  }
  // Lab1-4: alloc a page from kernel heap, abort when heap empty
  //TODO();
}

void kfree(void *ptr)
{
  if((int)ptr<KER_MEM||(int)ptr>PHY_MEM){
    assert(0);
  }
  void* va = free_page_list;
  free_page_list = ptr;
  free_page_list->next = va;

  // Lab1-4: free a page to kernel heap
  // you can just do nothing :)
  // TODO();
}

PD *vm_alloc()
{
  // Lab1-4: alloc a new pgdir, map memory under PHY_MEM identityly
  PD* pgdir = (PD*)kalloc();
  for (int i = 0; i < 32; i++)
  {
    pgdir->pde[i].val = MAKE_PDE(&kpt[i], 1);   
  }
  for(int i = 32; i< 1024; i++){
    pgdir->pde[i].val = 0;
  }
  return pgdir;
  //TODO();
}

void vm_teardown(PD *pgdir)
{
  // Lab1-4: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  // TODO();
}

PD *vm_curr()
{
  return (PD *)PAGE_DOWN(get_cr3());
}

PTE *vm_walkpte(PD *pgdir, size_t va, int prot)
{
  // Lab1-4: return the pointer of PTE which match va
  // if not exist (PDE of va is empty) and prot&1, alloc PT and fill the PDE
  // if not exist (PDE of va is empty) and !(prot&1), return NULL
  // remember to let pde's prot |= prot, but not pte
  int pte_index = ADDR2TBL(va);
  int pde_index = ADDR2DIR(va);
  PDE* pde_pointer = &pgdir->pde[pde_index];

  //assert(pde_pointer->val!=0);
  if(pde_pointer->present==0){
    if(prot!=0){
      PT* pt = kalloc();
      pde_pointer->val |= prot;
      pde_pointer->val = MAKE_PDE(pt, prot);
      PTE* pte_pointer = &(pt->pte[pte_index]);
      return pte_pointer;
    }
    else{
      return NULL;
    }
  }
  pde_pointer->val |= prot;
  PT* pt = PDE2PT(*pde_pointer);
  PTE* pte_pointer = &(pt->pte[pte_index]);
  //Log("%d",pte_pointer->val);
  //assert(pte_pointer->val!=0);
  assert((prot & ~7) == 0);
  return pte_pointer;
  //TODO();
}

void *vm_walk(PD *pgdir, size_t va, int prot)
{
  // Lab1-4: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  TODO();
}

void vm_map(PD *pgdir, size_t va, size_t len, int prot)
{
  // Lab1-4: map [PAGE_DOWN(va), PAGE_UP(va+len)) at pgdir, with prot
  // if have already mapped pages, just let pte->prot |= prot
  assert(prot & PTE_P);
  assert((prot & ~7) == 0);
  size_t start = PAGE_DOWN(va);
  size_t end = PAGE_UP(va + len);
  assert(start >= PHY_MEM);
  assert(end >= start);

  PTE* pte;
  while(start<end){
    
    pte = vm_walkpte(pgdir, start, prot);
    assert(pte!=NULL);
    if(pte->present==0){
      //pte->present = 1;
      void* add = kalloc();
      pte->val = MAKE_PTE(add,prot);
    }
    else{
      pte->val |= prot;
    }
    start+=PGSIZE;
  }
  //TODO();
}

void vm_unmap(PD *pgdir, size_t va, size_t len)
{
  // Lab1-4: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  // assert(ADDR2OFF(va) == 0);
  // assert(ADDR2OFF(len) == 0);
  // TODO();
}

void vm_copycurr(PD *pgdir)
{
  // Lab2-2: copy memory mapped in curr pd to pgdir
  TODO();
}

void vm_pgfault(size_t va, int errcode)
{
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}
