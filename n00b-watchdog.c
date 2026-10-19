#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/select.h>
#include <linux/input.h>

#define DEV_RESET "/dev/input/event3"
#define DEV_POWER "/dev/input/event2"

#define CODE_RESET 115
#define CODE_POWER 411

int main(int argc, char **argv) {
    int opt;
    int target_pid = 0;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                target_pid = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -p <pid>\n", argv[0]);
                return 1;
        }
    }

    if (target_pid == 0) {
        fprintf(stderr, "Usage: %s -p <pid>\n", argv[0]);
        return 1;
    }

    int fd_reset = open(DEV_RESET, O_RDONLY | O_NONBLOCK);
    int fd_power = open(DEV_POWER, O_RDONLY | O_NONBLOCK);
    if (fd_reset < 0 || fd_power < 0) {
        perror("open");
        return 1;
    }

    fd_set fds;
    int maxfd = (fd_reset > fd_power) ? fd_reset : fd_power;

    while (kill(target_pid, 0) == 0) {
        
        FD_ZERO(&fds);
        FD_SET(fd_reset, &fds);
        FD_SET(fd_power, &fds);

        if (select(maxfd + 1, &fds, NULL, NULL, NULL) < 0)
            break;

        struct input_event ev;

        if (FD_ISSET(fd_power, &fds)) {
            while (read(fd_power, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY && ev.code == CODE_POWER && ev.value == 1) {
                    kill(target_pid, SIGTERM);
                    sync();
                    system("poweroff");
                }
            }
        }

        if (FD_ISSET(fd_reset, &fds)) {
            while (read(fd_reset, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY && ev.code == CODE_RESET && ev.value == 1) {
                    kill(target_pid, SIGTERM);
                    break;
                }
            }
        }
    }

    close(fd_reset);
    close(fd_power);
    return 0;
}