#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SNAKE_MOVE_UP 'w'
#define SNAKE_MOVE_DOWN 's'
#define SNAKE_MOVE_LEFT 'a'
#define SNAKE_MOVE_RIGHT 'd'

#define LOW_BOUND_Y 0
#define UP_BOUND_Y 50
#define LOW_BOUND_X 0
#define UP_BOUND_X 50

// Higher is slower
#define GAME_BASE_SPEED 100

// Logging to file
#define DEBUG

typedef struct SnakeNode SnakeNode;
typedef struct Board Board;
typedef struct GameState GameState;
typedef enum Direction Direction;
typedef enum EntityType EntityType;

FILE* fp;

struct tm* tm_info;
time_t timer;
char time_buffer[26];

char* get_line();
void get_user_input(SnakeNode* snake);
void move_snake(SnakeNode* snake, int** board, GameState* game_state);
SnakeNode* add_snake_node(SnakeNode* snake, int** board);
void add_fruit(int** board);
void check_collision(SnakeNode* snake, int** board);
void print_status(SnakeNode* snake, GameState* game_state);
int hard_mode = 1;  // Global variable to toggle hard mode
int quit;

enum Direction { UP, DOWN, LEFT, RIGHT };
enum EntityType { EMPTY, SNAKE_HEAD, SNAKE_BODY, FRUIT, WALL };

struct SnakeNode {
    int y;
    int x;
    int prev_y;
    int prev_x;
    char drawc;
    Direction dir;
    SnakeNode* next;
    SnakeNode* prev;
    SnakeNode* tail;
};

struct GameState {
    int score;
};

void sleep_ms(int ms) { usleep(ms * 1000); }

void update_time() {
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
}

void game_loop(SnakeNode* snake, int** board) {
    SnakeNode* tail;
    GameState* game_state = malloc(sizeof(GameState));
    game_state->score = 0;
    int loop_count = 0;
    while (!quit) {
        fflush(stdin);

        update_time();
        print_status(snake, game_state);
        get_user_input(snake);
        move_snake(snake, board, game_state);

        if (loop_count % 50 == 0 && hard_mode) {
            snake->tail = add_snake_node(snake->tail, board);
            add_fruit(board);
        }
        loop_count++;
        refresh();
        if (hard_mode) {
            // In hard mode, the snake moves faster as the score increases
            sleep_ms(GAME_BASE_SPEED - game_state->score / 100);
        } else {
            // Normal mode speed
            sleep_ms(GAME_BASE_SPEED);
        }
    }

    // deallocate memory
    for (snake = snake->tail; snake != NULL;) {
        SnakeNode* temp = snake;
        snake = snake->next;
        free(temp);
    }

    for (int i = 0; i < UP_BOUND_X; ++i) {
        free(board[i]);
    }

    free(board);

    fprintf(fp, "[%s][game_loop] Game Over! Final Score: %d\n", time_buffer,
            game_state->score);

    free(game_state);
}

void init_game() {
    int** game_board = malloc(UP_BOUND_Y * sizeof(int*));

    for (int i = 0; i < UP_BOUND_Y; ++i) {
        game_board[i] = malloc(UP_BOUND_X * sizeof(int));
        for (int j = 0; j < UP_BOUND_X; ++j) {
            if (i == LOW_BOUND_Y || i == UP_BOUND_Y - 1 || j == LOW_BOUND_X ||
                j == UP_BOUND_X - 1) {
                game_board[i][j] = WALL;  // Set walls
                mvaddch(i, j, '#');       // Draw walls
            } else {
                game_board[i][j] = EMPTY;
            }
        }
    }

    char filename[] = "snake_game.log";
    fp = fopen(filename, "w+");

    SnakeNode* snake = malloc(sizeof(SnakeNode));

    snake->y = 25;
    snake->x = 25;
    snake->prev_y = snake->y;
    snake->prev_x = snake->x;
    snake->drawc = '^';
    snake->dir = UP;
    snake->prev = NULL;
    snake->tail = snake;
    snake->next = NULL;

    game_board[snake->y][snake->x] = SNAKE_HEAD;
    add_fruit(game_board);

    game_loop(snake, game_board);
}

void add_fruit(int** board) {
    srand(timer);
    int fruit_x, fruit_y;
    int first = 0;

#ifdef DEBUG
    static int fruit_counter = 0;
#endif

    do {
#ifdef DEBUG
        if (first == 1) {
            fprintf(fp, "[%s][add_fruit] Re-adding fruit\n", time_buffer);
        }
#endif
        fruit_x = LOW_BOUND_X + rand() % (UP_BOUND_X - LOW_BOUND_X);
        fruit_y = LOW_BOUND_Y + rand() % (UP_BOUND_Y - LOW_BOUND_Y);
        first = 1;
    } while (board[fruit_y][fruit_x] != EMPTY);

#ifdef DEBUG
    fprintf(fp, "[%s][add_fruit] Fruit [#%02d] init pos: y =  %d, x =  %d\n",
            time_buffer, ++fruit_counter, fruit_y, fruit_x);
#endif

    board[fruit_y][fruit_x] = FRUIT;
    mvaddch(fruit_y, fruit_x, '*');
    refresh();
}

SnakeNode* add_snake_node(SnakeNode* node, int** board) {
    SnakeNode* new_node = malloc(sizeof(SnakeNode));
    new_node->y = node->prev_y;
    new_node->x = node->prev_x;
    new_node->next = NULL;
    new_node->prev = node;
    new_node->drawc = 'o';

    node->next = new_node;
    board[new_node->y][new_node->x] = SNAKE_BODY;
    return new_node;
}

void update_snake_nodes(SnakeNode* snake_head, int** board) {
    mvaddch(snake_head->prev_y, snake_head->prev_x, ' ');
    board[snake_head->prev_y][snake_head->prev_x] = EMPTY;

    // Clear old chars
    for (SnakeNode* ptr = snake_head->next; ptr != NULL; ptr = ptr->next) {
        mvaddch(ptr->y, ptr->x, ' ');
        board[ptr->y][ptr->x] = EMPTY;

        ptr->prev_y = ptr->y;
        ptr->prev_x = ptr->x;
        ptr->y = ptr->prev->prev_y;
        ptr->x = ptr->prev->prev_x;
    }

    // Add new chars
    mvaddch(snake_head->y, snake_head->x, snake_head->drawc);
    board[snake_head->y][snake_head->x] = SNAKE_HEAD;

    for (SnakeNode* ptr = snake_head->next; ptr != NULL; ptr = ptr->next) {
        mvaddch(ptr->y, ptr->x, ptr->drawc);
        board[ptr->y][ptr->x] = SNAKE_BODY;
    }
}

void print_status(SnakeNode* snake, GameState* game_state) {
    mvprintw(1, 55, "Score: %d", game_state->score);
    mvprintw(2, 55, "Snake Position: (%d, %d)", snake->y, snake->x);
    mvprintw(3, 55, "Direction: %c", snake->drawc);
}

// TODO: FIX THIS !!!
void move_snake(SnakeNode* snake, int** board, GameState* game_state) {
    snake->prev_y = snake->y;
    snake->prev_x = snake->x;
    switch (snake->dir) {
        case UP:
            if (board[snake->y - 1][snake->x] == FRUIT) {
                game_state->score += 100;
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y - 1][snake->x] == SNAKE_BODY) {
                // Collision with itself
                fprintf(fp,
                        "[%s][move_snake] Collision with itself at (%d, %d)\n",
                        time_buffer, snake->y, snake->x);
                quit = 1;
                return;
            } else if (board[snake->y - 1][snake->x] == WALL) {
                // Collision with wall
                snake->y += UP_BOUND_Y - 2;
            }
            --(snake->y);
            snake->drawc = '^';
            break;
        case DOWN:
            if (board[snake->y + 1][snake->x] == FRUIT) {
                game_state->score += 100;
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y + 1][snake->x] == SNAKE_BODY) {
                // Collision with itself
                fprintf(fp,
                        "[%s][move_snake] Collision with itself at (%d, %d)\n",
                        time_buffer, snake->y, snake->x);
                quit = 1;
                return;
            } else if (board[snake->y + 1][snake->x] == WALL) {
                // Collision with wall
                // Loop over other side of the wall
                snake->y -= UP_BOUND_Y - 2;
            }
            ++(snake->y);
            snake->drawc = 'v';
            break;
        case LEFT:
            if (board[snake->y][snake->x - 1] == FRUIT) {
                game_state->score += 100;
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y][snake->x - 1] == SNAKE_BODY) {
                // Collision with itself
                fprintf(fp,
                        "[%s][move_snake] Collision with itself at (%d, %d)\n",
                        time_buffer, snake->y, snake->x);
                quit = 1;
                return;
            } else if (board[snake->y][snake->x - 1] == WALL) {
                // Collision with wall
                snake->x += UP_BOUND_X - 2;
            }
            --(snake->x);
            snake->drawc = '<';
            break;
        case RIGHT:
            if (board[snake->y][snake->x + 1] == FRUIT) {
                game_state->score += 100;
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y][snake->x + 1] == SNAKE_BODY) {
                // Collision with itself
                fprintf(fp,
                        "[%s][move_snake] Collision with itself at (%d, %d)\n",
                        time_buffer, snake->y, snake->x);
                quit = 1;
                return;
            } else if (board[snake->y][snake->x + 1] == WALL) {
                // Collision with wall
                snake->x -= UP_BOUND_X - 2;
            }
            ++(snake->x);
            snake->drawc = '>';
            break;
        default:
            break;
    }

    update_snake_nodes(snake, board);
}

void get_user_input(SnakeNode* snake) {
    switch (getch()) {
        case SNAKE_MOVE_UP:
            if (snake->dir == DOWN) return;  // Prevent reversing direction
            snake->dir = UP;
            break;
        case SNAKE_MOVE_DOWN:
            if (snake->dir == UP) return;  // Prevent reversing direction
            snake->dir = DOWN;
            break;
        case SNAKE_MOVE_LEFT:
            if (snake->dir == RIGHT) return;  // Prevent reversing direction
            snake->dir = LEFT;
            break;
        case SNAKE_MOVE_RIGHT:
            if (snake->dir == LEFT) return;  // Prevent reversing direction
            snake->dir = RIGHT;
            break;
        case 'q':
            quit = 1;
    }
}
