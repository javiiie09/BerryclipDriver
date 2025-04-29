#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main() {
    int fd1, fd2;

    // Primer open: modo bloqueante (ocupa el dispositivo)
    fd1 = open("/dev/buttons", O_RDONLY);
    if (fd1 < 0) {
        perror("Error al abrir /dev/buttons (1)");
        return 1;
    }
    printf("Primer open (bloqueante) exitoso: fd1 = %d\n", fd1);

    // Segundo open: modo no bloqueante (debe fallar con -EBUSY)
    fd2 = open("/dev/buttons", O_RDONLY | O_NONBLOCK);
    if (fd2 < 0) {
        printf("Segundo open (no bloqueante) falló como se esperaba:\n");
        printf("errno = %d (%s)\n", errno, strerror(errno));
    } else {
        printf("Segundo open tuvo éxito inesperadamente: fd2 = %d\n", fd2);
        close(fd2);
    }

    // Liberamos el recurso
    close(fd1);

    return 0;
}
