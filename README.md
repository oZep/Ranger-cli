## Ranger-cli
Real-Time Multiplayer Game in your terminal (not round based)

### CLI Shooter Duel (1v1)

A real-time, turn-based duel game played directly from the command line, built using Python's socket library for network communication.

This project simulates a 1-on-1 gunfight where players choose their actions: Shoot, Duck, or Reload. The server acts as the game authority, processing both players' moves at the end of each turn and broadcasting the resulting game state back to the clients.

```
   Game Rules:
   
   Health: 3 HP (First to 0 HP loses).
   
   Bullets: Start with 2 bullets (Max 3).
   
   Shoot: Costs 1 bullet. Deals 1 damage unless the opponent chose Duck for the same turn.
   
   Duck: Consumes no bullets. Prevents 1 incoming bullet damage for that turn.
   
   Reload: Adds 2 bullets (up to the max of 6).
```

Created using: C++ TCP Socket API, Real-time communication, multithreading, and efficient data handling.
