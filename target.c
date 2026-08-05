#include <stdio.h>
#include <unistd.h>

// values that a cheater would want to attack
int health = 100;
int ammo = 30;
int enemy_x = 50;
int enemy_visible = 0;

int main() {
    printf("Target running. PID: %d\n", getpid());
    printf("Watch the values.\n\n");
    int tick = 0;
    while (1) {
        enemy_x = (enemy_x + 1) % 100;
        enemy_visible = (tick % 5 == 0);
        if (ammo > 0)
            ammo = ammo - 1;
        printf("tick %d | health: %d | ammo: %d | enemy_x: %d | visible: %d\n", tick, health, ammo, enemy_x, enemy_visible);
        tick = tick + 1;
        sleep(1);
    }
    return 0;
}
