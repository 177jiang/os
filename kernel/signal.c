#include <signal.h>
#include <stddef.h>
#include <process.h>
#include <status.h>
#include <junistd.h>
#include <spike.h>
#include <syscall.h>

extern struct scheduler  sched_ctx;

void __USER_SPACE__ default_signal_hd(int sig){
    _exit(sig);
}

void __USER_SPACE__ default_signal_user(int sig){
    kprintf_error("Kill task at %d\n", getpid());
}

void *default_signal_handlers[_SIG_MAX] = {

    [_SIGINT]  = default_signal_hd,
    [_SIGTERM] = default_signal_hd,
    [_SIGKILL] = default_signal_hd,

    [_SIG_USER] = default_signal_user,

};


void *signal_dispatch(){

    if(!__current->sig_pending){
        return 0;
    }

    int cur_sig =
        31 - __builtin_clz(__current->sig_pending &
                                ~(__current->sig_mask | __current->sig_doing));

    __SIGCLEAR(__current->sig_pending, cur_sig);

    if(!cur_sig){
        return 0;
    }

    if(!__current->signal_handlers[cur_sig] &&
       !default_signal_handlers[cur_sig]){
        return 0;
    }

    int ustack = (int)__current->user_stack_top & 0xFFFFFFF0;

    if((ustack - (int)U_STACK_END) < sizeof(struct sig_struct)){
        return 0;
    }

    /* save old , because if sig causes a page_fault , we will lose old context */
    isr_param regs = __current->regs;

    struct sig_struct *sig =
        (struct sig_struct *)(ustack - sizeof(struct sig_struct));

    sig->prev_regs      = regs;
    sig->sig_num        = cur_sig;
    sig->signal_handler = __current->signal_handlers[cur_sig];

    if(!sig->signal_handler){
        sig->signal_handler = default_signal_handlers[cur_sig];
    }

    __SIGSET(__current->sig_doing, cur_sig);

    return sig;

}

int signal_send(pid_t pid, int sig){

    if(sig < 0 || sig >= _SIG_MAX){
        __current->k_status = EINVL;
        return -1;
    }

    struct task_struct * task;

    do{

        if(pid > 0){
            task = get_task(pid);
        }else if(pid == 0){
            task = __current;
        }else if(pid < -1){
            task = get_task(-pid);
            break;
        }else if(pid == -1){
            __current->k_status = EINVL;
            /* send all */
            return -1;
        }

        if(TASK_TERMINATED(task->state)){
            __current->k_status = EINVL;
            return -1;
        }

        __SIGSET(task->sig_pending, sig);

        return 0;

    }while(0);

    struct task_struct *pos, *n;
    list_for_each(pos, n, &task->group, group){
        __SIGSET(pos->sig_pending, sig);
    }
    return 0;
}

__DEFINE_SYSTEMCALL_1(void, sigreturn,
                      struct sig_struct, *sig_ctx){

    __current->regs   = sig_ctx->prev_regs;

    __current->flags &= ~TASK_FINPAUSE;

    __SIGCLEAR(__current->sig_doing, sig_ctx->sig_num);

    schedule();

}

__DEFINE_SYSTEMCALL_3(int, sigprocmask,
                      int, how,
                      const signal_t, *set,
                      signal_t, *old){

    if(how == _SIG_BLOCK){

        __current->sig_mask |= *set;

    }else if(how == _SIG_UNBLOCK){

        __current->sig_mask &= ~(*set);

    }else if(how == _SIG_SETMASK){

        __current->sig_mask = *set;

    }else{
        return 0;
    }

    __current->sig_mask &= ~(_SIGNAL_UNMASKABLE);

    return 1;

}

__DEFINE_SYSTEMCALL_2(int, signal,
                      int, signum,
                      signal_handler, handler){

    if(signum <= 0 || signum >= _SIG_MAX){
        return -1;
    }

    if(__SIGNAL(signum) & _SIGNAL_UNMASKABLE){
        return -1;
    }

    __current->signal_handlers[signum] = handler;

    return 0;

}

__DEFINE_SYSTEMCALL_2(int, kill, pid_t, pid, int, signum){
    return signal_send(pid, signum);
}

static  void __pause_interior(){

    __current->flags |= TASK_FINPAUSE;
    __SYSCALL_INTERRUPTIBLE({
        while((__current->flags & TASK_FINPAUSE)){
            sched_yield();
        }
    })
    __current->k_status = EINTR;

}

__DEFINE_SYSTEMCALL_0(int, pause){

    __pause_interior();
    return -1;

}
__DEFINE_SYSTEMCALL_1(int, sigpending,
                      signal_t, *set){

    *set  =__current->sig_pending;
    return 0;

}

__DEFINE_SYSTEMCALL_1(int, sigsuspend,
                      signal_t, *mask){

    signal_t t          = __current->sig_mask;
    __current->sig_mask = (*mask) & ~(_SIGNAL_UNMASKABLE);

    __pause_interior();

    __current->sig_mask = t;

    return -1;

}








