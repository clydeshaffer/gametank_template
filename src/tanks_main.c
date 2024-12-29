#include "gametank.h"
#include "drawing_funcs.h"
#include "input.h"
#include "dynawave.h"
#include "music.h"
#include "instruments.h"
#include "gen/assets/gfx.h"
#include "gen/assets/music.h"

typedef struct {
    char lsb, msb;
} twobytes;

typedef union {
    unsigned int i;
    twobytes b;
} coordinate;

int sine_wave[256] = {
    128,131,134,137,141,144,147,150,153,156,159,162,165,168,171,174,
    177,180,183,186,188,191,194,196,199,202,204,207,209,212,214,216,
    219,221,223,225,227,229,231,233,234,236,238,239,241,242,244,245,
    246,247,249,250,250,251,252,253,254,254,255,255,255,256,256,256,
    256,256,256,256,255,255,255,254,254,253,252,251,250,250,249,247,
    246,245,244,242,241,239,238,236,234,233,231,229,227,225,223,221,
    219,216,214,212,209,207,204,202,199,196,194,191,188,186,183,180,
    177,174,171,168,165,162,159,156,153,150,147,144,141,137,134,131,
    128,125,122,119,115,112,109,106,103,100,97,94,91,88,85,82,
    79,76,73,70,68,65,62,60,57,54,52,49,47,44,42,40,
    37,35,33,31,29,27,25,23,22,20,18,17,15,14,12,11,
    10,9,7,6,6,5,4,3,2,2,1,1,1,0,0,0,
    0,0,0,0,1,1,1,2,2,3,4,5,6,6,7,9,
    10,11,12,14,15,17,18,20,22,23,25,27,29,31,33,35,
    37,40,42,44,47,49,52,54,57,60,62,65,68,70,73,76,
    79,82,85,88,91,94,97,100,103,106,109,112,115,119,122,125
};

#define BULLET_POOL_SIZE 8
#define TANK_MAG_SIZE 3
#define TANK_CD_FRAMES 192

#define TANK_SOUND_AMP 3

unsigned char global_tick;

coordinate tank_angle[2];
coordinate tank_x[2];
coordinate tank_y[2];
unsigned char tank_hp[2];
unsigned char tank_ammo[2];
unsigned char tank_reloading[2];
unsigned char tank_victories[2];

coordinate bullet_x[BULLET_POOL_SIZE];
coordinate bullet_y[BULLET_POOL_SIZE];
int bullet_vx[BULLET_POOL_SIZE];
int bullet_vy[BULLET_POOL_SIZE];
unsigned char bullet_life[BULLET_POOL_SIZE];
unsigned char next_bullet = 0;

void draw_tank(char num) {
    char flippage, tank_angle_frame;
    if(tank_hp[num] & 128) {
        if(tank_hp[num] < 188) {
            draw_sprite_frame(&ASSET__gfx__exlposion_json,
            tank_x[num].b.msb, tank_y[num].b.msb, 
            (tank_hp[num] & 127) >> 2
            , 0, 2);
        }
    } else {
        flippage = 0;
        if(tank_angle[num].b.msb <= 64) {
            tank_angle_frame = tank_angle[num].b.msb >> 3;
        } else if(tank_angle[num].b.msb <= 128) {
            tank_angle_frame = (128 - tank_angle[num].b.msb) >> 3;
            flippage = SPRITE_FLIP_Y;
        } else if(tank_angle[num].b.msb <= 192) {
            tank_angle_frame = (tank_angle[num].b.msb - 129) >> 3;
            flippage = SPRITE_FLIP_BOTH;
        } else {
            tank_angle_frame = (256 - tank_angle[num].b.msb) >> 3;
            flippage = SPRITE_FLIP_X;
        }
        tank_angle_frame += num * 9;
        draw_sprite_frame(&ASSET__gfx__green_tank_json,
            tank_x[num].b.msb, tank_y[num].b.msb, tank_angle_frame, flippage, 0);
    }
}

void update_tank(char num, int inputs, int last_inputs) {
    char amp_out = 0;
    char note_out = 0;
    if(tank_hp[num] == 0) {
        ++tank_victories[1-num];
        tank_hp[num] = 128;
        play_sound_effect(&ASSET__music__explode_bin, 4);
        global_tick = 0;
    }
    if(tank_hp[num] & 128) {
        if(tank_hp[num] < 192) {
            ++tank_hp[num];
        }
    } else {
        if(inputs & INPUT_MASK_LEFT) {
            tank_angle[num].b.msb += 1;
        }

        if(inputs & INPUT_MASK_RIGHT) {
            tank_angle[num].b.msb -= 1;
        }

        if(inputs & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT)) {
            note_out = (num << 2) + 20 + ((global_tick & 8) >> 2);
            amp_out = TANK_SOUND_AMP;
        }

        if(inputs & INPUT_MASK_UP) {
            tank_y[num].i -= (sine_wave[(tank_angle[num].b.msb + 64) % 256] - 128);
            tank_x[num].i += (sine_wave[(tank_angle[num].b.msb + 128) % 256] - 128);
            note_out = (num << 2) + 20 + ((global_tick & 4) >> 2);
            amp_out = TANK_SOUND_AMP;
        } else if (inputs & INPUT_MASK_DOWN) {
            tank_y[num].i += (sine_wave[(tank_angle[num].b.msb + 64) % 256] - 128);
            tank_x[num].i -= (sine_wave[(tank_angle[num].b.msb + 128) % 256] - 128);
            note_out = (num << 2) + 21 + ((global_tick & 4) >> 2);
            amp_out = TANK_SOUND_AMP;
            
        }

        if(tank_x[num].b.msb & 128) {
            tank_x[num].b.msb &= 127;
        }

        if(tank_y[num].b.msb & 128) {
            tank_y[num].b.msb &= 127;
        }

        if((tank_reloading[num] == 0)) {
            if(inputs & ~last_inputs & INPUT_MASK_A) {
                bullet_life[next_bullet] = 255;
                bullet_vy[next_bullet] = -(sine_wave[(tank_angle[num].b.msb + 64) % 256] - 128) * 4;
                bullet_vx[next_bullet] = (sine_wave[(tank_angle[num].b.msb + 128) % 256] - 128) * 4;
                bullet_x[next_bullet].i = tank_x[num].i + (bullet_vx[next_bullet] * 4);
                bullet_y[next_bullet].i = tank_y[num].i + (bullet_vy[next_bullet] * 4);
                next_bullet = (next_bullet+1)%BULLET_POOL_SIZE;
                play_sound_effect(&ASSET__music__shot_bin, 2);
                --tank_ammo[num];
                if(tank_ammo[num] == 0) {
                    tank_reloading[num] = TANK_CD_FRAMES;
                    tank_ammo[num] = TANK_MAG_SIZE;
                }
            }
        } else {
            --tank_reloading[num];
            if(!tank_reloading[num]) {
                play_sound_effect(&ASSET__music__reload_bin, 1);
            }
        }

        if(tank_hp[num] == 1) {
            if((global_tick & 0b00001100) == 0) {
                note_out = 64;
                amp_out = TANK_SOUND_AMP;
            } else if((global_tick & 0b00001100) == 4) {
                note_out = 59;
                amp_out = TANK_SOUND_AMP;
            }
        }
    }
    set_note(num << 2, note_out);
    set_audio_param(AMPLITUDE+(num << 2) + 3, amp_out + sine_offset);
    
    if(amp_out) amp_out += 2;
    set_audio_param(AMPLITUDE+(num << 2) + 0, amp_out + sine_offset);
    set_audio_param(AMPLITUDE+(num << 2) + 1, amp_out + sine_offset);
    set_audio_param(AMPLITUDE+(num << 2) + 2, amp_out + sine_offset);
    
}

void init_tanks() {
    char i;
    tank_angle[0].b.msb = 128;
    tank_angle[0].b.lsb = 0;
    tank_x[0].i = 8196;
    tank_y[0].i = 8196;
    tank_hp[0] = 10;
    tank_reloading[0] = 0;
    tank_ammo[0] = TANK_MAG_SIZE;

    tank_angle[1].b.msb = 0;
    tank_angle[1].b.lsb = 0;
    tank_x[1].i = 24580;
    tank_y[1].i = 24580;
    tank_hp[1] = 10;
    tank_reloading[1] = 0;
    tank_ammo[1] = TANK_MAG_SIZE;

    for(i = 0; i < BULLET_POOL_SIZE; ++i) {
        bullet_life[i] = 0;
    }
    next_bullet = 0;
}

#define HITBOX_MANH_RADIUS 6
char check_tank_hit(char num, char x, char y) {
    char dx = tank_x[num].b.msb - x;
    char dy = tank_y[num].b.msb - y;
    if(tank_hp[num] & 128) {
        return 0;
    }
    if(dx & 128) dx = -dx;
    if(dy & 128) dy = -dy;
    return (dx < HITBOX_MANH_RADIUS) && (dy < HITBOX_MANH_RADIUS);
}

unsigned char hp_colors[10] = {91, 91, 91, 61, 61, 29, 29, 252, 252, 252};

void draw_hp_bar(char num, char x) {
    char hp = tank_hp[num];
    char sets;
    if(tank_hp[num] & 128) return;
    hp = hp << 2;
    if((hp != 4) || global_tick & 8) {
        draw_box(x, 56 - hp, 4, hp, hp_colors[tank_hp[num]-1]);
    }

    if(tank_reloading[num]) {
        draw_box(x, 57, 4, tank_reloading[num] >> 4, 7);
    } else {
        draw_sprite(x, 57, 4, tank_ammo[num] << 2, 0, 0, 3);
    }

    hp = tank_victories[num];
    sets = 69;
    while(hp >= 5) {
        draw_sprite(x - (num * 5), sets, 9, 4, 0, 16, 3);
        hp -= 5;
        sets += 4;
    }
    if(hp) {
        hp -= 1;
        draw_sprite(x - (num * 5), sets, 9, 4, 0, 32 + (hp << 2), 3);
    }
}

void draw_countdown(char count) {
    if(count == 0) return;
    draw_sprite_frame(&ASSET__gfx__countdown_json, 64, 64, (256 - count) >> 6, 0, 4);
}

extern char auto_tick_music;

int tanks_main () {
    char i;
    char tank_angle_frame;
    char intro_playing = 255;
    char count_down;

    load_spritesheet(&ASSET__gfx__green_tank_bmp, 0);
    load_spritesheet(&ASSET__gfx__ground_bmp, 1);
    load_spritesheet(&ASSET__gfx__exlposion_bmp, 2);
    load_spritesheet(&ASSET__gfx__title_bmp, 3);
    load_spritesheet(&ASSET__gfx__countdown_bmp, 4);

    init_tanks();
    tank_victories[0] = 0;
    tank_victories[1] = 0;
    count_down = 63;
    stop_music();
    play_song(&ASSET__music__tank_intro_mid, REPEAT_NONE);

    while (1) {             
        update_inputs();
        draw_sprite(0, 0, 127,127, 0, 0, 1);
        

        draw_tank(0);        
        draw_tank(1);
        
        draw_hp_bar(0, 1);
        draw_hp_bar(1, 123);

        for(i = 0; i < BULLET_POOL_SIZE; ++i) {
            if(bullet_life[i] > 0) {
                draw_box(bullet_x[i].b.msb - 1, bullet_y[i].b.msb - 1, 3, 3, 60);

                bullet_x[i].i += bullet_vx[i];
                bullet_y[i].i += bullet_vy[i];
                --bullet_life[i];
                if(bullet_x[i].b.msb & 128) {
                    bullet_x[i].b.msb &= 127;
                } else if(bullet_y[i].b.msb & 128) {
                    bullet_y[i].b.msb &= 127;
                } else if(check_tank_hit(0, bullet_x[i].b.msb, bullet_y[i].b.msb)){
                    bullet_life[i] = 0;
                    --tank_hp[0];
                    play_sound_effect(&ASSET__music__hit_bin, 3);
                } else if(check_tank_hit(1, bullet_x[i].b.msb, bullet_y[i].b.msb)) {
                    bullet_life[i] = 0;
                    --tank_hp[1];
                    play_sound_effect(&ASSET__music__hit_bin, 3);
                }
            }
        }
        
        if(intro_playing) {
            --intro_playing;
            draw_sprite(16, 0, 111, 127, 16, 0, 3);
        } else {
            if(count_down) {
                if((count_down & 62) == 62) {
                    play_sound_effect(&ASSET__music__ping1_bin, 1);
                }
                draw_countdown(count_down);
                --count_down;
            }
            if(count_down < 64) {
                update_tank(0, player1_buttons, player1_old_buttons);
                update_tank(1, player2_buttons, player2_old_buttons);
            } else if (count_down < 68) {
                play_sound_effect(&ASSET__music__ping2_bin, 2);
            }
        }
        //flush_audio_params();

        if(global_tick == 255) {
            if((tank_hp[0] | tank_hp[1]) & 128) {
                init_tanks();
                count_down = 255;
            }
        }

        clear_border(0);

        await_draw_queue();
        sleep(1);
        flip_pages();
        //tick_music();
        ++global_tick;
    }

  return (0);                                     //  We should never get here!
}