ENTRY(start_)

SECTIONS {
    . = 0x100000;
    __init_hhk_start = . ;

    .hhk_init_text BLOCK(4K) : {
        * (.multiboot)
        build/obj/arch/x86/*.o (.hhk_init)
        build/obj/arch/x86/*.o (.text)
    }

    .hhk_init_bss BLOCK(4K) : {
        build/obj/arch/x86/*.o (.bss)
    }

    .hhk_init_data BLOCK(4K) : {
        build/obj/arch/x86/*.o (.data)
    }

    .hhk_init_rodata BLOCK(4K) : {
        build/obj/arch/x86/*.o (.rodata)
    }
    __init_hhk_end = ALIGN(4K);

    /* Relocation of our higher half kernel */
    . += 0xC0000000;
    
    /* 好了，我们的内核…… */
    .text BLOCK(4K) : AT ( ADDR(.text) - 0xC0000000 ) {
        __kernel_start = .;
        build/obj/kernel/*.o (.text)
        build/obj/hal/*.o (.text)
    }

    __user_text_start = ALIGN(4K);
    .user_text BLOCK(4K) : AT ( ADDR(.user_text) - 0xC0000000 ) {
        build/obj/kernel/*.o (.user_text)
    }
    __user_text_end = ALIGN(4K);

    .bss BLOCK(4K) : AT ( ADDR(.bss) - 0xC0000000 ) {
        build/obj/kernel/*.o (.bss)
        build/obj/hal/*.o (.bss)
    }

    .data BLOCK(4k) : AT ( ADDR(.data) - 0xC0000000 ) {
        build/obj/kernel/*.o (.data)
        build/obj/hal/*.o (.data)
    }

    .rodata BLOCK(4K) : AT ( ADDR(.rodata) - 0xC0000000 ) {
        build/obj/kernel/*.o (.rodata)
        build/obj/hal/*.o (.rodata)
    }

    /* temp kernel page _table */
    .kpg BLOCK(4K) : AT (  ADDR(.kpg) - 0xC0000000  ) {
        build/obj/arch/x86/*.o (.kpg)
    }
    __kernel_end     = ALIGN(4K);
    __pg_mount_point = ALIGN(4K);
    . += 20K;
    __pd_mount_point = ALIGN(4M);
    . += 4M;
}
