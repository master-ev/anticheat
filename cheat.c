#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define HEALTH_ADDRESS 0x404048
#define AMMO_ADDRESS 0x40404c
#define ENEMY_X_ADDRESS 0x404050
#define ENEMY_VISIBLE_ADDRESS 0x404058

int read_int(int fd, long address) {
    int value = -1;
    if (lseek(fd, address, SEEK_SET) == -1) {
        return -1;
    }
    if (read(fd, &value, sizeof(value)) != sizeof(value)) {
        return -1;
    }
    return value;
}

int main (int argc, char *argv[]) {
    if (argc < 2) {
        printf ("Usage: %s <target_pid>\n", argv[0]);
        return 1;
    }
    int pid = atoi (argv[1]);
    char memory_path[64];
    snprintf (memory_path, sizeof(memory_path), "/proc/%d/mem", pid);
    int fd = open (memory_path, O_RDONLY);
    if (fd == -1) {
        return 1;
    }
    printf("Reading target %d:\n", pid);
    printf(" health: %d\n", read_int(fd, HEALTH_ADDRESS));
    printf(" ammo: %d\n", read_int(fd ,AMMO_ADDRESS));
    printf(" enemy_x: %d\n", read_int(fd, ENEMY_X_ADDRESS));
    printf(" visible: %d\n", read_int(fd, ENEMY_VISIBLE_ADDRESS));
    close(fd);
    return 0;
}