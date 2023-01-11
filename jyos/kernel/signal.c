#include <signal.h>
#include <process.h>

#include <syscall.h>

extern struct scheduler  sched_ctx;

void *default_signal_handlers[_SIG_MAX] = { };


void *signal_dispatch(){

    if(!__current->sig_pending){
        return 0;
    }

    int cur_sig =
        31 - __builtin_clz(__current->sig_pending & (~__current->sig_mask));

    __current->sig_pending &= ~_SIGNAL(cur_sig);

    if(!__current->signal_handlers[cur_sig] &&
       !default_signal_handlers[cur_sig]){
        return 0;
    }

    uint32_t ustack = (uint32_t)__current->user_stack_top & 0xFFFFFFF0;

    if((int)(ustack - U_STACK_END) < (int)sizeof(struct sig_struct)){
        return;
    }

    struct sig_struct *sig =
        (struct sig_struct *)(ustack - sizeof(struct sig_struct));

    sig->prev_regs      = __current->regs;
    sig->sig_num        = cur_sig;
    sig->signal_handler = __current->signal_handlers[cur_sig];

    if(!sig->signal_handler){
        sig->signal_handler = default_signal_handlers[cur_sig];
    }

    __current->sig_mask |= _SIGNAL(cur_sig);

    return sig;

}

__DEFINE_SYSTEMCALL_1(void, sigreturn,
                      struct sig_struct, *sig_ctx){

    __current->regs      = sig_ctx->prev_regs;
    __current->sig_mask &= ~_SIGNAL(sig_ctx->sig_num);

    schedule();

}

__DEFINE_SYSTEMCALL_3(int, sigtaskmask,
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

    if(signum < 0 || signum >= _SIG_MAX){
        return -1;
    }

    if((1 << signum) & _SIGNAL_UNMASKABLE){
        return -1;
    }

    __current->signal_handlers[signum] = handler;

    return 0;

}
