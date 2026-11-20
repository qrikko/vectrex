#include <raylib.h>
#include <stdlib.h>
#include <math.h>

#define GROW_TIME 1.f
#define GROW_AMOUNT 5.f


enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_MAP,
    GAME_STATE_PLAYING
};

typedef struct {
    Vector2 local[5];
    Vector2 world;
    float rotation;
    float speed;
} Player;

Player player = {
     .local = { { -20.f, 10.f}, {-20.f, -10.f}, {20.f, -10.f}, {40.f, 0.f}, {20.f, 10.f} },
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

    player.speed = player.speed > 0.f ? player.speed - 0.05f : 0.f;

    float cos_a = cosf(player.rotation);
    float sin_a = sinf(player.rotation);
    Vector2 f = { cos_a, sin_a };

    player.world.x += f.x * player.speed;
    player.world.y += f.y * player.speed;

    Vector2 p[6];
    int i = 0;
    for (i=0; i<5; i++) {
        float x = player.local[i].x * cos_a - player.local[i].y * sin_a;
        float y = player.local[i].x * sin_a + player.local[i].y * cos_a;
        p[i].x = x + player.world.x;
        p[i].y = y + player.world.y;
    }

    p[5].x = p[0].x;
    p[5].y = p[0].y;

    DrawLineStrip(p, 6, WHITE);

/*
    Rectangle lr;
    lr.x = player.world.x-8.f;
    lr.y = player.world.y;
    lr.width = 6.f;
    lr.height = 24.f;

    Rectangle rr;
    rr.x = player.world.x+8.f;
    rr.y = player.world.y;
    rr.width = 6.f;
    rr.height = 24.f;

    Rectangle cr;
    cr.x = player.world.x - 2.f;
    cr.y = player.world.y;
    cr.width = 10.f;
    cr.height = 24.f;


    //DrawRectangleLinesEx(lr, 1.f, WHITE);
    Vector2 l1 = { lr.x, lr.y+lr.height };
    Vector2 l2 = { lr.x+lr.width, lr.y+lr.height };
    Vector2 l3 = { lr.x+.5f*lr.width, lr.y+lr.height + 5.f};
    //DrawTriangleLines(l1, l2, l3, WHITE);

    //DrawRectangleLinesEx(rr, 1.f, WHITE);
    Vector2 r1 = { rr.x,                rr.y+rr.height };
    Vector2 r2 = { rr.x +    rr.width,  rr.y+rr.height };
    Vector2 r3 = { rr.x +.5f*rr.width,  rr.y+rr.height + 5.f};
    //DrawTriangleLines(r1, r2, r3, WHITE);

    DrawRectangleLinesEx(cr, 1.f, WHITE);
    Vector2 c1 = { cr.x,                cr.y+cr.height };
    Vector2 c2 = { cr.x +    cr.width,  cr.y+cr.height };
    Vector2 c3 = { cr.x +.5f*cr.width,  cr.y+cr.height + 10.f};
    DrawTriangleLines(c1, c2, c3, WHITE);

    Vector2 drill[5] = {
        { cr.x +.5f*cr.width,  cr.y+cr.height + 10.f},
        { cr.x +.5f*cr.width + 2.f,  cr.y+cr.height + 12.f},
        { cr.x +.5f*cr.width - 2.f,  cr.y+cr.height + 14.f},
        { cr.x +.5f*cr.width + 1.f,  cr.y+cr.height + 16.f},
        { cr.x +.5f*cr.width - 1.f,  cr.y+cr.height + 18.f}
    };
    //DrawLineStrip(drill, 5, WHITE);
*/
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
