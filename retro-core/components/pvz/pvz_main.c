#include "rg_system.h"
#include "rg_display.h"
#include "rg_input.h"
#include "esp_heap_caps.h"
#include "pvz_sprites.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PEAS 24
#define MAX_ZOMBIES 20
#define MAX_SUNS 12
#define LAWN_ROWS 5
#define LAWN_COLS 8

#define LAWN_TOP 21
#define LAWN_LEFT 20
#define ROW_H 19
#define COL_W 17

#define PVZ_SAVE_MAGIC 0x50565A33 // "PVZ3"
#define PVZ_PROGRESS_FILE RG_BASE_PATH_SAVES "/pvz/progress.bin"
#define MAPS_DIR_ROM   "/sd/roms/pvz/maps"
#define MAPS_DIR_PVZ   "/sd/pvz/maps"

typedef enum {
    DIFF_EASY = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2
} difficulty_t;

// Plant Types
typedef enum {
    PLANT_NONE = 0,
    PLANT_PEASHOOTER = 1,
    PLANT_SUNFLOWER = 2,
    PLANT_CHERRY = 3,
    PLANT_WALLNUT = 4,
    PLANT_POTATO = 5,
    PLANT_SNOWPEA = 6,
    PLANT_CHOMPER = 7,
    PLANT_REPEATER = 8,
    PLANT_JALAPENO = 9,
    PLANT_SHOVEL = 10,
    TOTAL_PLANTS = 10
} plant_type_t;

static const char *plant_names[TOTAL_PLANTS] = {
    "NONE", "PEASHOOTER", "SUNFLOWER", "CHERRY BOMB",
    "WALL-NUT", "POTATO MINE", "SNOW PEA", "CHOMPER",
    "REPEATER", "JALAPENO"
};

static const int plant_costs[TOTAL_PLANTS] = {
    0, 100, 50, 150, 50, 25, 175, 150, 200, 125
};

static const int plant_cooldowns[TOTAL_PLANTS] = {
    0, 150, 150, 600, 400, 450, 200, 250, 200, 600
};

// Zombie Types
typedef enum {
    ZOMBIE_NORMAL = 0,
    ZOMBIE_CONE = 1,
    ZOMBIE_BUCKET = 2,
    ZOMBIE_FLAG = 3,
    ZOMBIE_FOOTBALL = 4,
    ZOMBIE_NEWSPAPER = 5,
    ZOMBIE_VAULTING = 6,
    TOTAL_ZOMBIE_TYPES = 7
} zombie_type_t;

#define MAX_LOADOUT_SLOTS 6

typedef struct {
    plant_type_t type;
    int hp;
    int max_hp;
    int timer;
    int anim;
    bool is_grave;
    bool armed;       // For Potato Mine
    bool chewing;     // For Chomper
} cell_plant_t;

typedef struct {
    bool active;
    zombie_type_t type;
    float x;
    int row;
    int hp;
    int max_hp;
    int slow_timer;
    int anim;
    bool angry;       // For Newspaper Zombie (sprint when paper broken)
    bool vaulted;     // For Pole Vaulting Zombie
} entity_zombie_t;

typedef struct {
    bool active;
    float x;
    int y;
    int row;
    bool is_ice;
} entity_pea_t;

typedef struct {
    bool active;
    float x, y;
    float target_y;
    int timer;
    bool is_natural;
} entity_sun_t;

typedef struct {
    bool active;
    float x;
    bool triggered;
} entity_mower_t;

typedef struct {
    int x, y;
    int timer;
    char text[10];
} floater_t;

typedef struct {
    char filename[64];
    char name[24];
    int lanes;
    bool is_night;
    int sun;
    int zombies_count;
    int spawn_speed;
    int graves_count;
} custom_map_t;

typedef enum {
    STATE_MAIN_MENU,
    STATE_LEVEL_SELECT,
    STATE_CHOOSE_SEEDS,
    STATE_HELP,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_NEW_PLANT_UNLOCKED
} game_state_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    int current_level;
    int unlocked_level;
    uint32_t unlocked_plants_mask;
    int difficulty;
    int player_sun;
    int zombies_to_spawn;
    int zombies_spawned;
    int zombies_killed;
    cell_plant_t lawn[LAWN_ROWS][LAWN_COLS];
    entity_zombie_t zombies[MAX_ZOMBIES];
    entity_mower_t mowers[LAWN_ROWS];
    int active_loadout[MAX_LOADOUT_SLOTS];
} pvz_save_data_t;

// World & Game State
static cell_plant_t lawn[LAWN_ROWS][LAWN_COLS];
static entity_zombie_t zombies[MAX_ZOMBIES];
static entity_pea_t peas[MAX_PEAS];
static entity_sun_t suns[MAX_SUNS];
static entity_mower_t mowers[LAWN_ROWS];
static floater_t floaters[6];

static int player_sun = 150;
static int selected_seed_slot = -1; // -1 = None, 0..5
static int seed_cooldown_timers[MAX_LOADOUT_SLOTS];

static int active_loadout[MAX_LOADOUT_SLOTS] = {
    PLANT_PEASHOOTER, PLANT_SUNFLOWER, PLANT_WALLNUT,
    PLANT_CHERRY, PLANT_SNOWPEA, PLANT_SHOVEL
};

static uint32_t unlocked_plants_mask = (1 << PLANT_PEASHOOTER); // Bitmask of unlocked plants
static int newly_unlocked_plant = PLANT_NONE;

static int cursor_row = 2; // -1 = Card Bar, 0..4 = Lawn
static int cursor_col = 3; // 0..7 on Lawn, 0..5 on Card Bar

static int current_level = 1;
static int unlocked_level = 1;
static difficulty_t game_difficulty = DIFF_NORMAL;

static int zombies_to_spawn = 6;
static int zombies_spawned = 0;
static int zombies_killed = 0;
static int spawn_timer = 450;
static int natural_sun_timer = 200;
static int wave_banner_timer = 0;
static int jalapeno_burn_row = -1;
static int jalapeno_burn_timer = 0;

static game_state_t game_state = STATE_MAIN_MENU;
static int menu_cursor = 0;
static int pause_menu_cursor = 0;
static int seed_select_cursor = 0;
static int cloud_offset = 0;
static uint16_t *g_display_buffer = NULL;

// Joystick Auto-Repeat timers
static int repeat_timer_x = 0;
static int repeat_timer_y = 0;

// Custom Maps list
static custom_map_t custom_maps[16];
static int custom_maps_count = 0;
static bool is_custom_map = false;
static int level_select_tab = 0; // 0 = Campaign, 1 = Custom Maps
static custom_map_t active_custom_map;

// Forward Declarations
static void reset_level(int level);
static void start_custom_map(const custom_map_t *map);
static void scan_custom_maps(void);

// ----------------------------------------------------------------------------
// Lane Active Helper
// ----------------------------------------------------------------------------
static inline bool is_lane_active(int level, int row) {
    if (is_custom_map) {
        if (active_custom_map.lanes == 1) return (row == 2);
        if (active_custom_map.lanes == 3) return (row >= 1 && row <= 3);
        return true;
    }
    if (level == 1) return (row == 2);
    if (level == 2) return (row >= 1 && row <= 3);
    return true; // level >= 3: all 5 lanes
}

static inline bool is_night_level(int level) {
    if (is_custom_map) return active_custom_map.is_night;
    return (level == 6 || level == 7 || level == 8);
}

// ----------------------------------------------------------------------------
// Progression Save / Load
// ----------------------------------------------------------------------------
static void save_progress(void) {
    rg_storage_mkdir(RG_BASE_PATH_SAVES "/pvz");
    FILE *f = fopen(PVZ_PROGRESS_FILE, "wb");
    if (f) {
        uint32_t data[4] = {
            PVZ_SAVE_MAGIC,
            (uint32_t)unlocked_level,
            unlocked_plants_mask,
            (uint32_t)game_difficulty
        };
        fwrite(data, sizeof(uint32_t), 4, f);
        fclose(f);
        rg_storage_commit();
    }
}

static void load_progress(void) {
    FILE *f = fopen(PVZ_PROGRESS_FILE, "rb");
    if (f) {
        uint32_t data[4];
        size_t n = fread(data, sizeof(uint32_t), 4, f);
        if (n >= 2 && data[0] == PVZ_SAVE_MAGIC) {
            unlocked_level = (int)data[1];
            if (unlocked_level < 1) unlocked_level = 1;
            if (unlocked_level > 10) unlocked_level = 10;

            if (n >= 3 && data[2] != 0) {
                unlocked_plants_mask = data[2];
            } else {
                unlocked_plants_mask = (1 << PLANT_PEASHOOTER);
            }

            if (n >= 4) {
                game_difficulty = (difficulty_t)data[3];
                if (game_difficulty > DIFF_HARD) game_difficulty = DIFF_NORMAL;
            }
        }
        fclose(f);
    } else {
        unlocked_plants_mask = (1 << PLANT_PEASHOOTER);
    }
}

// ----------------------------------------------------------------------------
// Custom Maps Scanning & Safe JSON Parser
// ----------------------------------------------------------------------------
static void ensure_sample_maps(const char *dir_path) {
    char path[256];
    snprintf(path, sizeof(path), "%s/sample_survival.json", dir_path);
    FILE *f = fopen(path, "r");
    if (!f) {
        f = fopen(path, "w");
        if (f) {
            fprintf(f, "{\n  \"name\": \"SURVIVAL DAY\",\n  \"lanes\": 5,\n  \"is_night\": 0,\n  \"sun\": 250,\n  \"zombies\": 40,\n  \"spawn_speed\": 160,\n  \"graves\": 0\n}\n");
            fclose(f);
        }
    } else {
        fclose(f);
    }

    snprintf(path, sizeof(path), "%s/sample_night_horde.json", dir_path);
    f = fopen(path, "r");
    if (!f) {
        f = fopen(path, "w");
        if (f) {
            fprintf(f, "{\n  \"name\": \"NIGHT HORDE\",\n  \"lanes\": 5,\n  \"is_night\": 1,\n  \"sun\": 200,\n  \"zombies\": 35,\n  \"spawn_speed\": 140,\n  \"graves\": 4\n}\n");
            fclose(f);
        }
    } else {
        fclose(f);
    }
}

static void parse_json_map(const char *filepath, custom_map_t *out) {
    FILE *f = fopen(filepath, "r");
    if (!f) return;

    char buf[1024];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';

    // Defaults
    snprintf(out->name, sizeof(out->name), "CUSTOM MAP");
    out->lanes = 5;
    out->is_night = false;
    out->sun = 150;
    out->zombies_count = 30;
    out->spawn_speed = 180;
    out->graves_count = 0;

    // Parse "name"
    char *p = strstr(buf, "\"name\"");
    if (p) {
        char *col = strchr(p, ':');
        if (col) {
            char *q1 = strchr(col, '\"');
            if (q1) {
                char *q2 = strchr(q1 + 1, '\"');
                if (q2) {
                    int nlen = q2 - (q1 + 1);
                    if (nlen > 20) nlen = 20;
                    snprintf(out->name, sizeof(out->name), "%.*s", nlen, q1 + 1);
                }
            }
        }
    }

    // Parse integer fields
    p = strstr(buf, "\"lanes\"");
    if (p) sscanf(p, "\"lanes\"%*[^0-9]%d", &out->lanes);
    if (out->lanes != 1 && out->lanes != 3) out->lanes = 5;

    p = strstr(buf, "\"is_night\"");
    if (p) {
        int night = 0;
        sscanf(p, "\"is_night\"%*[^0-9]%d", &night);
        out->is_night = (night != 0);
    }

    p = strstr(buf, "\"sun\"");
    if (p) sscanf(p, "\"sun\"%*[^0-9]%d", &out->sun);
    if (out->sun < 50) out->sun = 50;

    p = strstr(buf, "\"zombies\"");
    if (p) sscanf(p, "\"zombies\"%*[^0-9]%d", &out->zombies_count);
    if (out->zombies_count < 5) out->zombies_count = 5;

    p = strstr(buf, "\"spawn_speed\"");
    if (p) sscanf(p, "\"spawn_speed\"%*[^0-9]%d", &out->spawn_speed);
    if (out->spawn_speed < 40) out->spawn_speed = 40;

    p = strstr(buf, "\"graves\"");
    if (p) sscanf(p, "\"graves\"%*[^0-9]%d", &out->graves_count);
    if (out->graves_count < 0) out->graves_count = 0;
    if (out->graves_count > 8) out->graves_count = 8;
}

static void scan_custom_maps(void) {
    custom_maps_count = 0;

    const char *dirs[] = {MAPS_DIR_ROM, MAPS_DIR_PVZ};

    for (int d = 0; d < 2; d++) {
        rg_storage_mkdir(dirs[d]);
        ensure_sample_maps(dirs[d]);

        DIR *dir = opendir(dirs[d]);
        if (!dir) continue;

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL && custom_maps_count < 16) {
            char *dot = strrchr(ent->d_name, '.');
            if (dot && (strcmp(dot, ".json") == 0 || strcmp(dot, ".JSON") == 0)) {
                char full_path[300];
                snprintf(full_path, sizeof(full_path), "%s/%.60s", dirs[d], ent->d_name);
                snprintf(custom_maps[custom_maps_count].filename, sizeof(custom_maps[custom_maps_count].filename), "%.60s", ent->d_name);
                parse_json_map(full_path, &custom_maps[custom_maps_count]);
                custom_maps_count++;
            }
        }
        closedir(dir);
        if (custom_maps_count > 0) break;
    }
}

// ----------------------------------------------------------------------------
// Retro-Go In-Game Save State Handlers
// ----------------------------------------------------------------------------
static bool pvz_save_state(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return false;

    pvz_save_data_t data = {
        .magic = PVZ_SAVE_MAGIC,
        .version = 3,
        .current_level = current_level,
        .unlocked_level = unlocked_level,
        .unlocked_plants_mask = unlocked_plants_mask,
        .difficulty = (int)game_difficulty,
        .player_sun = player_sun,
        .zombies_to_spawn = zombies_to_spawn,
        .zombies_spawned = zombies_spawned,
        .zombies_killed = zombies_killed,
    };
    memcpy(data.lawn, lawn, sizeof(lawn));
    memcpy(data.zombies, zombies, sizeof(zombies));
    memcpy(data.mowers, mowers, sizeof(mowers));
    memcpy(data.active_loadout, active_loadout, sizeof(active_loadout));

    size_t written = fwrite(&data, sizeof(data), 1, f);
    fclose(f);
    return (written == 1);
}

static bool pvz_load_state(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return false;

    pvz_save_data_t data;
    size_t read = fread(&data, sizeof(data), 1, f);
    fclose(f);

    if (read != 1 || data.magic != PVZ_SAVE_MAGIC) return false;

    current_level = data.current_level;
    unlocked_level = data.unlocked_level;
    unlocked_plants_mask = data.unlocked_plants_mask;
    game_difficulty = (difficulty_t)data.difficulty;
    player_sun = data.player_sun;
    zombies_to_spawn = data.zombies_to_spawn;
    zombies_spawned = data.zombies_spawned;
    zombies_killed = data.zombies_killed;

    memcpy(lawn, data.lawn, sizeof(lawn));
    memcpy(zombies, data.zombies, sizeof(zombies));
    memcpy(mowers, data.mowers, sizeof(mowers));
    memcpy(active_loadout, data.active_loadout, sizeof(active_loadout));

    memset(peas, 0, sizeof(peas));
    memset(suns, 0, sizeof(suns));
    memset(floaters, 0, sizeof(floaters));

    game_state = STATE_PLAYING;
    return true;
}

static bool pvz_screenshot(const char *filename, int width, int height) {
    if (!g_display_buffer) return false;
    rg_surface_t surf = {
        .width = SCREEN_W,
        .height = SCREEN_H,
        .stride = SCREEN_W * 2,
        .format = RG_PIXEL_565_LE,
        .data = g_display_buffer,
    };
    return rg_surface_save_image_file(&surf, filename, width, height);
}

static bool pvz_reset(bool hard) {
    (void)hard;
    reset_level(current_level);
    return true;
}

// ----------------------------------------------------------------------------
// Level Reset & Setup
// ----------------------------------------------------------------------------
static void setup_level_plants(int level) {
    // Chỉ dựa vào unlocked_level cao nhất để mở khóa mask
    int max_lvl = unlocked_level; 
    if (max_lvl >= 1) unlocked_plants_mask |= (1 << PLANT_PEASHOOTER);
    if (max_lvl >= 2) unlocked_plants_mask |= (1 << PLANT_SUNFLOWER);
    if (max_lvl >= 3) unlocked_plants_mask |= (1 << PLANT_CHERRY);
    if (max_lvl >= 4) unlocked_plants_mask |= (1 << PLANT_WALLNUT);
    if (max_lvl >= 5) unlocked_plants_mask |= (1 << PLANT_POTATO);
    if (max_lvl >= 6) unlocked_plants_mask |= (1 << PLANT_SNOWPEA);
    if (max_lvl >= 7) unlocked_plants_mask |= (1 << PLANT_CHOMPER);
    if (max_lvl >= 8) unlocked_plants_mask |= (1 << PLANT_REPEATER);
    if (max_lvl >= 9) unlocked_plants_mask |= (1 << PLANT_JALAPENO);

    // Lọc các cây chưa mở khóa khỏi loadout hiện tại
    int slot = 0;
    int temp_loadout[MAX_LOADOUT_SLOTS] = {0};
    
    for (int i = 0; i < MAX_LOADOUT_SLOTS - 1; i++) {
        int p = active_loadout[i];
        if (p != PLANT_NONE && (unlocked_plants_mask & (1 << p))) {
            temp_loadout[slot++] = p;
        }
    }
    
    // Nếu loadout trống, tự động điền các cây đã mở khóa
    if (slot == 0) {
        for (int p = 1; p < TOTAL_PLANTS && slot < 5; p++) {
            if (unlocked_plants_mask & (1 << p)) {
                temp_loadout[slot++] = p;
            }
        }
    }
    
    // Cập nhật lại loadout chính thức
    for (int i = 0; i < 5; i++) {
        active_loadout[i] = temp_loadout[i];
    }
    active_loadout[5] = PLANT_SHOVEL;
}

static void reset_level(int level) {
    is_custom_map = false;
    current_level = level;
    player_sun = 150;
    selected_seed_slot = -1;
    wave_banner_timer = 0;
    jalapeno_burn_row = -1;
    jalapeno_burn_timer = 0;

    cursor_row = (level == 1) ? 2 : ((level == 2) ? 2 : 2);
    cursor_col = 3;

    for (int r = 0; r < LAWN_ROWS; r++) {
        for (int c = 0; c < LAWN_COLS; c++) {
            lawn[r][c].type = PLANT_NONE;
            lawn[r][c].hp = 0;
            lawn[r][c].max_hp = 0;
            lawn[r][c].timer = 0;
            lawn[r][c].anim = 0;
            lawn[r][c].is_grave = false;
            lawn[r][c].armed = false;
            lawn[r][c].chewing = false;
        }
    }

    // Graves for Night levels (6, 7, 8)
    if (level == 6) {
        lawn[1][6].is_grave = true;
        lawn[3][7].is_grave = true;
    } else if (level == 7 || level == 8) {
        lawn[1][5].is_grave = true;
        lawn[2][7].is_grave = true;
        lawn[3][6].is_grave = true;
    }

    memset(zombies, 0, sizeof(zombies));
    memset(peas, 0, sizeof(peas));
    memset(suns, 0, sizeof(suns));
    memset(floaters, 0, sizeof(floaters));
    memset(seed_cooldown_timers, 0, sizeof(seed_cooldown_timers));

    for (int r = 0; r < LAWN_ROWS; r++) {
        mowers[r].active = is_lane_active(level, r);
        mowers[r].x = 2;
        mowers[r].triggered = false;
    }

    // Zombies Count
    if (level == 1)      zombies_to_spawn = 6;
    else if (level == 2) zombies_to_spawn = 12;
    else if (level == 3) zombies_to_spawn = 16;
    else if (level == 4) zombies_to_spawn = 20;
    else if (level == 5) zombies_to_spawn = 24;
    else if (level == 6) zombies_to_spawn = 26; // Night
    else if (level == 7) zombies_to_spawn = 30; // Football
    else if (level == 8) zombies_to_spawn = 34; // Newspaper
    else                 zombies_to_spawn = 40; // Final Stand

    zombies_spawned = 0;
    zombies_killed = 0;

    spawn_timer = (level == 1) ? 450 : ((level == 2) ? 360 : 300);
    natural_sun_timer = 120;
    game_state = STATE_PLAYING;
}

static void start_custom_map(const custom_map_t *map) {
    is_custom_map = true;
    active_custom_map = *map;
    player_sun = map->sun;
    selected_seed_slot = -1;
    wave_banner_timer = 0;
    jalapeno_burn_row = -1;
    jalapeno_burn_timer = 0;

    cursor_row = 2;
    cursor_col = 3;

    for (int r = 0; r < LAWN_ROWS; r++) {
        for (int c = 0; c < LAWN_COLS; c++) {
            lawn[r][c].type = PLANT_NONE;
            lawn[r][c].hp = 0;
            lawn[r][c].max_hp = 0;
            lawn[r][c].timer = 0;
            lawn[r][c].anim = 0;
            lawn[r][c].is_grave = false;
            lawn[r][c].armed = false;
            lawn[r][c].chewing = false;
        }
    }

    // Spawn graves randomly
    int g_placed = 0;
    while (g_placed < map->graves_count) {
        int r = rand() % LAWN_ROWS;
        int c = 5 + (rand() % 3);
        if (is_lane_active(99, r) && !lawn[r][c].is_grave) {
            lawn[r][c].is_grave = true;
            g_placed++;
        }
    }

    memset(zombies, 0, sizeof(zombies));
    memset(peas, 0, sizeof(peas));
    memset(suns, 0, sizeof(suns));
    memset(floaters, 0, sizeof(floaters));
    memset(seed_cooldown_timers, 0, sizeof(seed_cooldown_timers));

    for (int r = 0; r < LAWN_ROWS; r++) {
        mowers[r].active = is_lane_active(99, r);
        mowers[r].x = 2;
        mowers[r].triggered = false;
    }

    zombies_to_spawn = map->zombies_count;
    zombies_spawned = 0;
    zombies_killed = 0;
    spawn_timer = 300;
    natural_sun_timer = 120;
    game_state = STATE_PLAYING;
}

static void spawn_sun(float x, float y, float target_y, bool is_natural) {
    for (int i = 0; i < MAX_SUNS; i++) {
        if (!suns[i].active) {
            suns[i].active = true;
            suns[i].x = x;
            suns[i].y = y;
            suns[i].target_y = target_y;
            suns[i].timer = 360;
            suns[i].is_natural = is_natural;
            break;
        }
    }
}

static void add_floater(int x, int y, const char *txt) {
    for (int i = 0; i < 6; i++) {
        if (floaters[i].timer <= 0) {
            floaters[i].x = x;
            floaters[i].y = y;
            floaters[i].timer = 25;
            strncpy(floaters[i].text, txt, sizeof(floaters[i].text) - 1);
            break;
        }
    }
}

static void spawn_zombie(int type) {
    int active_rows[LAWN_ROWS];
    int count = 0;
    for (int r = 0; r < LAWN_ROWS; r++) {
        if (is_lane_active(current_level, r)) {
            active_rows[count++] = r;
        }
    }
    if (count == 0) return;

    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!zombies[i].active) {
            zombies[i].active = true;
            zombies[i].type = (zombie_type_t)type;
            zombies[i].x = SCREEN_W + 2;
            zombies[i].row = active_rows[rand() % count];
            zombies[i].slow_timer = 0;
            zombies[i].anim = 0;
            zombies[i].angry = false;
            zombies[i].vaulted = false;

            int base_hp = 200;
            if (type == ZOMBIE_CONE)        base_hp = 560;
            else if (type == ZOMBIE_BUCKET) base_hp = 1100;
            else if (type == ZOMBIE_FLAG)   base_hp = 350;
            else if (type == ZOMBIE_FOOTBALL) base_hp = 1400;
            else if (type == ZOMBIE_NEWSPAPER) base_hp = 320;
            else if (type == ZOMBIE_VAULTING)  base_hp = 350;

            if (game_difficulty == DIFF_EASY) base_hp = (base_hp * 8) / 10;
            else if (game_difficulty == DIFF_HARD) base_hp = (base_hp * 12) / 10;

            zombies[i].hp = zombies[i].max_hp = base_hp;
            zombies_spawned++;
            break;
        }
    }
}

static void collect_sun_at(float cx, float cy) {
    for (int i = 0; i < MAX_SUNS; i++) {
        if (suns[i].active) {
            float dx = suns[i].x + 5 - cx;
            float dy = suns[i].y + 5 - cy;
            if (dx * dx + dy * dy < 20 * 20) {
                suns[i].active = false;
                player_sun += 25;
                add_floater((int)suns[i].x, (int)suns[i].y, "+25");
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Update Game World
// ----------------------------------------------------------------------------
static void update_game(uint32_t joystick, uint32_t just_pressed) {
    // 1. Auto-Repeat Joystick navigation
    bool move_left = false, move_right = false, move_up = false, move_down = false;

    if (joystick & RG_KEY_LEFT) {
        if (repeat_timer_x <= 0) {
            move_left = true;
            repeat_timer_x = (repeat_timer_x == 0) ? 9 : 3;
        } else {
            repeat_timer_x--;
        }
    } else if (joystick & RG_KEY_RIGHT) {
        if (repeat_timer_x <= 0) {
            move_right = true;
            repeat_timer_x = (repeat_timer_x == 0) ? 9 : 3;
        } else {
            repeat_timer_x--;
        }
    } else {
        repeat_timer_x = 0;
    }

    if (joystick & RG_KEY_UP) {
        if (repeat_timer_y <= 0) {
            move_up = true;
            repeat_timer_y = (repeat_timer_y == 0) ? 9 : 3;
        } else {
            repeat_timer_y--;
        }
    } else if (joystick & RG_KEY_DOWN) {
        if (repeat_timer_y <= 0) {
            move_down = true;
            repeat_timer_y = (repeat_timer_y == 0) ? 9 : 3;
        } else {
            repeat_timer_y--;
        }
    } else {
        repeat_timer_y = 0;
    }

    // Khôi phục Joystick di chuyển lên thanh thẻ bài (cursor_row = -1)
    if (move_up) {
        if (cursor_row > 0) {
            if (current_level == 1 && !is_custom_map) {
                if (cursor_row == 2) cursor_row = -1; 
            } else if (current_level == 2 && !is_custom_map) {
                if (cursor_row == 1) cursor_row = -1;
                else cursor_row--;
            } else {
                cursor_row--;
            }
        } else if (cursor_row == 0) {
            cursor_row = -1;
        }
        if (cursor_row == -1 && cursor_col >= MAX_LOADOUT_SLOTS) cursor_col = MAX_LOADOUT_SLOTS - 1;
    }

    if (move_down) {
        if (cursor_row == -1) {
            if (current_level == 1 && !is_custom_map) cursor_row = 2;
            else if (current_level == 2 && !is_custom_map) cursor_row = 1;
            else cursor_row = 0;
        } else {
            if (current_level == 1 && !is_custom_map) {}
            else if (current_level == 2 && !is_custom_map) {
                if (cursor_row < 3) cursor_row++;
            } else {
                if (cursor_row < LAWN_ROWS - 1) cursor_row++;
            }
        }
    }

    if (move_left && cursor_col > 0) cursor_col--;
    if (move_right) {
        int max_c = (cursor_row == -1) ? (MAX_LOADOUT_SLOTS - 1) : (LAWN_COLS - 1);
        if (cursor_col < max_c) cursor_col++;
    }

    // Nút E / F (Map với L / R) cho đổi thẻ nhanh
    if (just_pressed & RG_KEY_L) {
        if (selected_seed_slot <= 0) selected_seed_slot = MAX_LOADOUT_SLOTS - 1;
        else selected_seed_slot--;
    }
    if (just_pressed & RG_KEY_R) {
        selected_seed_slot = (selected_seed_slot + 1) % MAX_LOADOUT_SLOTS;
    }

    // Nút C (Map với SELECT) cho Xẻng
    if (just_pressed & RG_KEY_SELECT) {
        if (selected_seed_slot == 5) selected_seed_slot = -1;
        else selected_seed_slot = 5;
    }

    // Nút D (Map với START) cho Pause
    if (just_pressed & RG_KEY_START) {
        pause_menu_cursor = 0;
        game_state = STATE_PAUSED;
        return;
    }

    // Nút B cho Hủy hoặc Hút mặt trời
    if (just_pressed & RG_KEY_B) {
        if (selected_seed_slot != -1) {
            selected_seed_slot = -1;
        } else {
            for (int i = 0; i < MAX_SUNS; i++) {
                if (suns[i].active) {
                    suns[i].active = false;
                    player_sun += 25;
                    add_floater((int)suns[i].x, (int)suns[i].y, "+25");
                }
            }
        }
    }

    int cur_px = 0, cur_py = 0;
    if (cursor_row == -1) {
        cur_px = 28 + cursor_col * 18 + 8;
        cur_py = 10;
    } else {
        cur_px = LAWN_LEFT + cursor_col * COL_W + COL_W / 2;
        cur_py = LAWN_TOP + cursor_row * ROW_H + ROW_H / 2;
    }

    collect_sun_at(cur_px, cur_py);

    // Nút A cho Chọn thẻ trên HUD hoặc Trồng cây xuống cỏ
    if (just_pressed & RG_KEY_A) {
        if (cursor_row == -1) {
            if (cursor_col < MAX_LOADOUT_SLOTS) {
                if (selected_seed_slot == cursor_col) selected_seed_slot = -1;
                else selected_seed_slot = cursor_col;
            }
        } else {
            int r = cursor_row;
            int c = cursor_col;

            if (is_lane_active(current_level, r)) {
                if (selected_seed_slot == 5) { // Shovel
                    if (lawn[r][c].type != PLANT_NONE) {
                        lawn[r][c].type = PLANT_NONE;
                        add_floater(cur_px - 8, cur_py - 6, "DUG");
                        selected_seed_slot = -1;
                    }
                } else if (selected_seed_slot >= 0) {
                    plant_type_t ptype = (plant_type_t)active_loadout[selected_seed_slot];
                    int cost = plant_costs[ptype];

                    if (player_sun >= cost && seed_cooldown_timers[selected_seed_slot] <= 0 &&
                        lawn[r][c].type == PLANT_NONE && !lawn[r][c].is_grave) {

                        player_sun -= cost;
                        seed_cooldown_timers[selected_seed_slot] = plant_cooldowns[ptype];
                        lawn[r][c].type = ptype;
                        lawn[r][c].timer = 0;
                        lawn[r][c].anim = 0;
                        lawn[r][c].armed = false;
                        lawn[r][c].chewing = false;

                        if (ptype == PLANT_WALLNUT) {
                            lawn[r][c].hp = lawn[r][c].max_hp = 4000;
                        } else if (ptype == PLANT_CHERRY) {
                            lawn[r][c].hp = lawn[r][c].max_hp = 300;
                            lawn[r][c].timer = 45; 
                        } else if (ptype == PLANT_POTATO) {
                            lawn[r][c].hp = lawn[r][c].max_hp = 300;
                            lawn[r][c].timer = 300; 
                        } else if (ptype == PLANT_JALAPENO) {
                            lawn[r][c].hp = lawn[r][c].max_hp = 300;
                            lawn[r][c].timer = 20; 
                        } else {
                            lawn[r][c].hp = lawn[r][c].max_hp = 300;
                        }
                        selected_seed_slot = -1;
                    }
                }
            }
        }
    }

    // 2. Cooldown Timers
    for (int i = 0; i < MAX_LOADOUT_SLOTS; i++) {
        if (seed_cooldown_timers[i] > 0) seed_cooldown_timers[i]--;
    }

    // 3. Natural Sun Spawner (Day levels only)
    if (!is_night_level(current_level)) {
        bool has_active_natural_sun = false;
        for (int i = 0; i < MAX_SUNS; i++) {
            if (suns[i].active && suns[i].is_natural) {
                has_active_natural_sun = true;
                break;
            }
        }
        if (!has_active_natural_sun && --natural_sun_timer <= 0) {
            int interval = 220;
            if (game_difficulty == DIFF_EASY) interval = 180;
            else if (game_difficulty == DIFF_HARD) interval = 280;

            natural_sun_timer = interval + (rand() % 60);

            int sx = LAWN_LEFT + (rand() % (LAWN_COLS * COL_W - 12));
            int target_r = rand() % LAWN_ROWS;
            if (!is_lane_active(current_level, target_r)) {
                if (current_level == 1) target_r = 2;
                else if (current_level == 2) target_r = 1 + (rand() % 3);
            }
            int sy_target = LAWN_TOP + target_r * ROW_H + 4;
            spawn_sun(sx, 0, sy_target, true);
        }
    }

    // 4. Update Suns
    for (int i = 0; i < MAX_SUNS; i++) {
        if (suns[i].active) {
            if (suns[i].y < suns[i].target_y) {
                suns[i].y += 0.8f;
            }
            if (--suns[i].timer <= 0) {
                suns[i].active = false;
            }
        }
    }

    // 5. Update Plants
    for (int r = 0; r < LAWN_ROWS; r++) {
        if (!is_lane_active(current_level, r)) continue;

        bool has_zombie_in_row = false;
        for (int z = 0; z < MAX_ZOMBIES; z++) {
            if (zombies[z].active && zombies[z].row == r && zombies[z].x > LAWN_LEFT) {
                has_zombie_in_row = true;
                break;
            }
        }

        for (int c = 0; c < LAWN_COLS; c++) {
            cell_plant_t *p = &lawn[r][c];
            if (p->type == PLANT_NONE) continue;

            p->anim++;

            if (p->type == PLANT_SUNFLOWER) {
                if (++p->timer >= 330) { // ~11s
                    p->timer = 0;
                    int px = LAWN_LEFT + c * COL_W + 3;
                    int py = LAWN_TOP + r * ROW_H + 3;
                    spawn_sun(px, py - 4, py + 2, false);
                }
            } else if (p->type == PLANT_PEASHOOTER || p->type == PLANT_SNOWPEA || p->type == PLANT_REPEATER) {
                if (has_zombie_in_row) {
                    int rate = (p->type == PLANT_REPEATER) ? 35 : 45;
                    if (++p->timer >= rate) {
                        p->timer = 0;
                        for (int k = 0; k < MAX_PEAS; k++) {
                            if (!peas[k].active) {
                                peas[k].active = true;
                                peas[k].x = LAWN_LEFT + c * COL_W + 11;
                                peas[k].y = LAWN_TOP + r * ROW_H + 4;
                                peas[k].row = r;
                                peas[k].is_ice = (p->type == PLANT_SNOWPEA);
                                break;
                            }
                        }
                    }
                }
            } else if (p->type == PLANT_CHERRY) {
                if (--p->timer <= 0) {
                    add_floater(LAWN_LEFT + c * COL_W, LAWN_TOP + r * ROW_H - 4, "BOOM!");
                    for (int z = 0; z < MAX_ZOMBIES; z++) {
                        if (zombies[z].active) {
                            int zc = (zombies[z].x - LAWN_LEFT) / COL_W;
                            if (abs(zombies[z].row - r) <= 1 && abs(zc - c) <= 1) {
                                zombies[z].hp -= 1800;
                            }
                        }
                    }
                    p->type = PLANT_NONE;
                }
            } else if (p->type == PLANT_POTATO) {
                if (!p->armed) {
                    if (--p->timer <= 0) {
                        p->armed = true;
                        add_floater(LAWN_LEFT + c * COL_W, LAWN_TOP + r * ROW_H - 4, "ARMED!");
                    }
                }
            } else if (p->type == PLANT_CHOMPER) {
                if (p->chewing) {
                    if (++p->timer >= 360) { // 12s chewing
                        p->chewing = false;
                        p->timer = 0;
                    }
                } else {
                    // Check if zombie is in front to eat
                    for (int z = 0; z < MAX_ZOMBIES; z++) {
                        if (zombies[z].active && zombies[z].row == r) {
                            float zx = zombies[z].x;
                            float px = LAWN_LEFT + c * COL_W;
                            if (zx >= px && zx <= px + 22) {
                                zombies[z].hp = 0; // Chomp!
                                p->chewing = true;
                                p->timer = 0;
                                add_floater((int)px, LAWN_TOP + r * ROW_H - 4, "CHOMP!");
                                break;
                            }
                        }
                    }
                }
            } else if (p->type == PLANT_JALAPENO) {
                if (--p->timer <= 0) {
                    jalapeno_burn_row = r;
                    jalapeno_burn_timer = 20;
                    add_floater(LAWN_LEFT + c * COL_W, LAWN_TOP + r * ROW_H - 4, "HOT!");
                    for (int z = 0; z < MAX_ZOMBIES; z++) {
                        if (zombies[z].active && zombies[z].row == r) {
                            zombies[z].hp = 0; // Burned!
                        }
                    }
                    p->type = PLANT_NONE;
                }
            }
        }
    }

    if (jalapeno_burn_timer > 0) jalapeno_burn_timer--;

    // 6. Update Peas
    for (int k = 0; k < MAX_PEAS; k++) {
        if (!peas[k].active) continue;

        peas[k].x += 2.8f;

        if (peas[k].x > SCREEN_W) {
            peas[k].active = false;
            continue;
        }

        // Graves collision
        if (is_night_level(current_level)) {
            int col = (peas[k].x - LAWN_LEFT) / COL_W;
            if (col >= 0 && col < LAWN_COLS && lawn[peas[k].row][col].is_grave) {
                peas[k].active = false;
                continue;
            }
        }

        for (int z = 0; z < MAX_ZOMBIES; z++) {
            if (zombies[z].active && zombies[z].row == peas[k].row) {
                if (peas[k].x >= zombies[z].x && peas[k].x <= (zombies[z].x + 10)) {
                    zombies[z].hp -= 20;

                    // Newspaper Zombie newspaper break
                    if (zombies[z].type == ZOMBIE_NEWSPAPER && zombies[z].hp <= 140 && !zombies[z].angry) {
                        zombies[z].angry = true;
                        add_floater((int)zombies[z].x, LAWN_TOP + zombies[z].row * ROW_H - 4, "GRRR!");
                    }

                    if (peas[k].is_ice) {
                        zombies[z].slow_timer = 90;
                    }
                    peas[k].active = false;
                    break;
                }
            }
        }
    }

    // 7. Zombie Spawner Logic
    if (zombies_spawned < zombies_to_spawn) {
        if (--spawn_timer <= 0) {
            int base_speed = is_custom_map ? active_custom_map.spawn_speed : (240 - (current_level * 20));
            if (game_difficulty == DIFF_EASY) base_speed += 60;
            else if (game_difficulty == DIFF_HARD) base_speed -= 40;
            if (base_speed < 50) base_speed = 50;

            spawn_timer = base_speed + (rand() % 50);

            int ztype = ZOMBIE_NORMAL;
            int r = rand() % 100;

            if (current_level >= 2 && r < 35) ztype = ZOMBIE_CONE;
            if (current_level >= 3 && r >= 35 && r < 60) ztype = ZOMBIE_BUCKET;
            if (current_level >= 7 && r >= 60 && r < 80) ztype = ZOMBIE_FOOTBALL;
            if (current_level >= 8 && r >= 80) ztype = ZOMBIE_NEWSPAPER;
            if (current_level >= 4 && (rand() % 100) < 20) ztype = ZOMBIE_VAULTING;

            // Flag Wave
            if (zombies_spawned == (zombies_to_spawn * 3 / 4)) {
                ztype = ZOMBIE_FLAG;
                wave_banner_timer = 70;
            }
            spawn_zombie(ztype);
        }
    }

    // 8. Update Zombies
    for (int z = 0; z < MAX_ZOMBIES; z++) {
        if (!zombies[z].active) continue;

        entity_zombie_t *zb = &zombies[z];

        if (zb->hp <= 0) {
            zb->active = false;
            zombies_killed++;
            continue;
        }

        if (zb->slow_timer > 0) zb->slow_timer--;
        zb->anim++;

        // Base speeds
        float speed = 0.11f;
        if (zb->type == ZOMBIE_FLAG) speed = 0.18f;
        else if (zb->type == ZOMBIE_FOOTBALL) speed = 0.20f;
        else if (zb->type == ZOMBIE_NEWSPAPER) speed = zb->angry ? 0.32f : 0.08f;
        else if (zb->type == ZOMBIE_VAULTING && !zb->vaulted) speed = 0.22f;

        if (game_difficulty == DIFF_EASY) speed *= 0.85f;
        else if (game_difficulty == DIFF_HARD) speed *= 1.20f;
        if (zb->slow_timer > 0) speed *= 0.5f;

        // Eating / Pole vaulting / Potato Mine interaction
        int col = (zb->x - LAWN_LEFT) / COL_W;
        bool is_eating = false;

        if (col >= 0 && col < LAWN_COLS) {
            cell_plant_t *p = &lawn[zb->row][col];
            if (p->type != PLANT_NONE) {
                float plant_x = LAWN_LEFT + col * COL_W + 8;

                if (zb->x >= plant_x - 4 && zb->x <= plant_x + 10) {
                    // Potato Mine explosion!
                    if (p->type == PLANT_POTATO && p->armed) {
                        zb->hp = 0;
                        p->type = PLANT_NONE;
                        add_floater((int)plant_x, LAWN_TOP + zb->row * ROW_H - 4, "SPUDOW!");
                        continue;
                    }

                    // Pole Vaulting Zombie Jump!
                    if (zb->type == ZOMBIE_VAULTING && !zb->vaulted) {
                        zb->vaulted = true;
                        zb->x -= 22.0f; // Jump over plant!
                        add_floater((int)zb->x, LAWN_TOP + zb->row * ROW_H - 4, "VAULT!");
                        continue;
                    }

                    // Regular Eating
                    is_eating = true;
                    p->hp -= 2;
                    if (p->hp <= 0) p->type = PLANT_NONE;
                }
            }
        }

        if (!is_eating) {
            zb->x -= speed;
        }

        // Mowers trigger
        if (zb->x <= 18 && mowers[zb->row].active && !mowers[zb->row].triggered) {
            mowers[zb->row].triggered = true;
        }

        // Game Over
        if (zb->x <= 2) {
            game_state = STATE_GAMEOVER;
            return;
        }
    }

    // 9. Update Mowers
    for (int r = 0; r < LAWN_ROWS; r++) {
        if (mowers[r].active && mowers[r].triggered) {
            mowers[r].x += 3.5f;
            for (int z = 0; z < MAX_ZOMBIES; z++) {
                if (zombies[z].active && zombies[z].row == r && zombies[z].x <= (mowers[r].x + 12)) {
                    zombies[z].hp = 0;
                }
            }
            if (mowers[r].x > SCREEN_W) {
                mowers[r].active = false;
            }
        }
    }

    // 10. Update Floaters
    for (int i = 0; i < 6; i++) {
        if (floaters[i].timer > 0) {
            floaters[i].timer--;
            floaters[i].y -= 0.5f;
        }
    }

    // 11. Victory Condition
    if (zombies_spawned >= zombies_to_spawn && zombies_killed >= zombies_to_spawn) {
        if (!is_custom_map) {
            // Check newly unlocked plant
            newly_unlocked_plant = PLANT_NONE;
            int next_level = current_level + 1;

            if (current_level == 1 && !(unlocked_plants_mask & (1 << PLANT_SUNFLOWER))) newly_unlocked_plant = PLANT_SUNFLOWER;
            else if (current_level == 2 && !(unlocked_plants_mask & (1 << PLANT_CHERRY))) newly_unlocked_plant = PLANT_CHERRY;
            else if (current_level == 3 && !(unlocked_plants_mask & (1 << PLANT_WALLNUT))) newly_unlocked_plant = PLANT_WALLNUT;
            else if (current_level == 4 && !(unlocked_plants_mask & (1 << PLANT_POTATO))) newly_unlocked_plant = PLANT_POTATO;
            else if (current_level == 5 && !(unlocked_plants_mask & (1 << PLANT_SNOWPEA))) newly_unlocked_plant = PLANT_SNOWPEA;
            else if (current_level == 6 && !(unlocked_plants_mask & (1 << PLANT_CHOMPER))) newly_unlocked_plant = PLANT_CHOMPER;
            else if (current_level == 7 && !(unlocked_plants_mask & (1 << PLANT_REPEATER))) newly_unlocked_plant = PLANT_REPEATER;
            else if (current_level == 8 && !(unlocked_plants_mask & (1 << PLANT_JALAPENO))) newly_unlocked_plant = PLANT_JALAPENO;

            if (newly_unlocked_plant != PLANT_NONE) {
                unlocked_plants_mask |= (1 << newly_unlocked_plant);
            }

            if (current_level >= unlocked_level && unlocked_level < 10) {
                unlocked_level = next_level;
            }
            save_progress();

            if (newly_unlocked_plant != PLANT_NONE) {
                game_state = STATE_NEW_PLANT_UNLOCKED;
                return;
            }
        }
        game_state = STATE_VICTORY;
    }
}

// ----------------------------------------------------------------------------
// Render Game
// ----------------------------------------------------------------------------
static void render_game(uint16_t *buf) {
    bool is_night = is_night_level(current_level);
    uint16_t col_light = is_night ? COLOR_NIGHT_LIGHT : COLOR_LAWN_LIGHT;
    uint16_t col_dark  = is_night ? COLOR_NIGHT_DARK  : COLOR_LAWN_DARK;

    // 1. Draw Lawn
    for (int r = 0; r < LAWN_ROWS; r++) {
        bool active_lane = is_lane_active(current_level, r);

        for (int c = 0; c < LAWN_COLS; c++) {
            int tx = LAWN_LEFT + c * COL_W;
            int ty = LAWN_TOP + r * ROW_H;

            if (active_lane) {
                uint16_t tile_col = ((r + c) % 2 == 0) ? col_light : col_dark;
                pvz_fill_rect(buf, tx, ty, COL_W, ROW_H, tile_col);
                if (lawn[r][c].is_grave) {
                    pvz_draw_gravestone(buf, tx + 3, ty + 3);
                }
            } else {
                pvz_fill_rect(buf, tx, ty, COL_W, ROW_H, COLOR_DARK_BROWN);
                pvz_fill_rect(buf, tx, ty + ROW_H - 1, COL_W, 1, COLOR_BLACK);
            }
        }
    }

    // Jalapeno Fire Row Effect
    if (jalapeno_burn_timer > 0 && jalapeno_burn_row >= 0) {
        int fy = LAWN_TOP + jalapeno_burn_row * ROW_H;
        uint16_t fcol = (jalapeno_burn_timer % 2 == 0) ? COLOR_FIRE_ORANGE : COLOR_RED;
        pvz_fill_rect(buf, LAWN_LEFT, fy, LAWN_COLS * COL_W, ROW_H, fcol);
    }

    // House porch
    pvz_fill_rect(buf, 0, LAWN_TOP, LAWN_LEFT, LAWN_ROWS * ROW_H, is_night ? COLOR_DARK_GRAY : COLOR_BROWN);
    pvz_fill_rect(buf, LAWN_LEFT - 2, LAWN_TOP, 2, LAWN_ROWS * ROW_H, COLOR_BLACK);

    // Right entrance
    pvz_fill_rect(buf, LAWN_LEFT + LAWN_COLS * COL_W, LAWN_TOP, 10, LAWN_ROWS * ROW_H, is_night ? COLOR_NIGHT_DARK : COLOR_DARK_GREEN);

    // 2. Top HUD Bar
    pvz_fill_rect(buf, 0, 0, SCREEN_W, LAWN_TOP, COLOR_DARK_GRAY);
    pvz_fill_rect(buf, 0, LAWN_TOP - 1, SCREEN_W, 1, COLOR_BLACK);

    // Sun box
    pvz_fill_rect(buf, 2, 2, 24, 16, COLOR_BLACK);
    pvz_draw_sun(buf, 3, 4);
    char sun_str[8];
    snprintf(sun_str, sizeof(sun_str), "%d", player_sun);
    pvz_draw_text(buf, 13, 7, sun_str, COLOR_YELLOW);

    // Seed cards (Active Loadout)
    for (int i = 0; i < MAX_LOADOUT_SLOTS; i++) {
        int cx = 28 + i * 18;
        int cy = 2;
        plant_type_t ptype = (plant_type_t)active_loadout[i];
        int cost = (ptype == PLANT_SHOVEL) ? 0 : plant_costs[ptype];
        bool can_afford = (player_sun >= cost) && (seed_cooldown_timers[i] <= 0);

        uint16_t card_bg = can_afford ? COLOR_GRAY : COLOR_DARK_GRAY;
        if (selected_seed_slot == i) card_bg = COLOR_WHITE;
        pvz_fill_rect(buf, cx, cy, 16, 16, card_bg);
        pvz_draw_rect(buf, cx, cy, 16, 16, (selected_seed_slot == i) ? COLOR_YELLOW : COLOR_BLACK);

        if (ptype == PLANT_PEASHOOTER) pvz_draw_peashooter(buf, cx + 2, cy + 1, 0);
        else if (ptype == PLANT_SUNFLOWER) pvz_draw_sunflower(buf, cx + 2, cy + 1);
        else if (ptype == PLANT_CHERRY) pvz_draw_cherry(buf, cx + 2, cy + 2, false);
        else if (ptype == PLANT_WALLNUT) pvz_draw_wallnut(buf, cx + 3, cy + 1, 100);
        else if (ptype == PLANT_POTATO) pvz_draw_potatomine(buf, cx + 3, cy + 2, true);
        else if (ptype == PLANT_SNOWPEA) pvz_draw_peashooter(buf, cx + 2, cy + 1, 1);
        else if (ptype == PLANT_CHOMPER) pvz_draw_chomper(buf, cx + 2, cy + 2, false);
        else if (ptype == PLANT_REPEATER) pvz_draw_peashooter(buf, cx + 2, cy + 1, 2);
        else if (ptype == PLANT_JALAPENO) pvz_draw_jalapeno(buf, cx + 3, cy + 1);
        else if (ptype == PLANT_SHOVEL) pvz_draw_shovel(buf, cx + 3, cy + 3);

        if (seed_cooldown_timers[i] > 0) {
            int max_cd = (ptype == PLANT_SHOVEL) ? 10 : plant_cooldowns[ptype];
            int cd_h = (seed_cooldown_timers[i] * 14) / (max_cd > 0 ? max_cd : 1);
            pvz_fill_rect(buf, cx + 1, cy + 15 - cd_h, 14, cd_h, COLOR_BLACK);
        }

        if (ptype != PLANT_SHOVEL) {
            char cost_str[6];
            snprintf(cost_str, sizeof(cost_str), "%d", cost);
            pvz_draw_text(buf, cx + 1, cy + 10, cost_str, COLOR_WHITE);
        }
    }

    // Stage indicator
    char lvl_str[10];
    if (is_custom_map) {
        snprintf(lvl_str, sizeof(lvl_str), "MAP");
    } else {
        snprintf(lvl_str, sizeof(lvl_str), "S%d%s", current_level, is_night ? "N" : "");
    }
    pvz_draw_text(buf, SCREEN_W - 20, 6, lvl_str, is_night ? COLOR_ICE_BLUE : COLOR_LIME);

    // 3. Mowers
    for (int r = 0; r < LAWN_ROWS; r++) {
        if (mowers[r].active && is_lane_active(current_level, r)) {
            pvz_draw_mower(buf, (int)mowers[r].x, LAWN_TOP + r * ROW_H + 4);
        }
    }

    // 4. Plants
    for (int r = 0; r < LAWN_ROWS; r++) {
        if (!is_lane_active(current_level, r)) continue;

        for (int c = 0; c < LAWN_COLS; c++) {
            cell_plant_t *p = &lawn[r][c];
            if (p->type == PLANT_NONE) continue;

            int px = LAWN_LEFT + c * COL_W + 3;
            int py = LAWN_TOP + r * ROW_H + 3;

            if (p->type == PLANT_PEASHOOTER) pvz_draw_peashooter(buf, px, py, 0);
            else if (p->type == PLANT_SUNFLOWER) pvz_draw_sunflower(buf, px, py);
            else if (p->type == PLANT_CHERRY) {
                bool flash = (p->timer < 20) && ((p->anim / 3) % 2 == 0);
                pvz_draw_cherry(buf, px, py, flash);
            }
            else if (p->type == PLANT_WALLNUT) {
                int pct = (p->hp * 100) / p->max_hp;
                pvz_draw_wallnut(buf, px, py, pct);
            }
            else if (p->type == PLANT_POTATO) pvz_draw_potatomine(buf, px, py, p->armed);
            else if (p->type == PLANT_SNOWPEA) pvz_draw_peashooter(buf, px, py, 1);
            else if (p->type == PLANT_CHOMPER) pvz_draw_chomper(buf, px, py, p->chewing);
            else if (p->type == PLANT_REPEATER) pvz_draw_peashooter(buf, px, py, 2);
            else if (p->type == PLANT_JALAPENO) pvz_draw_jalapeno(buf, px, py);
        }
    }

    // 5. Peas
    for (int k = 0; k < MAX_PEAS; k++) {
        if (peas[k].active) {
            pvz_draw_pea(buf, (int)peas[k].x, peas[k].y, peas[k].is_ice);
        }
    }

    // 6. Zombies
    for (int z = 0; z < MAX_ZOMBIES; z++) {
        if (zombies[z].active) {
            entity_zombie_t *zb = &zombies[z];
            int zy = LAWN_TOP + zb->row * ROW_H + 2;
            pvz_draw_zombie(buf, (int)zb->x, zy, zb->type, (zb->slow_timer > 0), zb->anim,
                            (zb->type == ZOMBIE_NEWSPAPER ? zb->angry : zb->vaulted));

            if (zb->hp < zb->max_hp) {
                int bar_w = 10;
                int hp_w = (zb->hp * bar_w) / zb->max_hp;
                pvz_fill_rect(buf, (int)zb->x, zy - 4, bar_w, 2, COLOR_RED);
                pvz_fill_rect(buf, (int)zb->x, zy - 4, hp_w, 2, COLOR_LIME);
            }
        }
    }

    // 7. Suns
    for (int i = 0; i < MAX_SUNS; i++) {
        if (suns[i].active) {
            if (suns[i].timer > 100 || (suns[i].timer / 6) % 2 == 0) {
                pvz_draw_sun(buf, (int)suns[i].x, (int)suns[i].y);
            }
        }
    }

    // 8. Floaters
    for (int i = 0; i < 6; i++) {
        if (floaters[i].timer > 0) {
            pvz_draw_text(buf, floaters[i].x, floaters[i].y, floaters[i].text, COLOR_YELLOW);
        }
    }

    // 9. Cursor (VẼ LẠI Ô CHỌN MÀU VÀNG)
    if (cursor_row == -1) {
        int cx = 28 + cursor_col * 18;
        pvz_draw_rect(buf, cx - 1, 1, 18, 18, COLOR_YELLOW);
    } else {
        int cx = LAWN_LEFT + cursor_col * COL_W;
        int cy = LAWN_TOP + cursor_row * ROW_H;
        pvz_draw_rect(buf, cx, cy, COL_W, ROW_H, COLOR_YELLOW);

        if (selected_seed_slot >= 0 && selected_seed_slot < 5) {
            // Hiển thị ô màu trắng nhỏ bên trong nếu đang cầm thẻ bài
            pvz_draw_rect(buf, cx + 2, cy + 2, COL_W - 4, ROW_H - 4, COLOR_WHITE); 
        }
    }

    // ========================================================================
    // 10. SOLID BOTTOM PROGRESS BAR (Completely clears Y=116..127 to fix bleed)
    // ========================================================================
    pvz_fill_rect(buf, 0, 116, SCREEN_W, 12, COLOR_BLACK);
    pvz_draw_text(buf, 2, 119, "PROGRESS", COLOR_WHITE);

    int bar_x = 55;
    int bar_w = 95;
    pvz_fill_rect(buf, bar_x, 120, bar_w, 4, COLOR_DARK_GRAY);
    int prog_w = (zombies_killed * bar_w) / (zombies_to_spawn > 0 ? zombies_to_spawn : 1);
    if (prog_w > bar_w) prog_w = bar_w;
    pvz_fill_rect(buf, bar_x, 120, prog_w, 4, COLOR_LIME);
    pvz_draw_pixel(buf, bar_x + bar_w - 1, 119, COLOR_RED);

    // 11. Huge Wave Banner
    if (wave_banner_timer > 0) {
        wave_banner_timer--;
        pvz_fill_rect(buf, 10, 52, 140, 20, COLOR_DARK_RED);
        pvz_draw_rect(buf, 10, 52, 140, 20, COLOR_YELLOW);
        pvz_draw_text(buf, 26, 58, "HUGE WAVE INCOMING!", COLOR_YELLOW);
    }
}

// ----------------------------------------------------------------------------
// Render Main Menu, Seed Chooser & Sub-Screens
// ----------------------------------------------------------------------------
static void render_main_menu(uint16_t *buf) {
    pvz_fill_rect(buf, 0, 0, SCREEN_W, 60, COLOR_SKY);

    cloud_offset = (cloud_offset + 1) % 400;
    int c1 = (cloud_offset / 2) % (SCREEN_W + 40) - 20;
    int c2 = ((cloud_offset / 3) + 90) % (SCREEN_W + 50) - 25;
    pvz_fill_rect(buf, c1, 10, 25, 8, COLOR_WHITE);
    pvz_fill_rect(buf, c1 + 4, 6, 16, 5, COLOR_WHITE);
    pvz_fill_rect(buf, c2, 22, 30, 9, COLOR_WHITE);
    pvz_fill_rect(buf, c2 + 5, 17, 20, 6, COLOR_WHITE);

    pvz_draw_sun(buf, SCREEN_W - 22, 8);

    pvz_fill_rect(buf, 0, 50, SCREEN_W, 78, COLOR_LAWN_DARK);
    pvz_fill_rect(buf, 0, 45, SCREEN_W, 6, COLOR_LAWN_LIGHT);

    pvz_fill_rect(buf, 0, 32, 24, 25, COLOR_BROWN);
    pvz_fill_rect(buf, 0, 28, 28, 5, COLOR_DARK_BROWN);
    pvz_fill_rect(buf, 6, 38, 8, 8, COLOR_YELLOW);
    pvz_draw_rect(buf, 6, 38, 8, 8, COLOR_BLACK);

    pvz_draw_sunflower(buf, 30, 40);
    pvz_draw_peashooter(buf, 46, 40, 0);
    pvz_draw_zombie(buf, SCREEN_W - 20, 38, 1, false, cloud_offset, false);

    pvz_fill_rect(buf, 14, 4, 132, 19, COLOR_DARK_GRAY);
    pvz_draw_rect(buf, 14, 4, 132, 19, COLOR_LIME);
    pvz_draw_text(buf, 20, 10, "PLANTS vs ZOMBIES", COLOR_YELLOW);

    pvz_fill_rect(buf, 18, 55, 124, 70, COLOR_BLACK);
    pvz_draw_rect(buf, 18, 55, 124, 70, COLOR_WHITE);

    const char *diff_names[3] = {"EASY", "NORMAL", "HARD"};
    char diff_str[24];
    snprintf(diff_str, sizeof(diff_str), "DIFF: <%s>", diff_names[game_difficulty]);

    const char *menu_items[6] = {
        "CONTINUE",
        "NEW GAME",
        "LEVEL SELECT",
        diff_str,
        "HOW TO PLAY",
        "QUIT"
    };

    for (int i = 0; i < 6; i++) {
        int my = 58 + i * 11;
        bool is_sel = (menu_cursor == i);

        if (is_sel) {
            pvz_fill_rect(buf, 21, my - 1, 118, 10, COLOR_DARK_GREEN);
            pvz_draw_text(buf, 24, my, ">", COLOR_YELLOW);
            pvz_draw_text(buf, 32, my, menu_items[i], COLOR_YELLOW);
        } else {
            pvz_draw_text(buf, 32, my, menu_items[i], COLOR_WHITE);
        }
    }
}

static void render_choose_seeds(uint16_t *buf) {
    pvz_fill_rect(buf, 0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    pvz_draw_text(buf, 18, 6, "CHOOSE YOUR PLANTS", COLOR_YELLOW);

    // Selected tray (5 slots)
    pvz_draw_text(buf, 10, 20, "TRAY:", COLOR_WHITE);
    for (int i = 0; i < 5; i++) {
        int cx = 45 + i * 20;
        int cy = 18;
        pvz_fill_rect(buf, cx, cy, 18, 18, COLOR_DARK_GRAY);
        pvz_draw_rect(buf, cx, cy, 18, 18, COLOR_WHITE);

        plant_type_t pt = (plant_type_t)active_loadout[i];
        if (pt == PLANT_PEASHOOTER) pvz_draw_peashooter(buf, cx + 3, cy + 3, 0);
        else if (pt == PLANT_SUNFLOWER) pvz_draw_sunflower(buf, cx + 3, cy + 3);
        else if (pt == PLANT_CHERRY) pvz_draw_cherry(buf, cx + 3, cy + 3, false);
        else if (pt == PLANT_WALLNUT) pvz_draw_wallnut(buf, cx + 4, cy + 3, 100);
        else if (pt == PLANT_POTATO) pvz_draw_potatomine(buf, cx + 4, cy + 4, true);
        else if (pt == PLANT_SNOWPEA) pvz_draw_peashooter(buf, cx + 3, cy + 3, 1);
        else if (pt == PLANT_CHOMPER) pvz_draw_chomper(buf, cx + 3, cy + 3, false);
        else if (pt == PLANT_REPEATER) pvz_draw_peashooter(buf, cx + 3, cy + 3, 2);
        else if (pt == PLANT_JALAPENO) pvz_draw_jalapeno(buf, cx + 4, cy + 3);
    }

    // Grid of all unlocked plants (9 plants total)
    pvz_draw_text(buf, 10, 42, "ALL UNLOCKED:", COLOR_LIME);
    for (int p = 1; p < TOTAL_PLANTS; p++) {
        int gx = 10 + ((p - 1) % 5) * 28;
        int gy = 56 + ((p - 1) / 5) * 26;

        bool unlocked = (unlocked_plants_mask & (1 << p));
        bool is_sel = (seed_select_cursor == (p - 1));

        if (unlocked) {
            pvz_fill_rect(buf, gx, gy, 24, 22, is_sel ? COLOR_DARK_GREEN : COLOR_GRAY);
            pvz_draw_rect(buf, gx, gy, 24, 22, is_sel ? COLOR_YELLOW : COLOR_BLACK);

            if (p == PLANT_PEASHOOTER) pvz_draw_peashooter(buf, gx + 6, gy + 3, 0);
            else if (p == PLANT_SUNFLOWER) pvz_draw_sunflower(buf, gx + 6, gy + 3);
            else if (p == PLANT_CHERRY) pvz_draw_cherry(buf, gx + 6, gy + 3, false);
            else if (p == PLANT_WALLNUT) pvz_draw_wallnut(buf, gx + 7, gy + 3, 100);
            else if (p == PLANT_POTATO) pvz_draw_potatomine(buf, gx + 7, gy + 4, true);
            else if (p == PLANT_SNOWPEA) pvz_draw_peashooter(buf, gx + 6, gy + 3, 1);
            else if (p == PLANT_CHOMPER) pvz_draw_chomper(buf, gx + 6, gy + 3, false);
            else if (p == PLANT_REPEATER) pvz_draw_peashooter(buf, gx + 6, gy + 3, 2);
            else if (p == PLANT_JALAPENO) pvz_draw_jalapeno(buf, gx + 7, gy + 3);

            char cost_str[6];
            snprintf(cost_str, sizeof(cost_str), "%d", plant_costs[p]);
            pvz_draw_text(buf, gx + 3, gy + 15, cost_str, COLOR_YELLOW);
        } else {
            pvz_fill_rect(buf, gx, gy, 24, 22, COLOR_DARK_GRAY);
            pvz_draw_text(buf, gx + 5, gy + 8, "?", COLOR_GRAY);
        }
    }

    // Controls
    pvz_draw_text(buf, 10, 114, "A: EQUIP   C: PLAY!", COLOR_YELLOW);
}

static void render_level_select(uint16_t *buf) {
    pvz_fill_rect(buf, 0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);

    // Tab Header
    pvz_fill_rect(buf, 5, 4, 72, 14, (level_select_tab == 0) ? COLOR_DARK_GREEN : COLOR_DARK_GRAY);
    pvz_draw_text(buf, 9, 8, "CAMPAIGN", (level_select_tab == 0) ? COLOR_YELLOW : COLOR_GRAY);

    pvz_fill_rect(buf, 80, 4, 75, 14, (level_select_tab == 1) ? COLOR_DARK_GREEN : COLOR_DARK_GRAY);
    pvz_draw_text(buf, 84, 8, "CUSTOM SD", (level_select_tab == 1) ? COLOR_YELLOW : COLOR_GRAY);

    if (level_select_tab == 0) {
        // Campaign (10 Stages)
        const char *stage_names[10] = {
            "S1: DAY 1-1 (1-LANE)",
            "S2: DAY 1-2 (3-LANES)",
            "S3: DAY 1-3 (CONEHEAD)",
            "S4: DAY 1-4 (5-LANES)",
            "S5: DAY 1-5 (BUCKET)",
            "S6: NIGHT 2-1 (GRAVES)",
            "S7: NIGHT 2-2 (FOOTBALL)",
            "S8: NIGHT 2-3 (NEWSPAPER)",
            "S9: DAY MEGA HORDE",
            "S10: FINAL BOSS RUSH"
        };

        for (int i = 0; i < 5; i++) {
            int page_idx = (menu_cursor / 5) * 5 + i;
            if (page_idx >= 10) break;

            int y = 24 + i * 17;
            bool unlocked = ((page_idx + 1) <= unlocked_level);
            bool is_sel = (menu_cursor == page_idx);

            if (is_sel) {
                pvz_fill_rect(buf, 5, y - 1, 150, 15, COLOR_DARK_GREEN);
                pvz_draw_rect(buf, 5, y - 1, 150, 15, COLOR_YELLOW);
            }

            if (unlocked) {
                pvz_draw_text(buf, 8, y + 2, stage_names[page_idx], is_sel ? COLOR_YELLOW : COLOR_WHITE);
            } else {
                pvz_draw_text(buf, 8, y + 2, "[LOCKED]", COLOR_DARK_GRAY);
            }
        }
    } else {
        // Custom Maps from SD Card
        if (custom_maps_count == 0) {
            pvz_draw_text(buf, 16, 40, "NO MAPS FOUND IN", COLOR_RED);
            pvz_draw_text(buf, 10, 55, "/sd/roms/pvz/maps/", COLOR_WHITE);
            pvz_draw_text(buf, 12, 75, "Put .json map files", COLOR_YELLOW);
            pvz_draw_text(buf, 14, 90, "into SD to play!", COLOR_YELLOW);
        } else {
            for (int i = 0; i < 5; i++) {
                int map_idx = (menu_cursor / 5) * 5 + i;
                if (map_idx >= custom_maps_count) break;

                int y = 24 + i * 17;
                bool is_sel = (menu_cursor == map_idx);

                if (is_sel) {
                    pvz_fill_rect(buf, 5, y - 1, 150, 15, COLOR_DARK_GREEN);
                    pvz_draw_rect(buf, 5, y - 1, 150, 15, COLOR_YELLOW);
                }

                pvz_draw_text(buf, 8, y + 2, custom_maps[map_idx].name, is_sel ? COLOR_YELLOW : COLOR_WHITE);
            }
        }
    }

    pvz_draw_text(buf, 6, 115, "L/R: TAB  A: SELECT  B: BACK", COLOR_GRAY);
}

static void render_pause_menu(uint16_t *buf) {
    pvz_fill_rect(buf, 16, 18, 128, 92, COLOR_BLACK);
    pvz_draw_rect(buf, 16, 18, 128, 92, COLOR_YELLOW);
    pvz_draw_text(buf, 60, 24, "PAUSED", COLOR_YELLOW);

    const char *diff_names[3] = {"EASY", "NORMAL", "HARD"};
    char diff_str[24];
    snprintf(diff_str, sizeof(diff_str), "DIFF: <%s>", diff_names[game_difficulty]);

    const char *items[5] = {
        "RESUME",
        "RESTART STAGE",
        diff_str,
        "LEVEL SELECT",
        "MAIN MENU"
    };

    for (int i = 0; i < 5; i++) {
        int y = 38 + i * 14;
        bool is_sel = (pause_menu_cursor == i);

        if (is_sel) {
            pvz_fill_rect(buf, 20, y - 2, 120, 12, COLOR_DARK_GREEN);
            pvz_draw_text(buf, 24, y, ">", COLOR_YELLOW);
            pvz_draw_text(buf, 34, y, items[i], COLOR_YELLOW);
        } else {
            pvz_draw_text(buf, 34, y, items[i], COLOR_WHITE);
        }
    }
}

static void render_new_plant_unlocked(uint16_t *buf) {
    pvz_fill_rect(buf, 0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    pvz_draw_text(buf, 20, 15, "YOU GOT A NEW PLANT!", COLOR_YELLOW);

    int cx = 72;
    int cy = 40;
    pvz_fill_rect(buf, cx - 2, cy - 2, 22, 22, COLOR_DARK_GRAY);
    pvz_draw_rect(buf, cx - 2, cy - 2, 22, 22, COLOR_WHITE);

    if (newly_unlocked_plant == PLANT_SUNFLOWER) pvz_draw_sunflower(buf, cx + 3, cy + 3);
    else if (newly_unlocked_plant == PLANT_CHERRY) pvz_draw_cherry(buf, cx + 3, cy + 3, false);
    else if (newly_unlocked_plant == PLANT_WALLNUT) pvz_draw_wallnut(buf, cx + 4, cy + 3, 100);
    else if (newly_unlocked_plant == PLANT_POTATO) pvz_draw_potatomine(buf, cx + 4, cy + 4, true);
    else if (newly_unlocked_plant == PLANT_SNOWPEA) pvz_draw_peashooter(buf, cx + 3, cy + 3, 1);
    else if (newly_unlocked_plant == PLANT_CHOMPER) pvz_draw_chomper(buf, cx + 3, cy + 3, false);
    else if (newly_unlocked_plant == PLANT_REPEATER) pvz_draw_peashooter(buf, cx + 3, cy + 3, 2);
    else if (newly_unlocked_plant == PLANT_JALAPENO) pvz_draw_jalapeno(buf, cx + 4, cy + 3);

    pvz_draw_text(buf, 30, 72, plant_names[newly_unlocked_plant], COLOR_LIME);

    char cost_txt[20];
    snprintf(cost_txt, sizeof(cost_txt), "SUN COST: %d", plant_costs[newly_unlocked_plant]);
    pvz_draw_text(buf, 40, 88, cost_txt, COLOR_YELLOW);

    pvz_draw_text(buf, 24, 112, "PRESS A TO CONTINUE", COLOR_WHITE);
}

static void render_help(uint16_t *buf) {
    pvz_fill_rect(buf, 0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    pvz_draw_text(buf, 44, 8, "CONTROLS", COLOR_YELLOW);

    pvz_draw_text(buf, 8, 22, "JOYSTICK: SMOOTH MOVE", COLOR_WHITE);
    pvz_draw_text(buf, 8, 33, "A: PLANT / GET SUN", COLOR_WHITE);
    pvz_draw_text(buf, 8, 44, "B: CANCEL / AUTO GET SUN", COLOR_WHITE);
    pvz_draw_text(buf, 8, 55, "L/R: QUICK SELECT CARD", COLOR_WHITE);
    pvz_draw_text(buf, 8, 66, "SELECT: SHOVEL TOOL", COLOR_WHITE);
    pvz_draw_text(buf, 8, 77, "START: PAUSE MENU", COLOR_LIME);
    pvz_draw_text(buf, 8, 88, "MENU: SAVE / SYSTEM MENU", COLOR_YELLOW);

    pvz_draw_text(buf, 35, 110, "PRESS B TO RETURN", COLOR_YELLOW);
}

// ----------------------------------------------------------------------------
// Main Application Entry
// ----------------------------------------------------------------------------
void pvz_main(void) {
    const int width = SCREEN_W;
    const int height = SCREEN_H;

    g_display_buffer = heap_caps_malloc(width * height * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!g_display_buffer) {
        RG_LOGE("Failed to allocate display buffer!");
        rg_system_exit();
        return;
    }

    const rg_handlers_t handlers = {
        .loadState = &pvz_load_state,
        .saveState = &pvz_save_state,
        .screenshot = &pvz_screenshot,
        .reset = &pvz_reset,
    };
    rg_app_t *app = rg_system_reinit(32000, &handlers, NULL);

    load_progress();
    setup_level_plants(unlocked_level);
    scan_custom_maps();

    if (app && (app->bootFlags & RG_BOOT_RESUME)) {
        if (rg_emu_load_state(app->saveSlot)) {
            game_state = STATE_PLAYING;
        }
    }

    uint32_t last_joystick = 0;

    while (true) {
        uint32_t joystick = rg_input_read_gamepad();
        uint32_t just_pressed = joystick & ~last_joystick;
        last_joystick = joystick;

        // Button MENU: System Save / Load dialog
        if (just_pressed & RG_KEY_MENU) {
            rg_gui_game_menu();
            if (game_state == STATE_PLAYING) render_game(g_display_buffer);
            else if (game_state == STATE_PAUSED) {
                render_game(g_display_buffer);
                render_pause_menu(g_display_buffer);
            }
            else if (game_state == STATE_MAIN_MENU) render_main_menu(g_display_buffer);
            else if (game_state == STATE_LEVEL_SELECT) render_level_select(g_display_buffer);
            else if (game_state == STATE_CHOOSE_SEEDS) render_choose_seeds(g_display_buffer);
            else if (game_state == STATE_HELP) render_help(g_display_buffer);
            rg_display_write_rect(0, 0, width, height, width, g_display_buffer, 0);
            continue;
        }

        // State Machine
        if (game_state == STATE_MAIN_MENU) {
            if (just_pressed & RG_KEY_UP) {
                if (--menu_cursor < 0) menu_cursor = 5;
            }
            if (just_pressed & RG_KEY_DOWN) {
                menu_cursor = (menu_cursor + 1) % 6;
            }

            if (menu_cursor == 3) {
                if (just_pressed & (RG_KEY_LEFT | RG_KEY_B)) {
                    if (game_difficulty <= 0) game_difficulty = DIFF_HARD;
                    else game_difficulty--;
                    save_progress();
                }
                if (just_pressed & (RG_KEY_RIGHT | RG_KEY_A)) {
                    game_difficulty = (difficulty_t)((game_difficulty + 1) % 3);
                    save_progress();
                }
            } else if (just_pressed & (RG_KEY_A | RG_KEY_START)) {
                if (menu_cursor == 0) { // Continue
                    setup_level_plants(unlocked_level);
                    game_state = STATE_CHOOSE_SEEDS;
                } else if (menu_cursor == 1) { // New Game
                    unlocked_plants_mask = (1 << PLANT_PEASHOOTER);
                    setup_level_plants(1);
                    reset_level(1);
                } else if (menu_cursor == 2) { // Level Select
                    menu_cursor = 0;
                    scan_custom_maps();
                    game_state = STATE_LEVEL_SELECT;
                } else if (menu_cursor == 4) { // How to Play
                    game_state = STATE_HELP;
                } else if (menu_cursor == 5) { // Quit
                    break;
                }
            }
            render_main_menu(g_display_buffer);

        } else if (game_state == STATE_CHOOSE_SEEDS) {
            // Seed Selection before match
            
            // Tìm vị trí cây cuối cùng đã được mở khóa
            int max_cursor = 0;
            for (int p = 1; p < TOTAL_PLANTS; p++) {
                if (unlocked_plants_mask & (1 << p)) max_cursor = p - 1;
            }

            // Chuyển đổi index thành tọa độ lưới 5 cột
            int row = seed_select_cursor / 5;
            int col = seed_select_cursor % 5;

            // Xử lý di chuyển lưới chặt chẽ
            if (just_pressed & RG_KEY_LEFT)  if (col > 0) col--;
            if (just_pressed & RG_KEY_RIGHT) if (col < 4) col++;
            if (just_pressed & RG_KEY_UP)    if (row > 0) row--;
            if (just_pressed & RG_KEY_DOWN)  if (row < 1) row++; // Game chỉ có 2 hàng cây

            int new_cursor = row * 5 + col;
            
            // Chỉ cho phép con trỏ di chuyển nếu ô đó đã mở khóa
            if (new_cursor <= max_cursor) {
                seed_select_cursor = new_cursor;
            }

            if (just_pressed & RG_KEY_A) {
                int plant_id = seed_select_cursor + 1;
                if (unlocked_plants_mask & (1 << plant_id)) {
                    // Check if already in loadout
                    int existing = -1;
                    for (int i = 0; i < 5; i++) {
                        if (active_loadout[i] == plant_id) {
                            existing = i;
                            break;
                        }
                    }
                    if (existing >= 0) {
                        active_loadout[existing] = PLANT_NONE;
                    } else {
                        // Put in first empty slot
                        for (int i = 0; i < 5; i++) {
                            if (active_loadout[i] == PLANT_NONE) {
                                active_loadout[i] = plant_id;
                                break;
                            }
                        }
                    }
                }
            }

            if (just_pressed & RG_KEY_START) {
                // Đảm bảo có ít nhất 1 cây tấn công/phòng thủ
                if (active_loadout[0] == PLANT_NONE) active_loadout[0] = PLANT_PEASHOOTER;
                
                if (is_custom_map) {
                    start_custom_map(&active_custom_map);
                } else {
                    reset_level(current_level); // Sửa 'unlocked_level' thành 'current_level'
                }
            }

            if (just_pressed & RG_KEY_B) {
                game_state = STATE_MAIN_MENU;
            }
            render_choose_seeds(g_display_buffer);

        } else if (game_state == STATE_LEVEL_SELECT) {
            if (just_pressed & (RG_KEY_L | RG_KEY_R)) {
                level_select_tab = 1 - level_select_tab;
                menu_cursor = 0;
            }

            int max_items = (level_select_tab == 0) ? 10 : custom_maps_count;
            if (max_items > 0) {
                if (just_pressed & RG_KEY_UP) {
                    if (--menu_cursor < 0) menu_cursor = max_items - 1;
                }
                if (just_pressed & RG_KEY_DOWN) {
                    menu_cursor = (menu_cursor + 1) % max_items;
                }

                if (just_pressed & RG_KEY_A) {
                    if (level_select_tab == 0) {
                        int sel_level = menu_cursor + 1;
                        if (sel_level <= unlocked_level) {
                            setup_level_plants(sel_level);
                            game_state = STATE_CHOOSE_SEEDS;
                        }
                    } else {
                        if (custom_maps_count > 0) {
                            start_custom_map(&custom_maps[menu_cursor]);
                        }
                    }
                }
            }

            if (just_pressed & (RG_KEY_B | RG_KEY_SELECT)) {
                menu_cursor = 0;
                game_state = STATE_MAIN_MENU;
            }
            render_level_select(g_display_buffer);

        } else if (game_state == STATE_HELP) {
            if (just_pressed & (RG_KEY_A | RG_KEY_B | RG_KEY_START | RG_KEY_SELECT)) {
                game_state = STATE_MAIN_MENU;
            }
            render_help(g_display_buffer);

        } else if (game_state == STATE_PAUSED) {
            if (just_pressed & RG_KEY_UP) {
                if (--pause_menu_cursor < 0) pause_menu_cursor = 4;
            }
            if (just_pressed & RG_KEY_DOWN) {
                pause_menu_cursor = (pause_menu_cursor + 1) % 5;
            }

            if (pause_menu_cursor == 2) {
                if (just_pressed & RG_KEY_LEFT) {
                    if (game_difficulty <= 0) game_difficulty = DIFF_HARD;
                    else game_difficulty--;
                    save_progress();
                }
                if (just_pressed & (RG_KEY_RIGHT | RG_KEY_A)) {
                    game_difficulty = (difficulty_t)((game_difficulty + 1) % 3);
                    save_progress();
                }
            } else if (just_pressed & (RG_KEY_A | RG_KEY_START)) {
                if (pause_menu_cursor == 0) { // Resume
                    game_state = STATE_PLAYING;
                } else if (pause_menu_cursor == 1) { // Restart
                    if (is_custom_map) start_custom_map(&active_custom_map);
                    else reset_level(current_level);
                } else if (pause_menu_cursor == 3) { // Level Select
                    menu_cursor = 0;
                    game_state = STATE_LEVEL_SELECT;
                } else if (pause_menu_cursor == 4) { // Main Menu
                    menu_cursor = 0;
                    game_state = STATE_MAIN_MENU;
                }
            }

            if (just_pressed & RG_KEY_B) {
                game_state = STATE_PLAYING;
            }

            render_game(g_display_buffer);
            render_pause_menu(g_display_buffer);

        } else if (game_state == STATE_PLAYING) {
            update_game(joystick, just_pressed);
            render_game(g_display_buffer);

        } else if (game_state == STATE_NEW_PLANT_UNLOCKED) {
            render_new_plant_unlocked(g_display_buffer);
            if (just_pressed & (RG_KEY_A | RG_KEY_START)) {
                game_state = STATE_VICTORY;
            }

        } else if (game_state == STATE_GAMEOVER) {
            pvz_fill_rect(g_display_buffer, 0, 0, width, height, COLOR_BLACK);
            pvz_draw_text(g_display_buffer, 25, 25, "THE ZOMBIES ATE", COLOR_RED);
            pvz_draw_text(g_display_buffer, 36, 40, "YOUR BRAINS!", COLOR_RED);

            pvz_draw_zombie(g_display_buffer, 75, 60, 0, false, 0, false);

            pvz_draw_text(g_display_buffer, 32, 90, "PRESS A TO RETRY", COLOR_WHITE);
            pvz_draw_text(g_display_buffer, 34, 105, "B: QUIT TO MENU", COLOR_GRAY);

            if (just_pressed & RG_KEY_A) {
                if (is_custom_map) start_custom_map(&active_custom_map);
                else reset_level(current_level);
            }
            if (just_pressed & RG_KEY_B) {
                menu_cursor = 0;
                game_state = STATE_MAIN_MENU;
            }

        } else if (game_state == STATE_VICTORY) {
            pvz_fill_rect(g_display_buffer, 0, 0, width, height, COLOR_BLACK);
            pvz_draw_text(g_display_buffer, 38, 20, "STAGE CLEAR!", COLOR_YELLOW);
            pvz_draw_text(g_display_buffer, 20, 35, "YOU SAVED YOUR LAWN!", COLOR_LIME);

            pvz_draw_sun(g_display_buffer, 75, 55);

            if (!is_custom_map && current_level < 10) {
                pvz_draw_text(g_display_buffer, 26, 85, "PRESS A: NEXT STAGE", COLOR_WHITE);
                pvz_draw_text(g_display_buffer, 34, 100, "B: QUIT TO MENU", COLOR_GRAY);
                if (just_pressed & RG_KEY_A) {
                    setup_level_plants(current_level + 1);
                    game_state = STATE_CHOOSE_SEEDS;
                }
            } else {
                pvz_draw_text(g_display_buffer, 28, 85, "YOU BEAT THE GAME!", COLOR_ICE_BLUE);
                pvz_draw_text(g_display_buffer, 32, 100, "PRESS A TO REPLAY", COLOR_WHITE);
                if (just_pressed & RG_KEY_A) {
                    menu_cursor = 0;
                    game_state = STATE_MAIN_MENU;
                }
            }

            if (just_pressed & RG_KEY_B) {
                menu_cursor = 0;
                game_state = STATE_MAIN_MENU;
            }
        }

        rg_display_write_rect(0, 0, width, height, width, g_display_buffer, 0);
        rg_task_delay(33);
    }

    if (g_display_buffer) {
        free(g_display_buffer);
        g_display_buffer = NULL;
    }
    rg_system_exit();
}
