#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define PIPE_NAME "/tmp/guess_pipe"
#define RESPONSE_PIPE_NAME "/tmp/response_pipe" // Новый канал для ответа

void play_game(int N, const char* player_role) {
    int fd, response_fd;
    int secret_number;
    int guess;
    int attempts = 0;
    time_t start_time, end_time;
    
    // Ожидаем получения канала
    if (strcmp(player_role, "player_1") == 0) {
        // Игрок 1 загадывает число
        secret_number = rand() % N + 1; // Загадать число от 1 до N
        
        // Открываем именованный канал для записи
        fd = open(PIPE_NAME, O_WRONLY);
        if (fd == -1) {
            perror("Failed to open pipe for writing");
            exit(1);
        }
        
        // Отправляем загаданное число в канал
        write(fd, &secret_number, sizeof(secret_number));
        close(fd);
        
        printf("[Player 1] Загадано число: %d\n", secret_number);
        printf("[Player 1] Ожидаю, что игрок 2 угадает...\n");

        // Открываем второй канал для получения ответа от Игрока 2
        response_fd = open(RESPONSE_PIPE_NAME, O_RDONLY);
        if (response_fd == -1) {
            perror("Failed to open response pipe for reading");
            exit(1);
        }

        // Ожидаем ответа от Игрока 2
        read(response_fd, &guess, sizeof(guess));
        printf("[Player 1] Игрок 2 угадал число: %d\n", guess);
        close(response_fd);

    } else if (strcmp(player_role, "player_2") == 0) {
        // Игрок 2 пытается угадать число
        
        // Открываем именованный канал для чтения
        fd = open(PIPE_NAME, O_RDONLY);
        if (fd == -1) {
            perror("Failed to open pipe for reading");
            exit(1);
        }

        // Получаем загаданное число от игрока 1
        read(fd, &secret_number, sizeof(secret_number));
        close(fd);
        
        // Начало угадывания
        printf("[Player 2] Получено число, начинаю угадывать...\n");
        attempts = 0;
        start_time = time(NULL);
        
        do {
            attempts++;
            guess = rand() % N + 1;
            printf("[Player 2] Попытка %d: %d\n", attempts, guess);
            
            // Открываем второй канал для отправки ответа
            response_fd = open(RESPONSE_PIPE_NAME, O_WRONLY);
            if (response_fd == -1) {
                perror("Failed to open response pipe for writing");
                exit(1);
            }

            // Отправляем догадку обратно
            write(response_fd, &guess, sizeof(guess));
            close(response_fd);

            if (guess == secret_number) {
                printf("[Player 2] Угадал число: %d за %d попыток\n", secret_number, attempts);
                break;
            }
        } while (1);
        
        end_time = time(NULL);
        printf("[Player 2] Время игры: %ld секунд\n", end_time - start_time);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]); // Получаем диапазон чисел N
    if (N <= 0) {
        fprintf(stderr, "Please provide a valid integer for N.\n");
        return 1;
    }

    // Создаем именованный канал (FIFO) для передачи числа
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo failed");
        return 1;
    }

    // Создаем второй канал (FIFO) для ответов
    if (mkfifo(RESPONSE_PIPE_NAME, 0666) == -1) {
        perror("mkfifo failed");
        return 1;
    }
    for (int i = 0; i < 10; i++) {  // Игры будут повторяться 10 раз
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            return 1;
        } else if (pid == 0) {
            // В дочернем процессе будет играть второй игрок
            play_game(N, "player_2");
            exit(0);
        } else {
            // В родительском процессе будет играть первый игрок
            play_game(N, "player_1");
            wait(NULL); // Ждем завершения второго процесса
        }
    }

    // Удаляем именованные каналы после завершения игры
    unlink(PIPE_NAME);
    unlink(RESPONSE_PIPE_NAME);
    
    return 0;
}
