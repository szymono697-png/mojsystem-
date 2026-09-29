// Bezpośredni adres pamięci tekstowej VGA
volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;

void czysc_ekran() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (unsigned short) ' ' | (0x0F << 8); // Czarne tło
    }
}

void pisz_tekst(const char* str, int wiersz, int kolumna, unsigned char kolor) {
    int pozycja = wiersz * 80 + kolumna;
    for (int i = 0; str[i] != '\0'; i++) {
        vga_buffer[pozycja + i] = (unsigned short)str[i] | (kolor << 8);
    }
}

void kernel_main(void) {
    czysc_ekran();
    
    // Rysowanie prostego okienka / nagłówka (jasnoniebieskie tło)
    for (int i = 0; i < 80; i++) {
        vga_buffer[i] = (unsigned short) ' ' | (0x1F << 8);
    }
    
    // Napis powitalny w Twoim własnym systemie!
    pisz_tekst("=== MOJ AUTORSKI SYSTEM OPERACYJNY ===", 0, 20, 0x1F);
    pisz_tekst("Witam w moim systemie", 10, 28, 0x0A); // Zielony tekst
    pisz_tekst("System uruchomiony pomyślnie z pliku ISO!", 14, 18, 0x0E);
    
    while(1); // Pętla główna kernela
}
