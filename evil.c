// an injected library that tampers with the game
#include <stdio.h>

__attribute__((constructor))
void on_load(void) {
    printf("[evil] Injected library loaded!\n");
}