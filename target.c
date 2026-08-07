#include <stdio.h>
#include <unistd.h>
#include <time.h>

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
    while (1) {
        enemy_x = (enemy_x + 1) % 100;
        enemy_visible = (tick % 5 == 0);
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
        printf("tick %d | health: %d | ammo: %d | enemy_x: %d | visible: %d | aim: %d | hit: %d\n", tick, health, ammo, enemy_x, enemy_visible, player_aim, hit);
        tick = tick + 1;
        sleep(1);
    }
    return 0;
}
