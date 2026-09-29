bits 32
section .text
    align 4
    dd 0x1BADB002              ; Magic number Multiboot
    dd 0x00                    ; Flagi
    dd - (0x1BADB002 + 0x00)   ; Suma kontrolna

global start
extern kernel_main

start:
    cli                        ; Wyłącz przerwania
    call kernel_main           ; Skok do naszego kernela w C
    hlt                        ; Zatrzymaj procesor
