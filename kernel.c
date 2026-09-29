// --- 1. DEFINICJE STRUKTUR I PORTÓW I/O ---
#define WIDTH 1024
#define HEIGHT 768

// Multiboot Structure do odczytu adresu LFB (Linear Framebuffer)
typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms[4];
    unsigned int mmap_length;
    unsigned int mmap_addr;
    unsigned int drives_length;
    unsigned int drives_addr;
    unsigned int config_table;
    unsigned int boot_loader_name;
    unsigned int apm_table;
    unsigned int vbe_control_info;
    unsigned int vbe_mode_info;
    unsigned short vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;
    unsigned long long framebuffer_addr;
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned char framebuffer_bpp;
} __attribute__((packed)) multiboot_info_t;

static unsigned int* lfb = (unsigned int*)0xFD000000; // Domyślny adres ramki graficznej QEMU/VirtualBox VBE

// Pamięć podręczna Double-Buffering (zmniejsza migotanie)
static unsigned int backbuffer[WIDTH * HEIGHT];

// Myszka PS/2
int mouse_x = WIDTH / 2;
int mouse_y = HEIGHT / 2;

// Porty I/O
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// --- 2. SILNIK GRAFICZNY (PIKSELE, KWADRATY, OKNA) ---

void draw_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        backbuffer[y * WIDTH + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, unsigned int color) {
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            draw_pixel(j, i, color);
        }
    }
}

void flip_screen() {
    unsigned int* dest = lfb;
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        dest[i] = backbuffer[i];
    }
}

// Czcionka bitmapowa 8x8 dla tekstów GUI
const unsigned char font8x8[2][8] = {
    {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // 'A'
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}  // 'B'
};

void draw_char_simple(int x, int y, unsigned int color) {
    draw_rect(x, y, 6, 8, color); // Uproszczony wskaźnik literowy
}

void draw_string(const char* str, int x, int y, unsigned int color) {
    int cx = x;
    while (*str) {
        if (*str != ' ') {
            draw_char_simple(cx, y, color);
        }
        cx += 8;
        str++;
    }
}

// Rysowanie Myszki
void draw_mouse(int x, int y) {
    for (int i = -5; i <= 5; i++) {
        draw_pixel(x + i, y, 0xFFFFFF); // Bielsza linia pozioma
        draw_pixel(x, y + i, 0xFFFFFF); // Bielsza linia pionowa
    }
}

// --- 3. STEROWNIK MYSZKI PS/2 ---

void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        while (timeout-- && (inb(0x64) & 1) == 0);
    } else {
        while (timeout-- && (inb(0x64) & 2));
    }
}

void mouse_write(unsigned char write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

unsigned char mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void init_mouse() {
    mouse_wait(1);
    outb(0x64, 0xA8); // Włącz port myszy
    mouse_write(0xF4); // Włącz przesyłanie pakietów
    mouse_read();     // ACK
}

void update_mouse() {
    if (inb(0x64) & 0x01) {
        unsigned char status = inb(0x60);
        if (status & 0x08) { // Poprawny pakiet myszy
            char rel_x = mouse_read();
            char rel_y = mouse_read();

            mouse_x += rel_x;
            mouse_y -= rel_y;

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= WIDTH) mouse_x = WIDTH - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= HEIGHT) mouse_y = HEIGHT - 1;
        }
    }
}

// --- 4. RYSOWANIE INTERFEJSU GRAFICZNEGO (GUI) ---

void draw_window(int x, int y, int w, int h, const char* title) {
    draw_rect(x, y, w, h, 0xC0C0C0); // Szare tło okna
    draw_rect(x, y, w, 25, 0x000080); // Ciemnoniebieski pasek
    draw_rect(x + w - 20, y + 3, 16, 18, 0xFF0000); // Zamknij [X]
    draw_rect(x + w - 40, y + 3, 16, 18, 0x00FF00); // Minimalizacja [_]
    draw_string(title, x + 8, y + 8, 0xFFFFFF);
}

// Pasek postępu (Progress Bar)
void draw_progress_bar(int x, int y, int w, int h, int progress) {
    draw_rect(x, y, w, h, 0x808080); // Tło paska
    int fill = (w * progress) / 100;
    draw_rect(x + 2, y + 2, fill - 4, h - 4, 0x0000FF); // Niebieski postęp
}

// Panel Logów Systemowych
void draw_logs_window(int x, int y, int w, int h) {
    draw_window(x, y, w, h, "LOGI SYSTEMOWE");
    draw_rect(x + 10, y + 35, w - 20, h - 45, 0x000000); // Czarny ekran terminala
    draw_string("[OK] Inicjalizacja sterownika VBE 1024x768", x + 15, y + 45, 0x00FF00);
    draw_string("[OK] Inicjalizacja myszki PS/2 na portach 0x60/0x64", x + 15, y + 65, 0x00FF00);
    draw_string("[OK] Zaladowano interfejs graficzny GUI", x + 15, y + 85, 0x00FF00);
    draw_string("[INFO] Witam w moim autorskim systemie!", x + 15, y + 105, 0x00FFFF);
}

// Mini-Przeglądarka Internetowa (Web Browser Simulator)
void draw_browser_window(int x, int y, int w, int h) {
    draw_window(x, y, w, h, "PRZEGLADARKA - http://lynex.os");
    
    // Pasek adresu URL
    draw_rect(x + 10, y + 32, w - 80, 22, 0xFFFFFF);
    draw_string("http://lynex.os/index.html", x + 15, y + 38, 0x000000);
    draw_rect(x + w - 65, y + 32, 55, 22, 0x808080); // Przycisk GO
    draw_string("GO", x + w - 50, y + 38, 0xFFFFFF);

    // Zawartość strony (Renderowanie HTML)
    draw_rect(x + 10, y + 60, w - 20, h - 70, 0xFFFFFF);
    draw_string("=== WITAM W MOIM SYSTEMIE ===", x + 30, y + 80, 0x000000);
    draw_string("To jest autorska strona wewnatrz przegladarki!", x + 30, y + 110, 0x000080);
    draw_string("System operacyjny stworzony bez uzycia Linuksa.", x + 30, y + 130, 0x800000);
}

// --- 5. GŁÓWNA PĘTLA SYSTEMU ---

void kernel_main(multiboot_info_t* mb_info) {
    // Odczyt rzeczywistego adresu LFB jeśli podany przez GRUB
    if (mb_info && mb_info->framebuffer_addr) {
        lfb = (unsigned int*)(unsigned long)mb_info->framebuffer_addr;
    }

    init_mouse();

    int progress = 0;

    while (1) {
        update_mouse();

        // 1. Tło Pulpitu (Morski / Teall)
        draw_rect(0, 0, WIDTH, HEIGHT, 0x008080);

        // 2. Pasek zadań na dole ekranu
        draw_rect(0, HEIGHT - 40, WIDTH, 40, 0xC0C0C0);
        draw_rect(5, HEIGHT - 35, 80, 30, 0x808080); // Przycisk START
        draw_string("START", 20, HEIGHT - 25, 0xFFFFFF);

        // 3. Okno Powitalne z Paskiem Postępu
        draw_window(50, 50, 400, 180, "SYSTEM OS - STARTUP");
        draw_string("Ladowanie moduow systemu...", 70, 90, 0x000000);
        draw_progress_bar(70, 120, 360, 25, progress);

        if (progress < 100) {
            progress++;
        }

        // 4. Konsola Logów Systemowych
        draw_logs_window(50, 260, 400, 220);

        // 5. Mini-Przeglądarka Internetowa
        draw_browser_window(480, 50, 500, 430);

        // 6. Rysowanie Kursora Myszki
        draw_mouse(mouse_x, mouse_y);

        // 7. Podmiana bufora (Render do ekranu)
        flip_screen();
    }
}
