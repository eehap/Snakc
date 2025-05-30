# Terminal ASCII Snake game in C. 

Random hobby project, still wip.

Doubly Linked list + 2D array implementation using Ncurses. Not tested on Windows systems.

# Dependencies

Ncurses

# Compilation

With gcc:
`$ gcc -o snake src/*.c -lncurses`

With make:
`$ make`

# Running

`$ ./snake`

# Keybinds

Movement using wasd-keys. To quit while playing press 'q'.

# Gameplay

Try to eat as many fruit '*' as possible while avoiding your own tail. 

Toggle hard mode from inside `src/snake.c` using the global variable `int hard_mode`.

Hard mode differences include automatic addition of snake size and increased spawns of fruit.
When score increases the game gets faster.

# Issues

Random seg faults may happen, though personally don't seem to experience them much.

# Future work

- Find out if decreasing Ncurses-window row padding is possible, to make the play field visually symmetric.
- Move program to center of terminal screen
- Menu
- Settings config
- Auto adjusted size based on terminal size
- Gameplay changes?

