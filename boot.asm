bits 32
section .text
    align 4
    dd 0x1BADB002              ; Magic number Multiboot
    dd 0x00000004              ; Flagi: Włącz tryb graficzny VBE
    dd - (0x1BADB002 + 0x00000004) ; Suma kontrolna

    ; Nagłówek graficzny Multiboot (Mode Control)
    dd 0, 0, 0, 0, 0
    dd 0                       ; 0 = Tryb graficzny LFB
    dd 1024                    ; Szerokość
    dd 768                     ; Wysokość
    dd 32                      ; Głębia koloru (32-bit BGR/RGB)

global start
extern kernel_main

start:
    cli                        ; Wyłącz przerwania
    call kernel_main           ; Skok do kernela w C
    hlt                        ; Zatrzymaj procesor
