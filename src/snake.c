#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SNAKE_MOVE_UP 'w'
#define SNAKE_MOVE_DOWN 's'
#define SNAKE_MOVE_LEFT 'a'
#define SNAKE_MOVE_RIGHT 'd'

#define LOW_BOUND_Y 20
#define UP_BOUND_Y 40
#define LOW_BOUND_X 70
#define UP_BOUND_X 80

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
void move_snake(SnakeNode* snake, int** board);
SnakeNode* add_snake_node(SnakeNode* snake, int** board);
void add_fruit(int** board);
void check_collision(SnakeNode* snake, int** board);

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

void game_loop(SnakeNode* snake, int** board) {
    SnakeNode* tail;
    GameState* game_state = malloc(sizeof(GameState));
    game_state->score = 0;
    int loop_counter = 0;
    while (1) {
        fflush(stdin);
        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
                 tm_info);

        get_user_input(snake);
        move_snake(snake, board);

        if (loop_counter % 50 == 0) {
            game_state->score += 100;
        }
        loop_counter++;

        refresh();
        sleep_ms(GAME_BASE_SPEED - game_state->score / 100);
    }

    // deallocate memory
    for (snake = snake->tail; snake != NULL;) {
        SnakeNode* temp = snake;
        snake = snake->next;
        free(temp);
    }

    for (int i = 0; i < 100; ++i) {
        free(board[i]);
    }

    free(game_state);

    free(board);
}

void init_game() {
    int** game_board = malloc(100 * sizeof(int*));

    for (int i = 0; i < 100; ++i) {
        game_board[i] = malloc(100 * sizeof(int));
        for (int j = 0; j < 100; ++j) {
            game_board[i][j] = EMPTY;
        }
    }

    char filename[] = "snake_game.log";
    fp = fopen(filename, "w+");

    box(stdscr, 0, 0);
    SnakeNode* snake = malloc(sizeof(SnakeNode));

    snake->y = 30;
    snake->x = 140;
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

    do {
        fruit_x = LOW_BOUND_X + rand() % (UP_BOUND_X - LOW_BOUND_X);
        fruit_y = LOW_BOUND_Y + rand() % (UP_BOUND_Y - LOW_BOUND_Y);
    } while (board[fruit_y][fruit_x] != EMPTY);

#ifdef DEBUG
    static int fruit_counter = 0;
    static const char function_name[] = "add_fruit";

    fprintf(fp, "[%s][%s] Fruit [#%d] init pos: y =  %d, x =  %d\n",
            time_buffer, function_name, ++fruit_counter, fruit_y, fruit_x);
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

void move_snake(SnakeNode* snake, int** board) {
    snake->prev_y = snake->y;
    snake->prev_x = snake->x;
    switch (snake->dir) {
        case UP:
            if (board[snake->y - 1][snake->x] == FRUIT) {
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y - 1][snake->x] == SNAKE_BODY) {
                // Collision with itself
                endwin();
                printf("Game Over! You collided with yourself.\n");
                exit(0);
            } else if (board[snake->y - 1][snake->x] == WALL) {
                // Collision with wall
                endwin();
                printf("Game Over! You hit a wall.\n");
                exit(0);
            }
            --(snake->y);
            snake->drawc = '^';
            break;
        case DOWN:
            if (board[snake->y + 1][snake->x] == FRUIT) {
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y + 1][snake->x] == SNAKE_BODY) {
                // Collision with itself
                endwin();
                printf("Game Over! You collided with yourself.\n");
                exit(0);
            } else if (board[snake->y + 1][snake->x] == WALL) {
                // Collision with wall
                endwin();
                printf("Game Over! You hit a wall.\n");
                exit(0);
            }
            ++(snake->y);
            snake->drawc = 'v';
            break;
        case LEFT:
            if (board[snake->y][snake->x - 1] == FRUIT) {
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y][snake->x - 1] == SNAKE_BODY) {
                // Collision with itself
                endwin();
                printf("Game Over! You collided with yourself.\n");
                exit(0);
            } else if (board[snake->y][snake->x - 1] == WALL) {
                // Collision with wall
                endwin();
                printf("Game Over! You hit a wall.\n");
                exit(0);
            }
            --(snake->x);
            snake->drawc = '<';
            break;
        case RIGHT:
            if (board[snake->y][snake->x + 1] == FRUIT) {
                add_fruit(board);
                snake->tail = add_snake_node(snake->tail, board);
            } else if (board[snake->y][snake->x + 1] == SNAKE_BODY) {
                // Collision with itself
                endwin();
                printf("Game Over! You collided with yourself.\n");
                exit(0);
            } else if (board[snake->y][snake->x + 1] == WALL) {
                // Collision with wall
                endwin();
                printf("Game Over! You hit a wall.\n");
                exit(0);
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
    }
}
