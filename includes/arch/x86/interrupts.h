#ifndef __jyos_interrupts_h_
#define  __jyos_interrupts_h_

#define FAULT_DIVISION_ERROR            0
#define FAULT_TRAP_DEBUG_EXCEPTION      1
#define INT_NMI                         2
#define TRAP_BREAKPOINT                 3
#define TRAP_OVERFLOW                   4
#define FAULT_BOUND_EXCEED              5
#define FAULT_INVALID_OPCODE            6
#define FAULT_NO_MATH_PROCESSOR         7
#define ABORT_DOUBLE_FAULT              8
#define FAULT_RESERVED_0                9
#define FAULT_INVALID_TSS               10
#define FAULT_SEG_NOT_PRESENT           11
#define FAULT_STACK_SEG_FAULT           12
#define FAULT_GENERAL_PROTECTION        13
#define FAULT_PAGE_FAULT                14
#define FAULT_RESERVED_1                15
#define FAULT_X87_FAULT                 16
#define FAULT_ALIGNMENT_CHECK           17
#define ABORT_MACHINE_CHECK             18
#define FAULT_SIMD_FP_EXCEPTION         19
#define FAULT_VIRTUALIZATION_EXCEPTION  20
#define FAULT_CONTROL_PROTECTION        21

#define JYOS_SYS_PANIC                  32
#define JYOS_SYS_CALL                   33


#define EX_INTERRUPT_BEGIN              200

// Keyboard
#define PC_KBD_IV                       201

#define RTC_TIMER_IV                    210

// 来自APIC的中断有着最高的优先级。
// APIC related
#define APIC_ERROR_IV                   250
#define APIC_LINT0_IV                   251
#define APIC_SPIV_IV                    252
#define APIC_TIMER_IV                   253

#define PC_AT_IRQ_RTC                   8
#define PC_AT_IRQ_KBD                   1

#define REGS_EBP      16
#define REGS_ESI      (REGS_EBP + 4)
#define REGS_EDI      (REGS_ESI + 4)
#define REGS_EDX      (REGS_EDI + 4)
#define REGS_ECX      (REGS_EDX + 4)
#define REGS_EBX      (REGS_ECX + 4)
#define REGS_EAX      (REGS_EBX + 4)
#define REGS_ESP      (REGS_EAX + 4)

#define REGS_VECTOR   (REGS_ESP     + 4)
#define REGS_ERRCODE  (REGS_VECTOR  + 4)
#define REGS_EIP      (REGS_ERRCODE + 4)
#define REGS_CS       (REGS_EIP     + 4)
#define REGS_EFLAGS   (REGS_CS      + 4)
#define REGS_UESP     (REGS_EFLAGS  + 4)
#define REGS_SS       (REGS_UESP    + 4)

#ifndef    __ASM_S_
typedef struct {

  unsigned int gs;
  unsigned int fs;
  unsigned int es;
  unsigned int ds;

  unsigned int ebp;
  unsigned int esi;
  unsigned int edi;
  unsigned int edx;
  unsigned int ecx;
  unsigned int ebx;
  unsigned int eax;
  unsigned int esp;
  unsigned int vector;
  unsigned int err_code;
  unsigned int eip;
  unsigned int cs;
  unsigned int eflags;
  unsigned int user_esp;
  unsigned int ss;

} __attribute__((packed)) isr_param;


typedef void (*interrupt_function)(const isr_param*) ;

#define INTERRUPTS_VECTOR_SIZE 256

// #pragma region ISR_DECLARATION
void _asm_isr0();
void _asm_isr1();
void _asm_isr2();
void _asm_isr3();
void _asm_isr4();
void _asm_isr5();
void _asm_isr6();
void _asm_isr7();
void _asm_isr8();
void _asm_isr9();
void _asm_isr10();
void _asm_isr11();
void _asm_isr12();
void _asm_isr13();
void _asm_isr14();
void _asm_isr15();
void _asm_isr16();
void _asm_isr17();
void _asm_isr18();
void _asm_isr19();
void _asm_isr20();
void _asm_isr21();

void _asm_isr32();
void _asm_isr33();

void _asm_isr201();

void _asm_isr210();

void _asm_isr250();
void _asm_isr251();
void _asm_isr252();
void _asm_isr253();
void _asm_isr254();

// #pragma endregion

void intr_setvector(const unsigned int vector, interrupt_function fun);

void intr_unsetvector(const unsigned int vector, interrupt_function fun);

void intr_set_fallback_handler(interrupt_function fun);

void _intr_handler(isr_param* param);

void _interrupts_handler(isr_param* param);

void intr_routine_init();

#endif

#endif
