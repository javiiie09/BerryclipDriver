#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(){

    int fd = open("/dev/leds", O_RDONLY);
    if(fd < 0){
	return 1;
    }

    for(int i = 0; i < 3; ++i){
	uint8_t val;
	ssize_t bytes = read(fd, &val, 1);
	if(bytes < 0){
	    perror("Error al leer\n");
	}else if(bytes == 0){
	    printf("EOF alcanzado\n");
	}else{
	    printf("Lectura %d: 0x%02X\n", i+1, val);
	}
	int ret = lseek(fd, 0, SEEK_SET);
	if(ret < 0){
	    perror("lseek");
	}
    }
    close(fd);
    return 0;
}
