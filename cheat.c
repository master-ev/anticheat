#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define HEALTH_ADDRESS 0x404048
#define AMMO_ADDRESS 0x40404c
#define ENEMY_X_ADDRESS 0x404050
#define ENEMY_VISIBLE_ADDRESS 0x404058

int read_int(int fd, long address) {
    int value = -1;
    if (lseek(fd, address, SEEK_SET) == -1)
        return -1;
    if (read(fd, &value, sizeof(value)) != sizeof(value))
        return -1;
    return value;
}

int write_int(int fd, long address, int value) {
    if (lseek(fd, address, SEEK_SET) == -1)
        return -1;
    if (write(fd, &value, sizeof(value)) != sizeof(value))
        return -1;
    return 0;
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf ("Usage: %s <target_pid> <command>\n", argv[0]);
        printf("Commands: read | godmode | freezeammo\n");
        return 1;
    }
    int pid = atoi (argv[1]);
    char * command = argv[2];
    char memory_path[64];
    snprintf (memory_path, sizeof(memory_path), "/proc/%d/mem", pid);
    int fd = open (memory_path, O_RDWR);
    if (fd == -1)
        return 1;
    if (strcmp(command, "read") == 0) {
        printf("health:  %d\n", read_int(fd, HEALTH_ADDRESS));
        printf("ammo:    %d\n", read_int(fd, AMMO_ADDRESS));
        printf("enemy_x: %d\n", read_int(fd, ENEMY_X_ADDRESS));
        printf("visible: %d\n", read_int(fd, ENEMY_VISIBLE_ADDRESS));
    } else if (strcmp(command, "godmode") == 0) {
        write_int(fd, HEALTH_ADDRESS, 9999);
        printf("Set health to 9999.\n");
    } else if (strcmp(command, "freezeammo") == 0) {
        printf("Freezing ammo at 999.\n");
        while (1) {
            write_int(fd, AMMO_ADDRESS, 999);
            usleep(10000);
        }
    } else {
        printf("Unknown command: %s\n", command);
    }
    close(fd);
    return 0;
}