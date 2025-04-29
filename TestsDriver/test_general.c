#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include <time.h>

#define DEV_LEDS "/dev/leds"
#define DEV_BUTTONS "/dev/buttons"
#define DEV_SPEAKER "/dev/speaker"

void test_leds() {
    int fd = open(DEV_LEDS, O_RDWR);
    if (fd < 0) {
        perror("Error abriendo /dev/leds");
        return;
    }

    printf("\n--- Test de LEDs ---\n");

    uint8_t patterns[] = {
        0b00000001,  // solo LED 0
        0b00000011,  // LED 0 y 1
        0b00011100,  // LED 2,3,4
        0b00111111,  // todos
        0b00000000   // ninguno
    };

    for (int i = 0; i < sizeof(patterns); i++) {
        write(fd, &patterns[i], 1);
        printf("LEDs: %02x\n", patterns[i]);
        sleep(1);
    }

    close(fd);
}

void test_speaker() {
    int fd = open(DEV_SPEAKER, O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo /dev/speaker");
        return;
    }

    printf("\n--- Test de Speaker ---\n");
    for (int i = 0; i < 3; ++i) {
        write(fd, "1", 1); // Encender
        usleep(300000);
        write(fd, "0", 1); // Apagar
        usleep(300000);
    }

    close(fd);
}

void test_buttons_blocking() {
    int fd = open(DEV_BUTTONS, O_RDONLY);
    if (fd < 0) {
        perror("Error abriendo /dev/buttons (bloqueante)");
        return;
    }

    printf("\n--- Esperando pulsación de botón (bloqueante) ---\n");
    char c;
    for (int i = 0; i < 3; ++i) {
        read(fd, &c, 1);
        printf("Botón presionado: %c\n", c);
    }

    close(fd);
}

void test_buttons_nonblocking() {
    int fd = open(DEV_BUTTONS, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Error abriendo /dev/buttons (no bloqueante)");
        return;
    }

    printf("\n--- Prueba de botones (no bloqueante con select) ---\n");

    fd_set rfds;
    struct timeval tv;
    int retval;
    char c;

    for (int i = 0; i < 10; ++i) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        retval = select(fd + 1, &rfds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select()");
            break;
        } else if (retval) {
            read(fd, &c, 1);
            printf("Botón presionado: %c\n", c);
        } else {
            printf("Esperando botón...\n");
        }
    }

    close(fd);
}

int main() {
    test_leds();
    test_speaker();
    test_buttons_blocking();
    test_buttons_nonblocking();

    return 0;
}
