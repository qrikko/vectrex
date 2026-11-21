#include <raylib.h>
#include <stdlib.h>
#include <math.h>

#define GROW_TIME 1.f
#define GROW_AMOUNT 5.f

#define SHIP_PTS 18
#define DRILL_PTS 5

int last_tick = -1;

enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_MAP,
    GAME_STATE_PLAYING
};

typedef struct {
    Vector2 local[SHIP_PTS];
    Vector2 drill[5];
    Vector2 world;
    float rotation;
    float speed;
} Player;

#define xmin -10.f
#define xmax  10.f
#define ymax 10.f
#define bymax 5.f

Player player = {
     .local = { 
         // left
         { xmin,        ymax },
         { xmax,        ymax },
         { xmax+4.f,     bymax+2.f },
         { xmax,         bymax },
         { xmin,         bymax },
         { xmin,        ymax },
         // body
         { xmin,         bymax}, 
         { xmax,         bymax}, 
         { xmax+6.f,     0.f}, 
         { xmax,        -bymax}, 
         { xmin,        -bymax},
         { xmin,         bymax}, 
         // right
         { xmin,       -ymax },
         { xmax,       -ymax },
         { xmax+4.f,    -bymax-2.f },
         { xmax,        -bymax },
         { xmin,        -bymax },
         { xmin,       -ymax }
     },
     .drill = {
         // drill
         { xmax+6.f,    0.f },
         { xmax+8.f,    1.f },
         { xmax+10.f,   -2.f },
         { xmax+12.f,    2.f },
         { xmax+14.f,   -0.f }
     },
     .world     = {200.f, 200.f },
     .rotation  = 0.f, 
     .speed     =0.f 
};

typedef enum DIRECTIONS { DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW } DIRECTIONS;

typedef struct Room {
    Vector2 pos;
    float core_radius;
    float grow_time;
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
    map->rooms[0].core_radius = 10.f;
    map->rooms[0].grow_time = GROW_TIME;

    map->rooms[1].pos.x = 200.f;
    map->rooms[1].pos.y = 200.f;
    map->rooms[1].core_radius = 10.f;
    map->rooms[1].grow_time = GROW_TIME;

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

void draw_room(const Room *room) {
    Vector2 c = {GetScreenWidth()/2.f, GetScreenHeight()*0.5f};
    DrawPolyLines(c, 8, c.x-10.f, 22.5f, WHITE);

    DrawPolyLines(c, 8, room->core_radius, 22.5f, WHITE);
}

void update_player(Room *room) {

    if (IsKeyDown(KEY_W) && player.speed < 5.f) {
        player.speed += 0.1f;
    }
    if (IsKeyDown(KEY_S) && player.speed < 5.f) {
        player.speed -= 0.1f;
    }
    if (IsKeyDown(KEY_A)) {
        player.rotation -= 0.05f;
    }
    if (IsKeyDown(KEY_D)) {
        player.rotation += 0.05f;
    }
    if (IsKeyDown(KEY_SPACE)) {
        int current_tick = (int)(GetTime() / 0.05f);
        if (current_tick != last_tick) { 
            int i = 0;
            for (i=0; i<DRILL_PTS; i++) {
                player.drill[i].y *= -1.f;
            }

            last_tick = current_tick;
        }
    }
    //bool shield = IsKeyDown(KEY_LEFT_SHIFT) ? true : false;

    player.speed = player.speed > 0.f ? player.speed - 0.05f : 0.f;

    float cos_a = cosf(player.rotation);
    float sin_a = sinf(player.rotation);
    Vector2 f = { cos_a, sin_a };

    player.world.x += f.x * player.speed;
    player.world.y += f.y * player.speed;

    Vector2 p[SHIP_PTS];
    int i = 0;
    for (i=0; i<SHIP_PTS; i++) {
        float x = player.local[i].x * cos_a - player.local[i].y * sin_a;
        float y = player.local[i].x * sin_a + player.local[i].y * cos_a;
        p[i].x = x + player.world.x;
        p[i].y = y + player.world.y;
    }

    Vector2 drill[DRILL_PTS];

    for (i=0; i<DRILL_PTS; i++) {
        float x = player.drill[i].x * cos_a - player.drill[i].y * sin_a;
        float y = player.drill[i].x * sin_a + player.drill[i].y * cos_a;
        drill[i].x = x + player.world.x;
        drill[i].y = y + player.world.y;
    }

    DrawLineStrip(p, SHIP_PTS, WHITE);
    DrawLineStrip(drill, DRILL_PTS, WHITE);
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        static float r = 0.f;
        r-=250.f*GetFrameTime();
        DrawPolyLines(player.world, 10, 18.f, r, WHITE);
    }

}

void update_room(struct Map* map, int room_idx) {
    DrawText("Playing game, esc to quit, nothing works right now", 24, 24, 20, RAYWHITE);
    Room *room = &map->rooms[room_idx];

    update_player(room);

    room->grow_time -= GetFrameTime();
    if(room->grow_time <= 0.f) {
        room->core_radius += GROW_AMOUNT;
        room->grow_time = GROW_TIME;
    }

    draw_room(room);
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

                if(IsKeyReleased(KEY_SPACE)) {
                    map_time = 0.f;
                }
                if(map_time <= 0.f) {
                    game_state = GAME_STATE_PLAYING;
                }

                break;
            case GAME_STATE_PLAYING:
                update_room(&map, 0);

                break;
        }

        EndDrawing();
    }
    
    CloseWindow();

    return 0;
}
