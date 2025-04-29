#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define DEV_LEDS "/dev/leds"
#define DEV_BUTTONS "/dev/buttons"
#define DEV_SPEAKER "/dev/speaker"

void test_open_close_leds() {
    printf("\n--- Test: Apertura y cierre de /dev/leds ---\n");
    int fd = open(DEV_LEDS, O_RDWR);
    if (fd < 0) {
        perror("Error al abrir /dev/leds");
        return;
    }
    printf("Abierto correctamente /dev/leds\n");

    if (close(fd) != 0) {
        perror("Error al cerrar /dev/leds");
    } else {
        printf("Cerrado correctamente /dev/leds\n");
    }
}

void test_open_close_speaker() {
    printf("\n--- Test: Apertura y cierre de /dev/speaker ---\n");
    int fd = open(DEV_SPEAKER, O_WRONLY);
    if (fd < 0) {
        perror("Error al abrir /dev/speaker");
        return;
    }
    printf("Abierto correctamente /dev/speaker\n");

    if (close(fd) != 0) {
        perror("Error al cerrar /dev/speaker");
    } else {
        printf("Cerrado correctamente /dev/speaker\n");
    }
}

void test_buttons_exclusive_open() {
    printf("\n--- Test: Exclusividad en /dev/buttons ---\n");

    int fd1 = open(DEV_BUTTONS, O_RDONLY);
    if (fd1 < 0) {
        perror("Error al abrir /dev/buttons (primer intento)");
        return;
    }
    printf("Primer descriptor /dev/buttons abierto correctamente\n");

    int fd2 = open(DEV_BUTTONS, O_RDONLY | O_NONBLOCK);
    if (fd2 < 0) {
        if (errno == EBUSY) {
            printf("Correcto: segundo intento bloqueado (EBUSY)\n");
        } else {
            perror("Error inesperado al abrir /dev/buttons (segundo intento)");
        }
    } else {
        printf("Error: segundo descriptor abierto cuando debería fallar\n");
        close(fd2);
    }

    close(fd1);
    printf("Cerrado primer descriptor\n");

    // Probar que después se puede volver a abrir
    fd1 = open(DEV_BUTTONS, O_RDONLY | O_NONBLOCK);
    if (fd1 < 0) {
        perror("Error al reabrir /dev/buttons después de liberar");
    } else {
        printf("Reabierto correctamente /dev/buttons\n");
        close(fd1);
    }
}

void test_multiple_opens_closes() {
    printf("\n--- Test: múltiples aperturas y cierres consecutivos ---\n");
    for (int i = 0; i < 5; ++i) {
        int fd = open(DEV_LEDS, O_RDWR);
        if (fd < 0) {
            perror("Fallo al abrir /dev/leds");
            return;
        }
        printf("Iteración %d: abierto y cerrado /dev/leds\n", i + 1);
        close(fd);
    }
}

int main() {
    test_open_close_leds();
    test_open_close_speaker();
    test_buttons_exclusive_open();
    test_multiple_opens_closes();
    return 0;
}
