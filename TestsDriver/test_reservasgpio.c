#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define GPIO_BASE 512

// GPIOs usados en tu driver
int gpios[] = {
    9, 10, 11, 17, 22, 27, // LEDs
    2, 3,                 // Botones
    4                    // Speaker
};

#define NUM_GPIOS (sizeof(gpios) / sizeof(gpios[0]))

void export_gpio(int gpio) {
    char path[64];
    int fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo export");
        return;
    }

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", GPIO_BASE + gpio);
    if (write(fd, buffer, strlen(buffer)) < 0) {
        if (errno == EBUSY)
            printf("GPIO %d ya exportado\n", GPIO_BASE + gpio);
        else
            perror("Error al exportar GPIO");
    } else {
        printf("GPIO %d exportado\n", GPIO_BASE + gpio);
    }

    close(fd);
}

int main() {
    printf("Inicializando/reservando GPIOs manualmente antes del driver\n");

    for (int i = 0; i < NUM_GPIOS; ++i) {
        export_gpio(gpios[i]);
    }

    printf("\nAhora ejecuta: sudo insmod berryclip.ko\n");
    printf("El módulo debería fallar al intentar reservar los GPIOs ya exportados.\n");

    return 0;
}
