#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <time.h>

volatile sig_atomic_t exit_flag = 0;

void sigterm_handler(int signum) {
    printf("Received SIGTERM signal (%d)\n", signum);
    exit_flag = 1;
}

int main() {
    int uartdev;
    struct termios options;
    char buffer[1024];
    ssize_t bytesRead;
    char filename[40];
    time_t now;
    struct tm *now_tm;

    signal(SIGTERM, sigterm_handler);

    // Get current time
    now = time(NULL);
    now_tm = localtime(&now);

    char cwd[100];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }

    // Create filename with date and time
    strftime(filename, sizeof(filename), "data_time_%Y%m%d_%H%M%S.csv", now_tm);

    // Open the UART device (e.g., /dev/ttyACM0) for reading and writing
    // Adjust the device path according to your system
    uartdev = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);
    if (uartdev == -1) {
        perror("Unable to open UART device");
        return 1;
    }

    FILE *file = fopen(filename, "w+");
    if (file == NULL) {
        perror("Failed to open the file");
        return 1;
    }
    fprintf(file, "Module, \n");

    // Get the current options for the UART port
    tcgetattr(uartdev, &options);

    // Set the baud rates and other options
    cfsetispeed(&options, B115200);    // Input speed
    cfsetospeed(&options, B115200);    // Output speed
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // Set the new options for the UART port
    tcsetattr(uartdev, TCSANOW, &options);

    // Read data from UART and write to the CSV file
    while (!exit_flag) {
        bytesRead = read(uartdev, buffer, sizeof(buffer) - 1);
        if (bytesRead < 0) {
            printf("Failed to read from UART device");
        } else {
            buffer[bytesRead] = '\0';  // Null terminate the string
            printf("%s", buffer);
            fprintf(file, "%s\n", buffer);
            fflush(file);
        }
    }
    printf("Closing the file.");
    fclose(file);
    close(uartdev);
    return 0;
}
