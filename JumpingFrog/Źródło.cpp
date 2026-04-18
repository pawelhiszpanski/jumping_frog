#define _CRT_SECURE_NO_WARNINGS
#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_VELOCITY 2                  // Maksymalna predkosc
#define BIRD_WAIT_TIME 10               // Czas po jakim bocian pojawia sie na 3 poziomie
#define BIRD_MOVE_PER_FRAMES 15         // Ilosc klatek gry po ktorych bocian sie porusza
#define FROG_MOVE_DELAY 100              // Czas opoznienia w milisekundach (100)
#define LINE_HEIGHT 3                   // Wysokosc pasa ruchu
#define GAP 2                           // Przerwa miedzy pasami
#define STOP_DISTANCE 3                 // Dystans w obszarze ktorego zatrzymuje sie samochod
#define HEIGHT 25                       // Wysokosc panelu gry

typedef struct {
    int y;                              // Pozycja y samochodu
    int x;                              // Pozycja x samochodu
    int velocity;                       // Predkosc samochodu
    int direction;                      // 1 dla poruszania w prawo; -1 dla poruszania w lewo
    int is_friendly;                    // Status samochodu "wrogi"/"przyjazny"
    int interacted_with_frog;           // Status czy samochod przeprowadzil interakcje z zaba
} Car;

typedef struct {
    int x, y;                           // Pozycja x; y (dla celu, zaby, bociana oraz przeszkod)
} Position;

typedef struct {
    char frog_shape;                    // Ksztalt zaby
    char obstacle_shape;                // Ksztalt przeszkody
    char car_shape;                     // Ksztalt samochodu
    char bird_shape;                    // Ksztalt bociana
    char goal_shape;                    // Ksztalt celu
    int car_count;                      // Ilosc samochodow
    int obstacle_count;                 // Ilosc przeszkod
    int width;                          // Szerokosc ekranu gry
} GameConfig;

void init_game(Position* frog, Position* goal, GameConfig* config, Car* cars, int* level);
void draw(clock_t* start_time, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level);
int move_frog(int direction, clock_t* last_frog_move_time, Position* frog, GameConfig* config, Position* obstatcles);
void move_cars(Position* frog, GameConfig* config, Car* cars, int* level, int* frames_counter);
void move_stork(Position* frog, Position* stork);
void check_collisions(clock_t* start_time, int* score, int* game_over, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level);
void display_message(const char* message, GameConfig* config);
void game_loop(int* game_over, clock_t* start_time, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level);
void load_config(const char* filename, GameConfig* config);
void draw_road_lanes(GameConfig* config);
void move_bird_towards_frog(Position* frog, Position* stork);
void draw_border(GameConfig* config);
void draw_time(clock_t* start_time, GameConfig* config);
void generate_obstacles(Position* obstacles, Position* goal, GameConfig* config);
void draw_obstacles(Position* obstacles, GameConfig* config);
int collision_with_obstacle(int x, int y, GameConfig* config, Position* obstacles);
void init_colors();
int distance_sq(int x, int y, int a, int b);
int* road_lanes(int* lane_count);
int checking_hit(Position* obstacles, Position* frog, GameConfig* config);
void draw_status(int* jumps, Position* frog, GameConfig* config, int* level);
void points(clock_t* start_time, int* score, GameConfig* config, int* jumps, int* level, int* game_over);
int is_lane_occupied(int lane_y, GameConfig* config, Car* cars);
void frog_on_goal(clock_t* start_time, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level, int* game_over);
void checking_car_route(int* game_over, clock_t* start_time, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level, int* score);
void read_char(int* game_over, int* ch, int* jumps, clock_t* last_frog_move_time, Position* frog, GameConfig* config, Position* obstacles);
void friendly_chooser(Car* cars, int* level, GameConfig* config);

int main() {
    srand(time(NULL));
    initscr();
    noecho();
    timeout(0);
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    GameConfig config;
    clock_t start_time = clock();
    load_config("frogger.txt", &config);
    int game_over = 0;
    int level = 1;
    Position frog;
    Position goal;
    Position stork;
    Position* obstacles;
    Car* cars;
    cars = (Car*)malloc(config.car_count * sizeof(Car));                        // Zadeklarowanie pamieci dla tablicy samochodow
    if (!cars) {
        endwin();
        fprintf(stderr, "Failed to allocate memory for cars");
        exit(1);
    }
    obstacles = (Position*)malloc(config.obstacle_count * sizeof(Position));    // Zadeklarowanie pamieci dla przeszkod
    if (!obstacles) {
        endwin();
        fprintf(stderr, "Failed to allocate memory for cars");
        exit(1);
    }
    init_game(&frog, &goal, &config, cars, &level);
    game_loop(&game_over, &start_time, &frog, &goal, &stork, &config, cars, obstacles, &level);
    free(cars);
    free(obstacles);
    endwin();
    return 0;
}

void load_config(const char* filename, GameConfig* config) {                    // Wczytanie parametrow gry z pliku
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Your file don't exist");
        exit(1);
    }
    else
    {
        char line[20];
        while (fgets(line, sizeof(line), file)) {
            int len = strlen(line);
            if (line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            if (strncmp(line, "frog_shape=", 11) == 0) {
                config->frog_shape = line[11];
            }
            else if (strncmp(line, "obstacle_shape=", 15) == 0) {
                config->obstacle_shape = line[15];
            }
            else if (strncmp(line, "car_shape=", 10) == 0) {
                config->car_shape = line[10];
            }
            else if (strncmp(line, "bird_shape=", 11) == 0) {
                config->bird_shape = line[11];
            }
            else if (strncmp(line, "goal_shape=", 11) == 0) {
                config->goal_shape = line[11];
            }
            else if (strncmp(line, "car_count=", 10) == 0) {
                config->car_count = atoi(line + 10);
            }
            else if (strncmp(line, "obstacle_count=", 15) == 0) {
                config->obstacle_count = atoi(line + 15);
            }
            else if (strncmp(line, "width=", 6) == 0) {
                config->width = atoi(line + 6);
            }
        }
        fclose(file);
    }
}

void init_game(Position* frog, Position* goal, GameConfig* config, Car* cars, int* level) {
    frog->x = config->width / 2;                                // Zainiciowanie samochodow
    frog->y = HEIGHT;
    goal->x = config->width / 2;
    goal->y = 1;

    int lane_count = 0;
    int* lanes = road_lanes(&lane_count);                       // Pobranie wskaznika na tablice i jej rozmiaru

    for (int i = 0; i < config->car_count; i++) {               // Tworzenie samochodow z roznymi predkosciami i losowymi kierunkami
        cars[i].interacted_with_frog = 0;                       // Poczatkowe przypisane interakcji samochodu z zaba na 0
        cars[i].x = rand() % config->width;

        int lane_found = 0;
        while (!lane_found)                                     // Szukanie wolnego pasa ruchu, jesli samochodow jest mniej niz pasow
        {
            int random_lane = lanes[rand() % lane_count];
            if (config->car_count <= lane_count)
            {
                if (!is_lane_occupied(random_lane, config, cars)) {
                    cars[i].y = random_lane;
                    lane_found = 1;
                }
            }
            else
            {
                cars[i].y = random_lane;
                lane_found = 1;
            }
        }
        cars[i].velocity = rand() % MAX_VELOCITY + 1;           // Ustawienie predkosci samochodu
        if (rand() % 2 == 0)
        {
            cars[i].direction = 1;
        }
        else                                                    // Losowy kierunek: 1 (prawo) lub -1 (lewo)
        {
            cars[i].direction = -1;
        }
        friendly_chooser(&cars[i], level, config);
    }
    init_colors();
}


void friendly_chooser(Car* car, int* level, GameConfig* config) {
    int friendly = 0;                                           // Ustalenie "wrogi"/"przyjazny" dla kazdego samochodu w zaleznosci od poziomu
    if (*level == 1) {
        friendly = rand() % 3;
        if (friendly == 0 || friendly == 1) {                   // 2/3 samochodow sa przyjazne
            car->is_friendly = 1;
        }
        else {
            car->is_friendly = 0;
        }
    }
    else if (*level == 2) {
        friendly = rand() % 3;
        if (friendly == 0) {                                    // 1/3 samochodow jest przyjaznych
            car->is_friendly = 1;
        }
        else {
            car->is_friendly = 0;
        }
    }
    else if (*level == 3) {                                     // Wszystkie wrogie. 
        car->is_friendly = 0;
    }
}

void init_colors() {
    init_color(44, 400, 400, 400);
    init_color(45, 200, 200, 200);
    init_color(46, 500, 250, 80);
    init_pair(1, COLOR_RED, 44);             // Wrogi samochod (czerwony)
    init_pair(2, COLOR_BLACK, COLOR_GREEN);  // zaba (zielona)
    init_pair(3, COLOR_CYAN, COLOR_BLACK);   // Status (niebieski)
    init_pair(4, COLOR_CYAN, 44);            // Przyjazny samochod (niebieski)
    init_pair(5, COLOR_YELLOW, COLOR_BLACK); // Ramka gry (zolta)
    init_pair(6, 45, COLOR_WHITE);           // Bocian (bialo-czarny)
    init_pair(7, COLOR_WHITE, 44);           // Pasy drogowe (szare)
    init_pair(8, COLOR_BLACK, 46);           // Przeszkody (brazowe)
}

void draw_border(GameConfig* config) {                          // Rysowanie granic gry
    for (int i = 0; i < config->width; i++) {
        attron(COLOR_PAIR(5));
        mvaddch(0, i, '*');
        mvaddch(HEIGHT + 1, i, '*');
        mvaddch(HEIGHT + 4, i, '*');
        attroff(COLOR_PAIR(5));
    }

    for (int i = 0; i < HEIGHT + 5; i++) {
        attron(COLOR_PAIR(5));
        mvaddch(i, 0, '*');
        mvaddch(i, config->width, '*');
        attroff(COLOR_PAIR(5));
    }
}

int is_lane_occupied(int lane_y, GameConfig* config, Car* cars) {
    for (int i = 0; i < config->car_count; i++) {
        if (cars[i].y == lane_y) {
            return 1;                       // Pas jest zajety
        }
    }
    return 0;                               // Pas jest wolny
}

void draw_status(int* jumps, Position* frog, GameConfig* config, int* level) {
    char statusx[5];                                            // Wyswietlanie statusu gry
    char statusy[5];
    char statusjumps[10];
    char levelstatus[10];

    sprintf(statusx, "X:%d", frog->x);
    sprintf(statusy, "Y:%d", frog->y);
    sprintf(statusjumps, "Jumps:%d", *jumps);
    sprintf(levelstatus, "Level:%d", *level);
    mvprintw(HEIGHT + 2, 1, statusx);                           // Wyswietlanie wartosci x zaby
    mvprintw(HEIGHT + 2, 6, statusy);                           // Wyswietlanie wartosci y zaby
    mvprintw(HEIGHT + 2, 11, statusjumps);                      // Wyswietlanie ilosci wykonanych skokow
    attron(COLOR_PAIR(2));
    mvprintw(HEIGHT + 2, 20, levelstatus);                      // Wyswietlanie aktualnego poziomu
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(3));
    mvprintw(HEIGHT + 3, 1, "Pawel Hiszpanski 203455");         // Wyswietlenie danych autora projektu
    attroff(COLOR_PAIR(3));
}

void draw_time(clock_t* start_time, GameConfig* config) {       // Obliczanie czasu spedzonego w grze
    clock_t current_time = clock();
    int elapsed_time = (int)((current_time - *start_time) / CLOCKS_PER_SEC);

    char time_str[18];
    snprintf(time_str, sizeof(time_str), "Time: %d seconds", elapsed_time);
    mvprintw(0, (config->width / 2) - (int)strlen(time_str) / 2, time_str);     // Wyswietlanie czasu spedzonego w grze
}

void draw_road_lanes(GameConfig* config) {                          // Rysowanie pasow drogowych
    int lane_count = HEIGHT / (LINE_HEIGHT + GAP);
    for (int i = 0; i < lane_count; i++) {

        int lane_start = 2 + i * (LINE_HEIGHT + GAP);               // Obliczenie pozycji pierwszego pasa

        for (int y = 0; y < LINE_HEIGHT; y++) {
            for (int x = 1; x < config->width; x++) {
                attron(COLOR_PAIR(7));
                mvaddch(lane_start + y, x, '-');
                attroff(COLOR_PAIR(7));
            }
        }
    }
}

void draw(clock_t* start_time, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level) {
    clear();                                            // Glowna funkcja do rysowania wszystkich elementow gry
    if (*level != 1)
    {
        draw_obstacles(obstacles, config);              // Wyswietlanie przeszkod
    }
    draw_road_lanes(config);                            // Wyswietlenie pasow drogowych

    for (int i = 0; i < config->car_count; i++) {       // Wyswietlenie samochodow
        if (cars[i].is_friendly) {
            attron(COLOR_PAIR(4));
        }
        else {
            attron(COLOR_PAIR(1));
        }
        mvaddch(cars[i].y, cars[i].x, config->car_shape);

        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(4));
    }

    draw_border(config);                                // Wyswietlenie ramki wokol gry
    draw_status(jumps, frog, config, level);            // Wyswietlenie statusu gry
    draw_time(start_time, config);                      // Wyswietlanie uplywajacego czasu

    attron(COLOR_PAIR(2));
    mvaddch(frog->y, frog->x, config->frog_shape);      // Rysowanie zaby
    attroff(COLOR_PAIR(2));

    attron(COLOR_PAIR(2));
    mvaddch(goal->y, goal->x, config->goal_shape);      // Rysowanie miejsca docelowego
    attroff(COLOR_PAIR(2));

    attron(COLOR_PAIR(6));
    mvaddch(stork->y, stork->x, config->bird_shape);     // Rysowanie bociana
    attroff(COLOR_PAIR(6));
}

int collision_with_obstacle(int x, int y, GameConfig* config, Position* obstacles) {
    for (int i = 0; i < config->obstacle_count; i++) {
        if (obstacles[i].x == x && obstacles[i].y == y) {
            return 1;                                   // Kolizja
        }
    }
    return 0;                                           //Brak kolizji
}

int move_frog(int direction, clock_t* last_frog_move_time, Position* frog, GameConfig* config, Position* obstacles) {
    // Wykonanie ruchu na podstawie kierunku
    if (direction == KEY_LEFT && frog->x > 1) {  // Ruch w lewo
        if (collision_with_obstacle(frog->x - 1, frog->y, config, obstacles)) {
            return 0;
        }
        frog->x--;
    }
    else if (direction == KEY_RIGHT && frog->x < config->width - 1) {  // Ruch w prawo
        if (collision_with_obstacle(frog->x + 1, frog->y, config, obstacles)) {
            return 0;
        }
        frog->x++;
    }
    else if (direction == KEY_UP && frog->y > 1) {  // Ruch do g?ry
        if (collision_with_obstacle(frog->x, frog->y - 1, config, obstacles)) {
            return 0;
        }
        frog->y--;
    }
    else if (direction == KEY_DOWN && frog->y < HEIGHT) {  // Ruch w d?
        if (collision_with_obstacle(frog->x, frog->y + 1, config, obstacles)) {
            return 0;
        }
        frog->y++;
    }
    return 1;  // Zaba si? poruszy?a
}

void move_cars(Position* frog, GameConfig* config, Car* cars, int* level, int* frames_counter) {
    int lane_count;
    int* lanes = road_lanes(&lane_count);                                   // Pobranie wskaznika na tablice i jej rozmiaru

    for (int i = 0; i < config->car_count; i++) {
        int distance = (int)distance_sq(frog->x, frog->y, cars[i].x, cars[i].y);

        if (*frames_counter % 20 == 0 && rand() % 3 == 0)
        {
            cars[i].velocity = rand() % MAX_VELOCITY + 1;
        }

        if (*level != 3 && *frames_counter % 20 == 0)
        {

            if (distance < STOP_DISTANCE * STOP_DISTANCE)
            {
                cars[i].velocity = 0;
            }
        }

        if (cars[i].velocity == 0 && distance > STOP_DISTANCE * STOP_DISTANCE)
        {
            cars[i].velocity = rand() % MAX_VELOCITY + 1;
        }

        cars[i].x += cars[i].velocity * cars[i].direction;

        if (cars[i].x <= 0 || cars[i].x >= config->width) {             // Obsluga przekroczenia krawedzi gry
            friendly_chooser(&cars[i], level, config);
            cars[i].interacted_with_frog = 0;

            int lane_found = 0;

            while (!lane_found) {
                int random_lane = lanes[rand() % lane_count];
                if (config->car_count <= lane_count)
                {
                    if (!is_lane_occupied(random_lane, config, cars)) {
                        cars[i].y = random_lane;
                        lane_found = 1;
                    }
                }
                else
                {
                    cars[i].y = random_lane;
                    lane_found = 1;
                }
            }

            if (rand() % 2 == 0)                // Samochod obiera losowy kierunek i wyswietla sie w odpowiednim miejscu  
            {
                cars[i].x = 0;
                cars[i].direction = 1;
            }
            else
            {
                cars[i].x = config->width;
                cars[i].direction = -1;
            }
        }
    }
}

void check_collisions(clock_t* start_time, int* score, int* game_over, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level) {
    if (*game_over)
    {
        return;
    }

    checking_car_route(game_over, start_time, jumps, frog, goal, stork, config, cars, obstacles, level, score);

    if (frog->x == goal->x && frog->y == goal->y) {
        if (*level == 3) {
            *score = *score + 1500;
            display_message("You completed all levels!", config);
            points(start_time, score, config, jumps, level, game_over);
            *game_over = 1;
            napms(3000);
        }
        else
        {
            frog_on_goal(start_time, jumps, frog, goal, stork, config, cars, obstacles, level, game_over);
        }
    }

    if (stork->x == frog->x && stork->y == frog->y) {
        *game_over = 1;
        display_message("Game Over! The stork caught you!", config); // Sprawdzanie, czy bocian zlapal zabe
        napms(1000);
    }
}

void checking_car_route(int* game_over, clock_t* start_time, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level, int* score) {
    for (int i = 0; i < config->car_count; i++) {                                       // Analiza trasy samochodu w kotekscie zderzenia z samochodem
        int car_start_x = cars[i].x;
        int car_end_x = car_start_x + cars[i].velocity * cars[i].direction;             // Miejsce samochodu po ruchu

        if (cars[i].y == frog->y) {
            if ((car_start_x <= frog->x && frog->x <= car_end_x) || (car_end_x <= frog->x && frog->x <= car_start_x)) {         // Sprawdzamy, czy pozycja zaby znajduje sie w przedziale, przez ktory przejezdza samochod
                if (!cars[i].is_friendly) {
                    display_message("Game Over! You were hit by a car!", config);       // Zderzenie z "wrogim" samochodem
                    *game_over = 1;
                    points(start_time, score, config, jumps, level, game_over);
                    napms(3000);
                    return;
                }
                else {
                    if (cars[i].interacted_with_frog == 0) {                            // Zderzenie zaby z przyjaznym samochodem
                        cars[i].interacted_with_frog = 1;

                        if (frog->y > 3 && checking_hit(obstacles, frog, config) != 0) {
                            display_message("Press 'j' to move further!", config);
                            napms(1000);
                            draw(start_time, jumps, frog, goal, stork, config, cars, obstacles, level);

                            int ch = getch();
                            if (ch == 'j') {
                                frog->y -= 3;                                           // Ruch zaby o 3 pozycje wyzej
                                (*jumps)++;
                            }
                            else {
                                display_message("You missed your chance!", config);
                                napms(1000);
                            }
                        }
                    }
                }
            }
        }
    }
}

int distance_sq(int x, int y, int a, int b) {
    return ((x - a) * (x - a)) + ((y - b) * (y - b));
}

void points(clock_t* start_time, int* score, GameConfig* config, int* jumps, int* level, int* game_over) {
    clock_t end_time = clock();
    double summary_time = (double)(end_time - *start_time) / CLOCKS_PER_SEC;

    double penalty_points = (summary_time * 10) + (*jumps * 15);
    double base_points = (*level * 1500);

    double points = base_points - penalty_points;

    *score = points;

    if (*level == 3)
        points += 1500;

    if (*game_over == 1)
        points -= (*level * 500);

    if (points < 0)
    {
        points = 0;
    }

    char points_str[15];
    snprintf(points_str, sizeof(points_str), "Points: %.2f", points);
    attron(COLOR_PAIR(6));
    mvprintw((HEIGHT / 2) + 1, (config->width / 2) - (strlen(points_str) / 2), "Points: %.3f", points);
    attroff(COLOR_PAIR(6));
    refresh();
}

void move_stork(Position* frog, Position* stork) {
    static int stork_move_counter = 0;
    stork_move_counter++;

    if (stork->y == -1) return;                                         // Bocian jeszcze nie pojawil sie

    if (stork_move_counter >= BIRD_MOVE_PER_FRAMES) {
        move_bird_towards_frog(frog, stork);
        stork_move_counter = 0;                                         // Reset licznika ruchow bociana
    }
}

void move_bird_towards_frog(Position* frog, Position* stork) {
    if (stork->x < frog->x) {                                           // Bocian porusza sie w kierunku zaby
        stork->x++;
    }
    else if (stork->x > frog->x) {
        stork->x--;
    }

    if (stork->y < frog->y) {
        stork->y++;
    }
    else if (stork->y > frog->y) {
        stork->y--;
    }
}

void frog_on_goal(clock_t* start_time, int* jumps, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level, int* game_over) {
    display_message("Congratulations!! You reached the goal!", config);  // Sprawdzanie, czy zaba dotarla do celu
    napms(1000);

    if (*level < 3) {
        *level += 1; // Przechodzenie do kolejnego poziomu
        init_game(frog, goal, config, cars, level);
        game_loop(game_over, start_time, frog, goal, stork, config, cars, obstacles, level);
    }
    else if (*level == 3) {
        (*level) += 1;
    }
}

int checking_hit(Position* obstacles, Position* frog, GameConfig* config) {
    for (int i = 0; i < config->obstacle_count; i++)
    {
        if (obstacles[i].y == (frog->y - 3) && obstacles[i].x == frog->x)
        {
            return 0;           // Brak aktywnosci jesli zaba probuje przeskoczyc w miejsce przeszkody
        }
    }
    return 1;
}

int* road_lanes(int* lane_count) {
    static int road_lanes[] = { 2, 3, 4, 7, 8, 9, 12, 13, 14, 17, 18, 19, 22, 23, 24 };         // Miejsca pasow drogowych
    *lane_count = sizeof(road_lanes) / sizeof(road_lanes[0]);                                   // Ilosc pasow drogowych
    return road_lanes;
}

void generate_obstacles(Position* obstacles, Position* goal, GameConfig* config) {      // Generowanie przeszkod
    int lane_count;
    int* lanes = road_lanes(&lane_count);                                               // Pobranie wskaznika na tablice i jej rozmiaru

    for (int i = 0; i < config->obstacle_count; i++) {
        int x, y;
        int valid_position = 0;

        while (!valid_position) {                                    // Generowanie pozycji przeszkody, ktora nie znajduje sie na pasach
            x = rand() % (config->width - 1) + 1;
            y = rand() % (HEIGHT - 2) + 1;

            int on_lane = 0;
            for (int j = 0; j < lane_count; j++) {
                if (y == lanes[j]) {
                    on_lane = 1;                                    // Sprawdzenie czy y nie znajduje sie na pasie jezdni
                    break;
                }
            }
            if (!on_lane && x != goal->x && y != goal->y) {
                obstacles[i].x = x;
                obstacles[i].y = y;
                valid_position = 1;                                 // Zatwierdzamy pozycje, jesli nie jest na pasie
            }
        }
    }
}

void draw_obstacles(Position* obstacles, GameConfig* config) {      // Rysowanie przeszkod
    for (int i = 0; i < config->obstacle_count; i++) {
        attron(COLOR_PAIR(8));
        mvaddch(obstacles[i].y, obstacles[i].x, config->obstacle_shape);
        attroff(COLOR_PAIR(8));
    }
}

void display_message(const char* message, GameConfig* config) {     // Wyswietlanie informacji do gracza (na srodku ekranu gry)
    attron(COLOR_PAIR(6));
    if (config->width > 38)
    {
        mvprintw(HEIGHT / 2, config->width / 2 - (int)strlen(message) / 2, message);
    }
    else
    {
        mvprintw(HEIGHT / 2, 0, message);
    }
    attroff(COLOR_PAIR(6));
    refresh();
}

void game_loop(int* game_over, clock_t* start_time, Position* frog, Position* goal, Position* stork, GameConfig* config, Car* cars, Position* obstacles, int* level) {      // Glowna petla zawierajaca logike calej gry
    int score = 0;
    static int jumps = 0;
    clock_t last_frog_move_time = clock();              // Zmienna do przechowywania czasu ostatniego ruchu

    int frames_counter = 0;
    stork->y = -1;                                      // Zainicjalizowanie pozycji bociana poza gra
    stork->x = -1;

    if (*level != 1)
    {
        generate_obstacles(obstacles, goal, config);
    }

    while (!*game_over) {
        int ch = 0;
        read_char(game_over, &ch, &jumps, &last_frog_move_time, frog, config, obstacles);                                   // Obsluga nacisniecia klawisza i sprawdzenie opoznienia dla ruchu zaby
        frames_counter++;
        if (frames_counter > 20)
            frames_counter = 0;

        move_cars(frog, config, cars, level, &frames_counter);                                  // Przemieszczanie samochodow
        draw(start_time, &jumps, frog, goal, stork, config, cars, obstacles, level);            // Rysowanie stanu gry

        clock_t time_interval = (clock() - last_frog_move_time) / CLOCKS_PER_SEC;

        if (*level == 3 && stork->y == -1 && (time_interval >= BIRD_WAIT_TIME)) {               // Sprawdzenie czasu bezczynnosci gracza w kontekscie pojawienia sie bociana
            stork->x = rand() % (config->width - 2) + 1;
            stork->y = rand() % (HEIGHT - 2) + 1;
        }

        if (stork->y != -1) {
            move_stork(frog, stork);
        }

        check_collisions(start_time, &score, game_over, &jumps, frog, goal, stork, config, cars, obstacles, level);         // Sprawdzanie kolizji
        napms(70);
    }
}

void read_char(int* game_over, int* ch, int* jumps, clock_t* last_frog_move_time, Position* frog, GameConfig* config, Position* obstacles) {
    *ch = getch();
    clock_t current_time = clock();
    if (*ch == 'q') {
        *game_over = 1;                                                                         // Koniec gry po wcisnieciu 'q'
    }
    else if (*ch == KEY_LEFT || *ch == KEY_RIGHT || *ch == KEY_UP || *ch == KEY_DOWN) {
        double elapsed_time = (double)((current_time - *last_frog_move_time) * 1000 / CLOCKS_PER_SEC);       // Porownywanie roznicy czasu w milisekundach
        if (elapsed_time >= FROG_MOVE_DELAY)
        {
            if (move_frog(*ch, last_frog_move_time, frog, config, obstacles)) {
                (*jumps)++;
                *last_frog_move_time = clock();// Zaba nie porusza si?, je?li nie min?o wystarczaj?co du?o czasu
            }
        }

        if (*jumps > 999)                                                                       // Koniec gry po wykonaniu 1000 ruchow
        {
            display_message("You made too many jumps", config);
            *game_over = 1;
            napms(2000);
        }
    }
}