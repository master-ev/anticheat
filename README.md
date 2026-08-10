# Anticheat

Three programs that fight each other. A little CS-style game, a cheat that breaks into its memory from outside, and the anticheat built into the game trying to catch it. All local - I wanted to understand memory cheats by writing both the attack and the defense. The game holds the things a cheater cares about: health, ammo, where the enemy is, whether it's visible. The cheat reaches in through `/proc/<pid>/mem` and messes with them. The game watches for exactly that.

## The attacks, and what stops them

- Reading memory - the cheat scans the game's RAM for a value. The game hides its real values behind disguised copies, so a plain scan finds nothing useful.
- Godmode / infinite ammo - the cheat writes health to 9999 or freezes ammo. The game keeps a shadow of each value and repairs anything that doesn't match.
- Aimbot / wallhack - reads the enemy's position, forces "visible" on. Same memory checks catch it.
- Debugger - the cheat attaches with `ptrace` to poke around. The game traces itself first, which slams the door: nobody else can attach.
- Injection - a library loaded with `LD_PRELOAD` runs code inside the game. The game checks its own loaded libraries against a known-good list.
- Patching the code - the cheat overwrites the anticheat's own instructions with NOPs. The game checksums its code and rewrites it from a clean copy.

Every detection goes through one place that counts them and reacts in steps - a few warnings, then a kick, like a real anticheat banning after it's sure.

## Files

```
target.c - the game and all the defenses
cheat.c - the cheat: read, godmode, freezeammo, wallhack, aimbot, patch
evil.c - the injected library (build as evil.so)
flash_debugger.c - a debugger that attaches and leaves fast, to test the anti-debug
```

## Run

```bash
gcc -no-pie -o target target.c
gcc -o cheat cheat.c

./target # it prints its PID
./cheat <pid> godmode # another terminal
```

Watch the game notice, repair the value, and kick after enough tries.

## Notes

None of these defenses holds alone — I could break each one with a little more effort. So the last part was closing the gaps: catch the debugger as it attaches, trust libraries by a known list, let the game repair its own code when it's patched. Security here is a back-and-forth, not a checklist. One thing that kept getting me: recompiling the game moves every address, so the cheat breaks. Which is exactly why real cheats die on patch day.