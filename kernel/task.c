#include "task.h"
#include "paging.h"
#include "buddy.h"
#include "monitor.h"
#include "descriptor.h"
#include "initrd.h"


static void showTask();
Task ready[MAX_TASK];
int run;

int load_task(int inode){
    int size; 
    u32 addr = load_initrd(inode, &size);
    addTask( addr, addr+(u32)size );
    m_printf("load task addr:%x size:%x\n",addr,size);
}
void ini_task(){

    memset( (u8*)ready, 0, sizeof(Task)*MAX_TASK );
    //addTask( (u32)&ini_begin, (u32)&ini_end );
    //addTask( (u32)&test_begin, (u32)&test_end );
    load_task(11);
    load_task(16);
    run = -1;
    showTask();
    ibreak();
}
void save_context(Task* now,registers_t regs){
    
    now->eip = regs.eip;
    now->cs  = regs.cs;
    now->eflags = regs.eflags;
    now->esp = regs.useresp;
    now->ss = regs.ss;
 
    now->ds  = regs.ds;
    now->edi = regs.edi;
    now->esi = regs.esi;
    now->ebp = regs.ebp;
    now->ebx = regs.ebx;
    now->edx = regs.edx;
    now->ecx = regs.ecx;
    now->eax = regs.eax;
}
void restore_context(Task next,registers_t* regs){

    regs->eip = next.eip;
    regs->cs = next.cs;

    if(next.eflags!=0)
        regs->eflags = next.eflags;

    regs->useresp = next.esp;
    regs->ss = next.ss;

    regs->ds  = next.ds;
    regs->edi = next.edi;
    regs->esi = next.esi;
    regs->ebp = next.ebp;
    regs->ebx = next.ebx;
    regs->edx = next.edx;
    regs->ecx = next.ecx;
    regs->eax = next.eax;
}
void switchTask(registers_t* regs){

    int i;
    
    //unmap the task
    if(run!=-1 && ready[run].status == 2){

        Task* now = &ready[run];
        save_context(now,*regs);

        for (i = 0; i < now->mlen; i++) {
            
            clear_map( now->mmap[i].vaddr );
        }
        now->status = 1;
    }

    
    //pick up which to run
    //TODO schedual should be a interface
    //just find next ready task

    for (
        i = run == MAX_TASK-1 ? 0 : run+1 ;
        i!=run ;
        i = i==MAX_TASK-1 ? 0 : i+1
    ){
        if( ready[i].status==1 )
            break;
    }


    run = i;
    //m_printf("pick up %d\n",run);
    Task* next = &ready[i];
    for (i = 0; i < next->mlen; i++) {
        map_page( next->mmap[i].vaddr, next->mmap[i].paddr, next->mmap[i].rw );
    }

    restore_context(*next, regs);
    set_kernel_stack(next->k_esp);

    next->status = 2;
    
}

static void showTask(){
    int i;
    for (i = 0; i < MAX_TASK; i++) {
        Task* t = &ready[i];
        if(t->status){
            m_printf("id:%d  eip:%x  esp:%x\n",i,t->eip,t->esp);
            int j;
            for (j = 0; j < t->mlen; j++) {
                m_printf("\tvaddr:%x  paddr:%x  rw:%x\n",
                    t->mmap[j].vaddr ,t->mmap[j].paddr ,t->mmap[j].rw);
            }
        }
    }
}
int addTask(u32 begin,u32 end){
    m_printf("add task begin:%x  end:%x  ",begin,end);

    int t_num;

    for(t_num=0;t_num<MAX_TASK;t_num++){
        if(ready[t_num].status == 0)
          break;
    }

    if(t_num == MAX_TASK){
        //TODO panic
        return 0;
    }
    if( (u32)( begin<<22 ) != 0){
        //begin must align 4k
        return 0;
    }
    if( (end-begin)>>12 > TASK_MAX_PAGE ){
        //to big to map
        return 0;
    }
    m_printf("insert pos:%d\n",t_num);

    Task* t = &ready[t_num];
    memset((u8*)t, 0, sizeof(Task));
    u32 load = LOADPLACE;
    while(begin < end){
        t->mmap[ t->mlen ].vaddr = load;
        t->mmap[ t->mlen ].rw = 0;
        t->mmap[ t->mlen++ ].paddr = begin;
        load+=0x1000;
        begin+=0x1000;
    }

    //malloc stack
    u32 stack = k_malloc(1);
    if(stack == -1){
        //TODO panic
    }
    
    //map stack
    t->mmap[ t->mlen ].vaddr = LOADSTACK;
    t->mmap[ t->mlen ].rw = 1;
    t->mmap[ t->mlen++ ].paddr = stack;

    //malloc kernel stack used when trap into ring0
    u32 k_stack = k_malloc(1);
    if(k_stack == -1){
        //TODO panic
    }
    
    //map kernel_stack
    t->mmap[ t->mlen ].vaddr = KERNELSTACK;
    t->mmap[ t->mlen ].rw = 0;
    t->mmap[ t->mlen++ ].paddr = k_stack;

    t->eip = LOADPLACE;
    t->cs = 0x1B;
    //t->eflags ?
    t->ss = 0x23;
    t->ds = 0x23;
    t->esp = LOADSTACK + 0x1000;    //top of the stack(page)
    t->k_esp = KERNELSTACK + 0x1000;
    t->status = 1;

    return 1;
    
}
void killTask(int pid){
    ready[pid].status=0;
    int i;
    for (i = 0; i < ready[pid].mlen; i++) {
        k_free( ready[pid].mmap[i].paddr );
    }
}
