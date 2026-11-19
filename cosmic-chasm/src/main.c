#include <raylib.h>
#include <stdlib.h>

enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_MAP,
    GAME_STATE_PLAYING
};

typedef enum DIRECTIONS { DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW } DIRECTIONS;

typedef struct Room {
    Vector2 pos;
} Room;

typedef struct Door {
    int to_room;
    int from_room;
} Door;

typedef struct Map {
    struct Room *rooms;
    struct Door *doors;

    int num_rooms;
    int num_doors;
    int current_room;
} Map;

void clear_map(struct Map *map) {
    if(map->rooms) {
        free(map->rooms);
        free(map->doors);
        map->num_rooms = 0;
        map->current_room = 0;
    }
}


int init_map(struct Map *map) {
    map->num_rooms = 2;
    map->num_doors = 1;

    map->rooms = malloc(sizeof(struct Room) * map->num_rooms);
    map->doors = malloc(sizeof(struct Door) * map->num_doors);

    map->rooms[0].pos.x = 100.f;
    map->rooms[0].pos.y = 100.f;

    map->rooms[1].pos.x = 200.f;
    map->rooms[1].pos.y = 200.f;

    map->doors[0].from_room = 0;
    map->doors[0].to_room = 1;

    return 0;
}

void draw_map(const struct Map* map) {
    int i = 0;
    for(i=0; i<map->num_rooms-1; i++) {
        DrawPolyLines(map->rooms[i].pos, 4, 20.f, 90.f, WHITE);
    }
    DrawPolyLines(map->rooms[map->num_rooms-1].pos, 8, 20.f, 20.f, WHITE);

    for(i=0; i<map->num_doors; i++) {
        Door *door = &map->doors[i];
        Room *f = &map->rooms[door->from_room];
        Room *t = &map->rooms[door->to_room];
        DrawLineV(f->pos, t->pos, WHITE);
    }
}

void enter_room(struct Map* map, int room) {
    DrawText("Playing game, esc to quit, nothing works right now", 24, 24, 20, RAYWHITE);
}

int main() {
    const int screen_w = 700;
    const int screen_h = 800;

    InitWindow(screen_w, screen_h, "Cosmic Chasm");
    SetTargetFPS(60);

    enum GameState game_state = GAME_STATE_MENU;
    Color bg = {32, 0, 0, 255};
    struct Map map;
    float map_time = 0.f;

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bg);

        switch(game_state) {
            case GAME_STATE_MENU:
                DrawText("Ready, press space to start", 24, 24, 20, RAYWHITE);

                if(IsKeyReleased(KEY_SPACE)) {
                    init_map(&map);

                    map_time = 5.f;
                    game_state = GAME_STATE_MAP;
                }
                break;
            case GAME_STATE_MAP:
                map_time -= GetFrameTime();
                DrawText("Showing map", 24, 24, 20, RAYWHITE);
                draw_map(&map);

                if(map_time <= 0.f) {
                    game_state = GAME_STATE_PLAYING;
                }

                break;
            case GAME_STATE_PLAYING:
                enter_room(&map, 0);
                break;
        }

        EndDrawing();
    }
    
    CloseWindow();

    return 0;
}
