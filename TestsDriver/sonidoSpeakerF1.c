/*************************************************
 * Prueba de sonido con /dev/speaker (Tetris)
 * 
 * Compilar:
 *   gcc sonidoTetris.c -o sonidoTetris -lrt -lpthread
 *
 * Uso:
 *   sudo ./sonidoTetris [BPM]
 *     BPM: pulsos por minuto (por defecto 120)
 *************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define DEVICE      "/dev/speaker"
#define LOW_LEVEL   '0'
#define HIGH_LEVEL  '1'
#define ESC         27  // Código ASCII de Escape

int fd_speaker;

// ─── Notas definidas por octava ────────────────────────────────────
// Octava 3
#define C3   130
#define CS3  138
#define D3   146
#define DS3  155
#define E3   164
#define F3   174
#define FS3  185
#define G3   196
#define GS3  207
#define A3   220
#define AS3  233
#define B3   246
// Octava 4
#define C4   261
#define CS4  277
#define D4   294
#define DS4  311
#define E4   329
#define F4   349
#define FS4  370
#define G4   392
#define GS4  415
#define A4   440
#define AS4  466
#define B4   493
// Octava 5
#define C5   523
#define CS5  554
#define D5   587
#define DS5  622
#define E5   659
#define F5   698
#define FS5  740
#define G5   784
#define GS5  830
#define A5   880
#define AS5  932
#define B5   987
// Octava 6
#define C6   1046
#define CS6  1108
#define D6   1175
#define DS6  1245
#define E6   1318
#define F6   1397
#define FS6  1480
#define G6   1568
#define GS6  1661
#define A6   1760
#define AS6  1864
#define B6   1976

// Duraciones en milisegundos (variables según tempo)
int blanca;
int negra;
int corch;     // corchea (la nota indivisible original)
int semicor;   // mitad de corchea
int trescor;   // tercera parte de negra

void flanco_altavoz()
{
    static char state = 0;
    char c = (state = !state) ? HIGH_LEVEL : LOW_LEVEL;
    write(fd_speaker, &c, 1);
}

#define TIME_ADD(ts1, ts2)                  \
    do {                                    \
        ts1.tv_sec  += ts2.tv_sec;          \
        ts1.tv_nsec += ts2.tv_nsec;         \
        if (ts1.tv_nsec >= 1000000000) {    \
            ts1.tv_nsec -= 1000000000;      \
            ts1.tv_sec += 1;                \
        }                                   \
    } while(0)

static inline int TIME_CMP(struct timespec a, struct timespec b)
{
    return (a.tv_sec > b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_nsec > b.tv_nsec));
}

static inline struct timespec TIME_MIN(struct timespec a, struct timespec b)
{
    if (a.tv_sec < b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec))
        return a;
    return b;
}

static void delay_ms(long ms)
{
    struct timespec t = { ms/1000, (ms%1000)*1000000 };
    clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
}

// frecuencia en Hz, duración en ms
void toneManual(int frequency, int duration)
{
    double dperiod = 1.0/frequency;
    long iperiod = dperiod;
    long fperiod = (dperiod-iperiod)*1000000000;
    struct timespec period = {iperiod, fperiod};
    struct timespec length = {0, duration*1000000};
    struct timespec next, end, min;
    clock_gettime(CLOCK_MONOTONIC, &next);
    end = next;
    TIME_ADD(end, length);
    TIME_ADD(next, period);
    next = TIME_MIN(end, next);
    do {
        flanco_altavoz();
        TIME_ADD(next, period);
        min = TIME_MIN(end, next);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &min, NULL);
    } while (TIME_CMP(end, next));
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &end, NULL);
}

void Beep(int note, int duration)
{
    toneManual(note, duration);
    delay_ms(20);
}

// ─── Canciones ────────────────────────────────────────────────────
void tetris()
{
    Beep(E6,   blanca);  Beep(B5,   negra);   Beep(C6,   negra);   Beep(D6,   negra);
    Beep(E6,   corch);   Beep(D6,   corch);   Beep(C6,   negra);   Beep(B5,   negra);
    Beep(A5,   blanca);  Beep(A5,   negra);   Beep(C6,   negra);   Beep(E6,   blanca);
    Beep(D6,   negra);   Beep(C6,   negra);   Beep(B5,   750);     Beep(C6,   negra);
    Beep(D6,   blanca);  Beep(E6,   blanca);  Beep(C6,   blanca);  Beep(A5,   blanca);
    Beep(A5,   blanca);  Beep(D6,   blanca);  Beep(F6,   negra);   Beep(A6,   blanca);
    Beep(G6,   negra);   Beep(F6,   negra);   Beep(E6,   750);     Beep(C6,   negra);
    Beep(E6,   blanca);  Beep(D6,   negra);   Beep(C6,   negra);   Beep(B5,   blanca);
    Beep(B5,   negra);   Beep(C6,   negra);   Beep(D6,   blanca);  Beep(E6,   blanca);
    Beep(C6,   blanca);  Beep(A5,   blanca);  Beep(A5,   blanca);
}

void starWars1()
{
    Beep(C4,   blanca);  Beep(G4,   negra);  Beep(F4,   trescor);  Beep(D4,   trescor);  Beep(E4,   trescor);
    Beep(C5,   blanca);  Beep(G4,   negra);  Beep(F4,   trescor);  Beep(D4,   trescor);  Beep(E4,   trescor);
    Beep(C5,   blanca);  Beep(G4,   negra);  Beep(F4,   trescor);  Beep(D4,   trescor);  Beep(F4,   trescor);
    Beep(D4,   blanca);  Beep(D4,   negra);  Beep(G3,   negra);
    Beep(C4,   blanca);  Beep(G4,   negra);  Beep(F4,   trescor);  Beep(D4,   trescor);  Beep(E4,   trescor);
    Beep(C5,   blanca);  Beep(G4,   negra);  Beep(F4,   trescor);  Beep(D4,   trescor);  Beep(E4,   trescor);
    Beep(C5,   blanca);  Beep(G4,   negra);  Beep(F4,   trescor);  Beep(D4,   trescor);  Beep(F4,   trescor);
    Beep(D4,   blanca);  Beep(D4,   negra);  Beep(G3,   negra);
}

int main(int argc, char *argv[])
{
    int tempo = 120;
    if (argc > 1) {
        int t = atoi(argv[1]);
        tempo = (t > 0) ? t : 120;
    }
    const float K = 60000.0f;
    negra     = (int)(K / tempo);
    blanca    = (int)(2 * K / tempo);
    corch     = (int)(K / (2 * tempo));
    semicor   = corch / 2;
    trescor   = negra / 3;

    printf("\nRaspberry Pi - selector de canciones en %s\n", DEVICE);
    printf("======================================\n");
    printf("Tempo: %d BPM -> negra=%d ms, blanca=%d ms, corchea=%d ms, semicor=%d ms, trescor=%d ms\n\n",
           tempo, negra, blanca, corch, semicor, trescor);

    if (argc > 1) {
        struct sched_param sp = { .sched_priority = 85 };
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
    }

    int salir = 0;
    while (salir != ESC) {
        printf("\nOpciones (ESC=%d para salir):\n", ESC);
        printf("  1) Tetris\n");
        printf("  2) Star Wars 1\n");
        printf("Selecciona canción: ");
        int opcion;
        if (scanf("%d", &opcion) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            opcion = 1;
        }
        if (opcion == ESC) {
            salir = ESC;
            break;
        }
        fd_speaker = open(DEVICE, O_WRONLY);
        if (fd_speaker < 0) {
            perror("ERROR al abrir el device");
            return EXIT_FAILURE;
        }
        if (opcion == 2)
            starWars1();
        else
            tetris();
        close(fd_speaker);
    }
    printf("Saliendo...\n");
    return 0;
}
