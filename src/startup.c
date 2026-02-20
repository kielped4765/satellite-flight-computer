#include <stdint.h>

/* Symbols from linker.ld */
extern uint32_t _etext;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern uint32_t _estack;

void Reset_Handler(void);
void Default_Handler(void);
int main(void);

/* Weak aliases - FreeRTOS port.c overrides SVC, PendSV, SysTick */
void NMI_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler"))); 

/* Interrupt vector table — MUST be first in Flash (see linker.ld) */
__attribute__((section(".isr_vector")))
uint32_t isr_vectors[] = {
    (uint32_t)&_estack, /* 0: Initial stack pointer */
    (uint32_t)&Reset_Handler, /* 1: Reset */
    (uint32_t)&NMI_Handler, /* 2: NMI */
    (uint32_t)&HardFault_Handler, /* 3: Hard Fault */
    (uint32_t)&MemManage_Handler, /* 4: Memory Management */
    (uint32_t)&BusFault_Handler, /* 5: Bus Fault */
    (uint32_t)&UsageFault_Handler,/* 6: Usage Fault */
    0, 0, 0, 0, /* 7-10: Reserved */
    (uint32_t)&SVC_Handler, /* 11: SVC (FreeRTOS) */
    (uint32_t)&DebugMon_Handler, /* 12: Debug Monitor */
    0, /* 13: Reserved */
    (uint32_t)&PendSV_Handler, /* 14: PendSV (FreeRTOS) */
    (uint32_t)&SysTick_Handler, /* 15: SysTick (FreeRTOS) */
};

void Reset_Handler(void) {
    uint32_t *src, *dst;
    /* Copy .data from Flash to SRAM */
    src = &_etext;
    dst = &_sdata;
    while (dst < &_edata) { *dst++ = *src++; }
    /* Zero .bss */
    dst = &_sbss;
    while (dst < &_ebss) { *dst++ = 0; }
    main();

        for (;;) {}
}
void Default_Handler(void) {
        for (;;) {} /* Hang — attach GDB to see which IRQ fired */
}