// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define MAX_SWAP_PAGES 1024

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// struct swap_info {
//   struct swap* swap_page;
//   uint64 swap_page_va;
//   int pid;
//   char exist;
// };

struct swap_info swap_info_list[MAX_SWAP_PAGES];



struct live_page {
  uint64 va;
  uint64 pa;
  int pid;
  char use_bit;
  char exist;
};

struct spinlock live_page_lock;
struct spinlock swap_info_lock;

#define MAX_LIVE_PAGE 50

struct live_page live_page_list[MAX_LIVE_PAGE];

int total_live_page = 0;

int victim_index = 0;

// get a victim page using clock algorithm
struct live_page get_victim() {
  if(DEBUG) printf("get victim called\n");
  int i = 0;
  // acquire(&live_page_lock);
  while (i < MAX_LIVE_PAGE) {
    if (live_page_list[victim_index].exist == 1) {
      if (live_page_list[victim_index].use_bit == 1) {
        live_page_list[victim_index].use_bit = 0;
        victim_index = (victim_index + 1) % MAX_LIVE_PAGE;
      } else {
        // release(&live_page_lock);
        live_page_list[victim_index].exist = 0;
        return live_page_list[victim_index];
      }
    }
    i++;
  }

  for(int i = 0; i < MAX_LIVE_PAGE; i++) {
    if(live_page_list[i].exist == 1) {
      live_page_list[i].exist = 0;
      return live_page_list[i];
    }
  }
  // release(&live_page_lock);
  for(int i = 0; i < MAX_LIVE_PAGE; i++) {
    printf("exist: %d, use_bit: %d\n", live_page_list[i].exist, live_page_list[i].use_bit);
  }
  panic("no victim page");
}


// add a swap_info to the swap_info_list
void add_swap_info(int pid, uint64 swap_page_va, struct swap *swap_page) {
  if(DEBUG) printf("add_swap_info called: %p, %p\n", swap_page_va, swap_page);
  if(!swap_page) printf("swap_page is null %p",swap_page_va);
  for(int i = 0; i < MAX_SWAP_PAGES; i++) {
    if(swap_info_list[i].exist == 0) {
      swap_info_list[i].swap_page_va = swap_page_va;
      swap_info_list[i].swap_page = swap_page;
      swap_info_list[i].pid = pid;
      swap_info_list[i].exist = 1;
      return;
    }
  }
  panic("swap_info_list is full");
}


// add a page to live page list
void add_a_live_page(uint64 va, uint64 pa, int pid) {
  if(pid == 0) return;
  if(DEBUG) printf("add a live page called %d: %p, %p %d\n", total_live_page, va, pa, pid);
  if(va == TRAMPOLINE || va == TRAPFRAME) return;
  acquire(&live_page_lock);
  if (total_live_page >= MAX_LIVE_PAGE) {
    struct live_page victim = get_victim();
    if(DEBUG) printf("victim: %p, %p\n", victim.va, victim.pa);
    release(&live_page_lock);
    if(DEBUG) printf("before walk\n");
    pagetable_t victim_pagetable;
    // printf("add_a_live_page:\n");
    if((victim_pagetable = get_pagetable_by_pid(victim.pid))) {
      // printf("pid: %d\n", victim.pid);
      // panic("THIS SHOULD NOT HAPPEN");
    }
    pte_t *pte = walk(victim_pagetable, victim.va, 0);
    
    if(DEBUG) printf("HELLO\n");
    *pte |= PTE_SWAP;
    *pte &= ~PTE_V;
    sfence_vma();
    struct swap * new_swap_page = swapalloc();
    swapout(new_swap_page, (char *) PTE2PA(*pte));
    
    add_swap_info(victim.pid, victim.va, new_swap_page);
    printf(">>>swapped out va:%p pid:%d\n",victim.va, victim.pid);
    
    // *walk(victim_pagetable, victim_va, 0) = 0;
    sfence_vma();
    // add the victim page to free list
    kfree((void *)(victim.pa));
    total_live_page--;
    // release(&live_page_lock);
    acquire(&live_page_lock);
  }

  // acquire(&live_page_lock);
  for (int i = 0; i < MAX_LIVE_PAGE; i++) {
    if (live_page_list[i].exist == 0) {
      live_page_list[i].va = va;
      live_page_list[i].pa = pa;
      live_page_list[i].pid = pid;
      live_page_list[i].use_bit = 1;
      live_page_list[i].exist = 1;
      total_live_page++;
      break;
    }
  }
  release(&live_page_lock);
}

// remove a livepage from live page list
void remove_a_live_page(uint64 va, int pid) {
  if(DEBUG) printf("remove_a_live_page() called: %p, %d\n", va, pid);
  if(va == TRAMPOLINE || va == TRAPFRAME) return;
  acquire(&live_page_lock);
  for (int i = 0; i < MAX_LIVE_PAGE; i++) {
    if (live_page_list[i].va == va && live_page_list[i].pid == pid && live_page_list[i].exist == 1) {
      live_page_list[i].exist = 0;
      total_live_page--;
      break;
    }
  }
  release(&live_page_lock);
}



// remove a swap_info from the swap_info_list
struct swap_info remove_swap_info(uint64 swap_page_va, int pid) {
  if(DEBUG) printf("remove_swap_info() called: %p\n", swap_page_va);
  for(int i = 0; i < MAX_SWAP_PAGES; i++) {
    if(swap_info_list[i].swap_page_va == swap_page_va && swap_info_list[i].pid == pid && swap_info_list[i].exist == 1) {
      swap_info_list[i].exist = 0;
      if(!swap_info_list[i].swap_page) printf("swap_page is null\n");
      // printf("-s removed a swap_info: %p, %d\n", swap_page_va, pid);
      return swap_info_list[i];
    }
  }
  struct swap_info ret;
  ret.exist = -1;
  return ret;
}

void remove_and_free_swap_info_by_pid(int pid) {
  // acquire(&swap_info_lock);
  for(int i = 0; i < MAX_SWAP_PAGES; i++) {
    if(swap_info_list[i].pid == pid && swap_info_list[i].exist == 1) {
      swap_info_list[i].exist = 0;
      if(!swap_info_list[i].swap_page) printf("swap_page is null\n");
      swapfree(swap_info_list[i].swap_page);
      // printf("-sf removed and freed a swap_info: %d\n", pid);
      // return swap_info_list[i];
    }
  }
  // release(&swap_info_lock);
}

void set_use_bit1(uint64 va, int pid) {
  if(pid == 0) return;
  // if(DEBUG) printf("set_use_bit called: %p\n", va);
  acquire(&live_page_lock);
  for (int i = 0; i < MAX_LIVE_PAGE; i++) {
    if (live_page_list[i].exist == 1 && live_page_list[i].va == va && live_page_list[i].pid == pid) {
      live_page_list[i].use_bit = 1;
      break;
    }
  }
  release(&live_page_lock);
}

void set_use_bit2(uint64 va, pagetable_t pagetable) {
  // if(pid == 0) return;
  // if(DEBUG) printf("set_use_bit called: %p\n", va);
  acquire(&live_page_lock);
  for (int i = 0; i < MAX_LIVE_PAGE; i++) {
    // printf("set_use_bit:\n");
    if (live_page_list[i].exist == 1 && live_page_list[i].va == va && get_pagetable_by_pid(live_page_list[i].pid) == pagetable) {
      // printf("set_use_bit:\n");
      live_page_list[i].use_bit = 1;
      break;
    }
  }
  release(&live_page_lock);
}


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&live_page_lock, "live_page_lock");
  initlock(&swap_info_lock, "swap_info_lock");
  freerange(end, (void*)PHYSTOP);
  for(int i = 0; i < MAX_LIVE_PAGE; i++) {
    live_page_list[i].va = 0;
    live_page_list[i].pa = 0;
    live_page_list[i].pid = 0;
    live_page_list[i].use_bit = 0;
    live_page_list[i].exist = 0;
  }
  for(int i = 0; i < MAX_SWAP_PAGES; i++) {
    swap_info_list[i].swap_page = 0;
    swap_info_list[i].swap_page_va = 0;
    swap_info_list[i].pid = 0;
    swap_info_list[i].exist = 0;
  }
  swapinit();
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// live page count
int get_live_page_count(int pid) {
  int count = 0;
  acquire(&live_page_lock);
  for(int i = 0; i < MAX_LIVE_PAGE; i++) {
    if(live_page_list[i].exist == 1 && live_page_list[i].pid == pid) {
      count++;
    }
  }
  release(&live_page_lock);
  return count;
}

