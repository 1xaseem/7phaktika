#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

volatile sig_atomic_t guessed = 0;
volatile sig_atomic_t target = 0;

void signal_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGUSR1) {
        guessed = 1; 
    } else if (sig == SIGUSR2) {
        guessed = 0; 
    } else if (sig >= SIGRTMIN && sig <= SIGRTMAX) {
        target = info->si_value.sival_int; 
    }
}

void play_as_guesser(pid_t other_pid, int range) {
    int guess;
    union sigval value;

    srand(getpid());

    while (1) {
        guess = rand() % range + 1;
        value.sival_int = guess;

        printf("Игрок-угадыватель: Предполагаю число %d\n", guess);
        sigqueue(other_pid, SIGRTMIN, value);

        pause();

        if (guessed) {
            printf("Игрок-угадыватель: Угадал число %d!\n", guess);
            break;
        } else {
            printf("Игрок-угадыватель: Число %d неверное, пробую снова.\n", guess);
        }
    }
}

void play_as_host(pid_t other_pid, int range) {
    srand(getpid());
    int target_number = rand() % range + 1;
    int received_guess;

    printf("Игрок-загадчик: Загадал число %d\n", target_number);

    while (1) {
        pause();

        received_guess = target;
        printf("Игрок-загадчик: Получено число %d\n", received_guess);

        if (received_guess == target_number) {
            printf("Игрок-загадчик: Число %d угадано!\n", received_guess);
            kill(other_pid, SIGUSR1); 
            break;
        } else {
            kill(other_pid, SIGUSR2); 
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <диапазон>\n", argv[0]);
        return 1;
    }

    int range = atoi(argv[1]);
    if (range <= 0) {
        fprintf(stderr, "Диапазон должен быть положительным числом.\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGRTMIN, &sa, NULL);

    pid_t pid = fork();

    if (pid < 0) {
        perror("Ошибка вызова fork");
        return 1;
    } else if (pid == 0) {
        play_as_guesser(getppid(), range);
    } else {
        play_as_host(pid, range);
    }

    return 0;
}
