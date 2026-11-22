#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GROW_TIME 1.f
#define GROW_AMOUNT 5.f

#define CORE_SIDES 8
#define CORE_ROTATION 22.5f

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
    Vector2 velocity;
    float rotation;
    float radius;
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
     .velocity  = {0.f,0.f},
     .radius    = 18.f
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

static Vector2 core_vertex(Vector2 c, float radius, int i) {
    float a = (CORE_ROTATION + i*(360.f/CORE_SIDES))*DEG2RAD;
    return (Vector2){c.x + cosf(a)*radius, c.y + sinf(a)*radius};
}

static int inside_inflated_core(
    Vector2 p, Vector2 c, float radius,
    Vector2 *nearest_n, float *penetration)
{
    float max_s = -INFINITY;
    int i;
    for (i = 0; i < CORE_SIDES; i++) {
        Vector2 a = core_vertex(c, radius, i);
        Vector2 b = core_vertex(c, radius, (i+1)%CORE_SIDES);
        Vector2 e = { b.x-a.x, b.y-a.y };
        float len = sqrtf(e.x*e.x + e.y*e.y);
        Vector2 n = { e.y/len, -e.x/len };

        float s = (p.x-a.x)*n.x + (p.y-a.y)*n.y;
        if (s > max_s) {
            max_s = s;
            *nearest_n = n;
        }
    }

    *penetration = max_s;
    return max_s <= 0.f;
}

static int segment_interserction(
    Vector2 p0, Vector2 p1, 
    Vector2 q0, Vector2 q1, 
    Vector2 *hit)
{
    Vector2 r = { p1.x-p0.x, p1.y-p0.y };
    Vector2 s = { q1.x-q0.x, q1.y-q0.y };
    float den = r.x*s.y - r.y*s.x;
    if (fabsf(den) < 1e-6f) return 0;

    Vector2 qp = { q0.x-p0.x, q0.y-p0.y };
    float t = (qp.x*s.y - qp.y*s.x)/den;
    float u = (qp.x*r.y - qp.y*r.x)/den;
    if (t < 0.f || t > 1.f || u < 0.f || u > 1.f) return 0;

    *hit = (Vector2){ p0.x + t*r.x, p0.y + t*r.y };
    return 1;
}

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
    DrawPolyLines(c, CORE_SIDES, room->core_radius, 22.5f, WHITE);
}

void update_player(Room *room) {

    if (IsKeyDown(KEY_A)) {
        player.rotation -= 0.05f;
    }
    if (IsKeyDown(KEY_D)) {
        player.rotation += 0.05f;
    }

    float cos_a = cosf(player.rotation);
    float sin_a = sinf(player.rotation);
    Vector2 f = { cos_a, sin_a };

    if (IsKeyDown(KEY_W)) { // && player.velocity.magnitude < 5.f) {
        player.velocity.x += 0.1f*f.x;
        player.velocity.y += 0.1f*f.y;
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
    bool shield = IsKeyDown(KEY_LEFT_SHIFT) ? true : false;

    char velocitystr[256];
    sprintf(velocitystr, "velocity: [%.2f, %.2f]", player.velocity.x, player.velocity.y);
    DrawText(velocitystr, 24, 44, 18, GRAY);

    Vector2 next_pos = { player.world.x + player.velocity.x, player.world.y + player.velocity.y };

    { // Collision ship and room core
        Vector2 c = {GetScreenWidth()/2.f, GetScreenHeight()*0.5f};
        Vector2 d = { next_pos.x-c.x, next_pos.y-c.y };
        float inflated = room->core_radius + player.radius/cosf(PI/CORE_SIDES);

        if(d.x*d.x + d.y*d.y < inflated*inflated) {
            Vector2 contact = {0};
            Vector2 n = {0};

            int collided = 0;
            Vector2 near_n = {0};
            float s;

            if(inside_inflated_core(player.world, c, inflated, &near_n, &s)) {
                contact = (Vector2){
                    player.world.x - s*near_n.x,
                    player.world.y - s*near_n.y
                };
                n = near_n;
                collided = 1;
            } else {
                int i;
                for(i=0; i<CORE_SIDES; i++) {
                    Vector2 a = core_vertex(c, inflated, i);
                    Vector2 b = core_vertex(c, inflated, (i+1)%CORE_SIDES);
                    Vector2 e = { b.x-a.x, b.y-a.y };
                    float len = sqrtf(e.x*e.x + e.y*e.y);
                    if (len < 1e-6f) { continue; }

                    Vector2 edge_n = { e.y/len, -e.x/len };
                    Vector2 hit = {0};

                    if(segment_interserction(player.world, next_pos, a, b, &hit)) {
                        contact = hit;
                        n = edge_n;
                        collided = 1;
                        break;
                    }
                }
            }
            if(collided && shield) {
                float dot = player.velocity.x*n.x + player.velocity.y*n.y;
                float impulse = dot < 0.f ? -2.f*dot : -2.f;
                player.velocity.x += impulse * n.x;
                player.velocity.y += impulse * n.y;

                next_pos = (Vector2){ 
                    contact.x + n.x*0.1f,
                    contact.y + n.y*0.1f
                };
            }
        }
    }

    player.world.x = next_pos.x;
    player.world.y = next_pos.y;

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
    if (shield) {
        static float r = 0.f;
        r-=250.f*GetFrameTime();
        DrawPolyLines(player.world, 10, player.radius, r, WHITE);
    }

    { // friction
        float speed = sqrtf(player.velocity.x*player.velocity.x +
                            player.velocity.y*player.velocity.y);
        if (speed > 0.0f) {
            float drag = 0.05f;
            float scale = fmaxf(speed - drag, 0.0f) / speed;
            player.velocity.x *= scale;
            player.velocity.y *= scale;
        }
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
