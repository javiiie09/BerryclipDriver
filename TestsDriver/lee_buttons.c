#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEVICE "/dev/buttons"

int main(int argc, char **argv)
{
    FILE  *f;
	
    if (argc > 1) fork();
	
    printf("Abriendo fichero %s por pid %d\n", DEVICE, getpid());
    
    while ((f = fopen(DEVICE, "r")) == NULL) {
        printf("ERROR en pid %d\n", getpid());
        perror("No pudo abrir el fichero, reintento en 5 segundos");
        sleep(5);
    }
    setvbuf(f, NULL, _IONBF, 0);
        
    for (int i = 0; i < 5; i++) {
        char c;
        int n = fscanf(f, "%c", &c);
	if (EOF == n) {
            if (ferror(f)) perror("fscanf");
            else printf("EOF ????\n");
            break;
        }
        printf("Leida pulsación #%d = '%c' por pid %d\n", i, c ,getpid());
    }
    
    printf("Cerrando fichero por pid %d\n", getpid());
    fclose(f);
    exit(EXIT_SUCCESS);
}

