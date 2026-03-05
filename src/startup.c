#include <stdint.h>

extern uint32_t _etext;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern uint32_t _estack;

void Reset_Handler(void);
void Default_Handler(void);
int main(void);

void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector")))
void (* const isr_vectors[])(void) = {
    (void (*)(void))&_estack,   /* 0: Initial stack pointer */
    Reset_Handler,               /* 1: Reset */
    NMI_Handler,                 /* 2: NMI */
    HardFault_Handler,           /* 3: Hard Fault */
    MemManage_Handler,           /* 4: Memory Management */
    BusFault_Handler,            /* 5: Bus Fault */
    UsageFault_Handler,          /* 6: Usage Fault */
    0, 0, 0, 0,                  /* 7-10: Reserved */
    SVC_Handler,                 /* 11: SVC (FreeRTOS) */
    DebugMon_Handler,            /* 12: Debug Monitor */
    0,                           /* 13: Reserved */
    PendSV_Handler,              /* 14: PendSV (FreeRTOS) */
    SysTick_Handler,             /* 15: SysTick (FreeRTOS) */
};

void Reset_Handler(void) {
    uint32_t *src, *dst;
    src = &_etext;
    dst = &_sdata;
    while (dst < &_edata) { *dst++ = *src++; }
    dst = &_sbss;
    while (dst < &_ebss) { *dst++ = 0; }
    main();
    for (;;) {}
}

void Default_Handler(void) {
    for (;;) {}
}
