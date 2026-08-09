#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/ptrace.h>
#include <string.h>
#include <stdlib.h>

unsigned int code_checksum_reference;

int debugger_via_ptrace(void) {
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
        return 1; // traced
    ptrace(PTRACE_DETACH, 0, 0, 0);
    return 0;
}

int debugger_via_status(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (f == NULL)
        return 0;
    char line[256];
    int tracer_pid = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "TracerPid:", 10) == 0) {
            tracer_pid = atoi(line + 10);
            break;
        }
    }
    fclose(f);
    return tracer_pid != 0;
}

int injection_detected(void) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (f == NULL)
        return 0;
    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strstr(line, ".so") == NULL)
            continue;
        int is_system = strstr(line, "/usr/") != NULL || strstr(line, "/lib/") != NULL;
        if (!is_system) {
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

void log_detection(const char *what) {
    FILE *f = fopen("anticheat.log", "a");
    if (f == NULL)
        return;
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    fprintf(f, "[%.24s] DETECTED: %s\n", timestamp, what);
    fclose(f);
}

#define SHADOW_KEY 0xA5A5A5A5
unsigned int health_shadow;
unsigned int ammo_shadow;

unsigned int seal(int value) {
    return (unsigned int)value ^ SHADOW_KEY;
}

int unseal(unsigned int shadow) {
    return (int)(shadow ^ SHADOW_KEY);
}

int is_tampered(int value, unsigned int shadow) {
    return seal(value) != shadow;
}

unsigned int checksum(unsigned char *start, unsigned char *end) {
    unsigned int sum = 0;
    for (unsigned char *p = start; p < end; p++) {
        sum = sum + *p;
        sum = (sum << 1) | (sum >> 31);
    }
    return sum;
}

// values that a cheater would want to attack
int health = 100;
int ammo = 30;
int enemy_x = 50;
int enemy_visible = 0;
int player_aim = 0;
int hit = 0;

int main() {
    printf("Target running. PID: %d\n", getpid());
    printf("Watch the values.\n\n");
    int tick = 0;
    health_shadow = seal(health);
    ammo_shadow = seal(ammo);
    if (debugger_via_ptrace()) {
          printf(">>> DEBUGGER DETECTED at startup! <<<\n");
          log_detection("debugger attached (ptrace)");
    }
    code_checksum_reference = checksum((unsigned char *)seal, (unsigned char *)unseal);
    struct timespec last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);
    while (1) {
        enemy_x = (enemy_x + 1) % 100;
        enemy_visible = (tick % 5 == 0);
        struct timespec now_time;
        clock_gettime(CLOCK_MONOTONIC, &now_time);
        double elapsed = (now_time.tv_sec - last_time.tv_sec) + (now_time.tv_nsec - last_time.tv_nsec) / 1e9;
        last_time = now_time;
        if (tick > 0 && elapsed < 0.5) {
            printf(">>> SPEDHACK DETECTED: tick came too fast (%.2fs) <<<\n", elapsed);
            log_detection("speedhack");
        }
        if (ammo > 0) {
            ammo = ammo - 1;
            ammo_shadow = seal(ammo);
        }
        hit = (player_aim == enemy_x);
        if (is_tampered(health, health_shadow)) {
            printf(">>> CHEAT DETECTED: health was modified! Restoring. <<<\n");
            log_detection("health modified externally");
            health = unseal(health_shadow);
        }
        if (is_tampered(ammo, ammo_shadow))
            printf(">>> CHEAT DETECTED: ammo was modified! <<<\n");
        // if (debugger_via_status()) {
        //     printf(">>> DEBUGGER DETECTED: someone is tracing! <<<\n");
        //     log_detection("debugger attached (TracerPid)");
        // }
        if (injection_detected()) {
            printf(">>> INJECTION DETECTED: an unknown library is loaded! <<<\n");
            log_detection("injected library in memory maps");
        }
        unsigned int current = checksum((unsigned char *)seal, (unsigned char *)unseal);
        if (current != code_checksum_reference) {
            printf(">>> CODE TAMPERING DETECTED: seal() was modified! <<<\n");
            log_detection("code section modified");
        }
        printf("tick %d | health: %d | ammo: %d | enemy_x: %d | visible: %d | aim: %d | hit: %d\n", tick, health, ammo, enemy_x, enemy_visible, player_aim, hit);
        tick = tick + 1;
        sleep(1);
    }
    return 0;
}
