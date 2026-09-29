// --- 1. Obsługa Pamięci Ekranu VGA ---
volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
int kursor_x = 0;
int kursor_y = 2; // Zaczynamy pisanie pod nagłówkiem

void czysc_ekran() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (unsigned short)' ' | (0x0F << 8); // Biały tekst na czarnym tłu
    }
}

void pisz_tekst(const char* str, int wiersz, int kolumna, unsigned char kolor) {
    int pozycja = wiersz * 80 + kolumna;
    for (int i = 0; str[i] != '\0'; i++) {
        vga_buffer[pozycja + i] = (unsigned short)str[i] | (kolor << 8);
    }
}

// Wyświetlanie pojedynczego znaku z obsługą Nowej Linii i Backspace
void pisz_znak(char c) {
    if (c == '\n') {
        kursor_x = 0;
        kursor_y++;
    } else if (c == '\b') { // Backspace
        if (kursor_x > 0) {
            kursor_x--;
            int pos = kursor_y * 80 + kursor_x;
            vga_buffer[pos] = (unsigned short)' ' | (0x0F << 8);
        }
    } else {
        int pos = kursor_y * 80 + kursor_x;
        vga_buffer[pos] = (unsigned short)c | (0x0E << 8); // Żółty tekst
        kursor_x++;
        if (kursor_x >= 80) {
            kursor_x = 0;
            kursor_y++;
        }
    }
}

// --- 2. Obsługa Portów I/O w C (Asembler Inline) ---

// Odczyt bajtu z portu I/O
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// --- 3. Tablica Scancode PS/2 -> Znak ASCII ---
const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

// --- 4. Główna Pętla Kernela ---
void kernel_main(void) {
    czysc_ekran();

    // Rysowanie nagłówka (Jasnoniebieskie tło)
    for (int i = 0; i < 80; i++) {
        vga_buffer[i] = (unsigned short)' ' | (0x1F << 8);
    }
    pisz_tekst("=== MOJ AUTORSKI SYSTEM OPERACYJNY ===", 0, 20, 0x1F);
    pisz_tekst("Witam w moim systemie! Wpisz cos z klawiatury:", 1, 0, 0x0A);

    // Pętla odczytu klawiatury (Polling / Polling Portu 0x60 i 0x64)
    while (1) {
        // Sprawdzenie bitu 0 na porcie 0x64 (Czy dane z klawiatury są gotowe)
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60); // Odczytujemy scancode z portu 0x60

            // Bit 7 oznacza zwolnienie klawisza. Jeśli bit 7 jest równy 0, klawisz został wciśnięty!
            if (!(scancode & 0x80)) {
                char ch = scancode_to_ascii[scancode];
                if (ch != 0) {
                    pisz_znak(ch);
                }
            }
        }
    }
}
