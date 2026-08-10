#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/ptrace.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define CHEAT_MEMORY 0
#define CHEAT_DEBUGGER 1
#define CHEAT_INJECTION 2
#define CHEAT_CODE 3
#define CHEAT_TYPES 4

int detection_counts[CHEAT_TYPES] = {0, 0, 0, 0};
int total_detections = 0;
#define KICK_THRESHOLD 10

const char *cheat_names[CHEAT_TYPES] = {"memory tampering", "debugger", "code injection", "code patching",};

#define SEAL_SIZE ((unsigned char *)unseal - (unsigned char *)seal)
#define SEAL_CAPACITY 64

unsigned int code_checksum_reference;
unsigned char seal_original[SEAL_CAPACITY];

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

const char *allowed_libs[] = {"libc.so", "libc-", "ld-linux", "ld.so", "libm.so", "libdl.so", "libpthread",};
int allowed_count = 7;

int is_allowed_lib(const char *line) {
    for (int i = 0; i < allowed_count; i++) {
        if (strstr(line, allowed_libs[i]) != NULL)
            return 1;
    }
    return 0;
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
        if (!is_allowed_lib(line)) {
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

void report(int cheat_type) {
    detection_counts[cheat_type]++;
    total_detections++;
    const char *name = cheat_names[cheat_type];
    if (total_detections < KICK_THRESHOLD)
        printf(">>> WARNING: %s detected (%d total) <<<\n", name, total_detections);
    else
        printf(">>> KICK: too many detections (%d) <<<\n", total_detections);
    log_detection(name);
    if (total_detections >= KICK_THRESHOLD) {
        printf("Player kicked after %d detections. Summary:\n", total_detections);
        for (int i = 0; i < CHEAT_TYPES; i++)
            printf(" %s: %d\n", cheat_names[i], detection_counts[i]);
        exit(1);
    }
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

void repair_code(void) {
    void *address = (void *)seal;
    unsigned long seal_size = (unsigned char *)unseal - (unsigned char *)seal;
    long page_size = sysconf(_SC_PAGESIZE);
    void *page = (void *)((long)address & ~(page_size - 1));
    if (mprotect(page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == -1)
        return;
    // rewrite the original bytes
    memcpy(address, seal_original, seal_size);
    mprotect(page, page_size, PROT_READ | PROT_EXEC);
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
    if (debugger_via_ptrace())
        report(CHEAT_DEBUGGER);
    code_checksum_reference = checksum((unsigned char *)seal, (unsigned char *)unseal);
    unsigned long seal_size = (unsigned char *)unseal - (unsigned char *)seal;
    memcpy(seal_original, (unsigned char *)seal, seal_size);
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
            report(CHEAT_MEMORY);
            health = unseal(health_shadow);
        }
        if (is_tampered(ammo, ammo_shadow))
            report(CHEAT_MEMORY);
        // if (debugger_via_status()) {
        //     printf(">>> DEBUGGER DETECTED: someone is tracing! <<<\n");
        //     log_detection("debugger attached (TracerPid)");
        // }
        if (injection_detected())
            report(CHEAT_INJECTION);
        unsigned int current = checksum((unsigned char *)seal, (unsigned char *)unseal);
        if (current != code_checksum_reference) {
            report(CHEAT_CODE);
            repair_code();
        }
        printf("tick %d | health: %d | ammo: %d | enemy_x: %d | visible: %d | aim: %d | hit: %d\n", tick, health, ammo, enemy_x, enemy_visible, player_aim, hit);
        tick = tick + 1;
        sleep(1);
    }
    return 0;
}
