#include "common.h"
#include "splash_tiles.h"
#include "splash_tilemap.h"
#include "splash_sprites.h"
#include "splash_title_tiles.h"
#include "splash_title_map.h"

#include <gb/cgb.h>
#include <gb/gb.h>
#include <gbdk/font.h>

typedef enum Level0State {
    LEVEL0STATE_INTRO_CHARACTERS,
    LEVEL0STATE_INTRO_SCROLL_LOGO,
    LEVEL0STATE_IDLE,
    LEVEL0STATE_CODE_SELECT
} Level0State;

#define TOP_TILE_TO_SCANLINE( tile ) ( tile * 8 - 1)
// Parallax at end-of tile 6, 10, 14, (maybe) 16, 17
#define NUM_PARALLAX 6
const uint16_t parallax_scanlines[ NUM_PARALLAX ] = {
    0,
    TOP_TILE_TO_SCANLINE( 8 ) + 1,
    TOP_TILE_TO_SCANLINE( 11 ),
    TOP_TILE_TO_SCANLINE( 14 ),
    TOP_TILE_TO_SCANLINE( 15 ),
    TOP_TILE_TO_SCANLINE( 17 )
};

uint16_t parallax_counters[ NUM_PARALLAX ] = {
    0,
    0,
    0,
    0,
    0,
    0
};

const uint16_t parallax_counter_thresholds[ NUM_PARALLAX ] = {
    10,
    8,
    6,
    4,
    3,
    2
};

uint16_t sprite_counters[ 1 ] = {
    0
};

// Sprites are assumed to be 2x2
typedef struct SpriteCtx {
    uint8_t tile_offset;
    uint8_t x, y;
    uint8_t timer;
    uint8_t frame_threshold;
    uint8_t frame;
} SpriteCtx;

static struct Level0Context {
    Level0State state;
    uint16_t scroll_position;
    uint16_t frame_cnt;
    uint16_t parallax_scroll_offsets[ NUM_PARALLAX ];
    uint8_t current_parallax;

    SpriteCtx sprites[ 2 ];
} level_ctx;

void lcdc_isr(void) NONBANKED {
    move_bkg( level_ctx.parallax_scroll_offsets[ level_ctx.current_parallax ], 0 );
    if ( ++level_ctx.current_parallax == NUM_PARALLAX ) {
        level_ctx.current_parallax = 0;
    }
    LYC_REG = parallax_scanlines[ level_ctx.current_parallax ];
}

#define SpriteProperty( priority, vflip, hflip, bwPalette, bank, colourPalette ) \
    (uint8_t) ( \
        ( ( priority ? 0x00 : 0x01 ) << 7 ) | \
        ( ( vflip ? 0x01 : 0x0 ) << 6 ) | \
        ( ( hflip ? 0x01 : 0x0 ) << 5 ) | \
        ( ( bwPalette & 0x01 ) << 4 ) | \
        ( ( bank & 0x01 ) << 3 ) | \
        ( colourPalette & 0x07 ) \
    )

static inline void set_sprite_position( const SpriteCtx *sprite ) {
    const uint8_t sprite_offset = sprite->tile_offset;
    const uint8_t x = sprite->x;
    const uint8_t y = sprite->y;

    shadow_OAM[sprite_offset + 0].x = x;
    shadow_OAM[sprite_offset + 0].y = y;
    shadow_OAM[sprite_offset + 1].x = x + 8;
    shadow_OAM[sprite_offset + 1].y = y;
    shadow_OAM[sprite_offset + 2].x = x;
    shadow_OAM[sprite_offset + 2].y = y + 8;
    shadow_OAM[sprite_offset + 3].x = x + 8;
    shadow_OAM[sprite_offset + 3].y = y + 8;
}

static inline void move_sprite_abs( SpriteCtx *sprite, uint8_t x, uint8_t y ) {
    sprite->x = x;
    sprite->y = y;
    set_sprite_position( sprite );
}

static inline void move_sprite_rel( SpriteCtx *sprite, int8_t x, int8_t y ) {
    sprite->x += x;
    sprite->y += y;
    set_sprite_position( sprite );
}

static inline void swap_sprite_frame( SpriteCtx *sprite ) {
    const uint8_t sprite_offset = sprite->tile_offset;
    shadow_OAM[sprite_offset + 0].tile ^= 0x80;
    shadow_OAM[sprite_offset + 1].tile ^= 0x80;
    shadow_OAM[sprite_offset + 2].tile ^= 0x80;
    shadow_OAM[sprite_offset + 3].tile ^= 0x80;

    sprite->frame ^= 0x01;
    if ( sprite->frame == 0 ) {
        move_sprite_rel( sprite, 0, -2 );
    }
    else {
        move_sprite_rel( sprite, 0, 2 );
    }
}

static inline void sprite_animation_update( SpriteCtx *sprite ) {
    if ( --sprite->timer == 0 ) {
        sprite->timer = sprite->frame_threshold;
        swap_sprite_frame( sprite );
    }
}

static void move_control_text( uint8_t x ) {
    for( uint8_t i = 8; i < 18; ++i ) {
        const uint16_t x_adj = x + ( ( i - 8 ) * 8 );
        move_sprite( i, x_adj > 168 ? 0 : x_adj , 82 );
    }

    for( uint8_t j = 18; j < 26; ++j ) {
        const uint16_t x_adj = x + ( ( j - 17 ) * 8 );
        move_sprite( j, x_adj > 168 ? 0 : x_adj , 94 );
    }
}

static inline void show_control_text() {
    move_sprite( 8,  50 + 8 * 0, 75 );
    move_sprite( 9,  50 + 8 * 1, 75 );
    move_sprite( 10, 50 + 8 * 2, 75 );
    move_sprite( 11, 50 + 8 * 3, 75 );
    move_sprite( 12, 50 + 8 * 4, 75 );
    move_sprite( 13, 50 + 8 * 5, 75 );
    move_sprite( 14, 50 + 8 * 6, 75 );
    move_sprite( 15, 50 + 8 * 7, 75 );
    move_sprite( 16, 50 + 8 * 8, 75 );
    move_sprite( 17, 50 + 8 * 9, 75 );

    move_sprite( 18, 58 + 8 * 0, 75 + 12 );
    move_sprite( 19, 58 + 8 * 1, 75 + 12 );
    move_sprite( 20, 58 + 8 * 2, 75 + 12 );
    move_sprite( 21, 58 + 8 * 3, 75 + 12 );
    move_sprite( 22, 58 + 8 * 4, 75 + 12 );
    move_sprite( 23, 58 + 8 * 5, 75 + 12 );
    move_sprite( 24, 58 + 8 * 6, 75 + 12 );
    move_sprite( 25, 58 + 8 * 7, 75 + 12 );
}

void begin_level_0(){
    // Reset context
    level_ctx.state = LEVEL0STATE_INTRO_CHARACTERS;
    level_ctx.scroll_position = 0;
    level_ctx.frame_cnt = 0;
    level_ctx.current_parallax = 0;

    level_ctx.sprites[0].tile_offset = 0;
    level_ctx.sprites[0].x = 0;
    level_ctx.sprites[0].y = 103;
    level_ctx.sprites[0].timer = 8;
    level_ctx.sprites[0].frame_threshold = 8;
    level_ctx.sprites[0].frame = 0;

    level_ctx.sprites[1].tile_offset = 4;
    level_ctx.sprites[1].x = 57;
    level_ctx.sprites[1].y = 106;
    level_ctx.sprites[1].timer = 3;
    level_ctx.sprites[1].frame_threshold = 9;
    level_ctx.sprites[1].frame = 0;

    for ( uint8_t i = 0; i < NUM_PARALLAX; ++i ) {
        level_ctx.parallax_scroll_offsets[i] = 4;
    }

    VBK_REG=0;
    // Load BG tile data
    set_bkg_data( 0, 11, SplashTiles );
    set_bkg_palette( 0, SplashPalettesLen, SplashPalettes );
    set_bkg_tiles( 0, 0, splash_tilemapWidth, splash_tilemapHeight, splash_tilemap );
    VBK_REG=1;
    set_bkg_tiles( 0, 0, splash_tilemapWidth, splash_tilemapHeight, splash_tilemapPLN1 );

    set_bkg_data( 0, splash_title_tilesLen, splash_title_tiles );
    VBK_REG=0;
    set_bkg_tiles( 5, 1, splash_title_mapWidth - 2, 1, splash_title_map + 1 );
    set_bkg_tiles( 5, 2, splash_title_mapWidth - 2, 1, splash_title_map + splash_title_mapWidth * 1 + 1 );
    set_bkg_tiles( 5, 3, splash_title_mapWidth - 2, 1, splash_title_map + splash_title_mapWidth * 2 + 1 );
    set_bkg_tiles( 5, 4, splash_title_mapWidth - 2, 1, splash_title_map + splash_title_mapWidth * 3 + 1 );
    set_bkg_tiles( 4, 5, splash_title_mapWidth, 3, splash_title_map + splash_title_mapWidth * 4 );
    VBK_REG=1;
    set_bkg_tiles( 5, 1, splash_title_mapWidth - 2, 1, splash_title_mapPLN1 + 1 );
    set_bkg_tiles( 5, 2, splash_title_mapWidth - 2, 1, splash_title_mapPLN1 + splash_title_mapWidth * 1 + 1 );
    set_bkg_tiles( 5, 3, splash_title_mapWidth - 2, 1, splash_title_mapPLN1 + splash_title_mapWidth * 2 + 1 );
    set_bkg_tiles( 5, 4, splash_title_mapWidth - 2, 1, splash_title_mapPLN1 + splash_title_mapWidth * 3 + 1 );
    set_bkg_tiles( 4, 5, splash_title_mapWidth, 3, splash_title_mapPLN1 + splash_title_mapWidth * 4 );
    move_bkg( 4, 0 );

    VBK_REG=1;
    set_sprite_1bpp_data( 0x80, 96, font_ibm + 130);

    // Load sprites
    uint16_t sprite_palettes[] = {
        splash_spritesCGBPal0c0, splash_spritesCGBPal0c1, splash_spritesCGBPal0c2, splash_spritesCGBPal0c3,
        splash_spritesCGBPal1c0, splash_spritesCGBPal1c1, splash_spritesCGBPal1c2, splash_spritesCGBPal1c3,
    };
    uint8_t blappyProps1 = SpriteProperty( true, false, false, 0, 0, 0 );
    uint8_t blappyProps2 = SpriteProperty( true, false, false, 0, 0, 1 );

    VBK_REG = 0;
    set_sprite_palette( 0, 2, sprite_palettes );
    set_sprite_data( 0, splash_spritesLen / 4, splash_sprites );
    set_sprite_data( 0x80, splash_spritesLen / 4, splash_sprites + ( 2 * 8 * 4 ) );
    set_sprite_data( 4, splash_spritesLen / 4, splash_sprites + ( 2 * 8 * 8 ));
    set_sprite_data( 0x84, splash_spritesLen / 4, splash_sprites + ( 2 * 8 * 12 ) );

    set_sprite_tile( 1, 0 );
    set_sprite_tile( 0, 1 );
    set_sprite_tile( 2, 2 );
    set_sprite_tile( 3, 3 );
    set_sprite_prop( 0, blappyProps1 );
    set_sprite_prop( 1, blappyProps1 );
    set_sprite_prop( 2, blappyProps1 );
    set_sprite_prop( 3, blappyProps2 );

    set_sprite_tile( 4, 4 );
    set_sprite_tile( 5, 5 );
    set_sprite_tile( 6, 6 );
    set_sprite_tile( 7, 7 );
    set_sprite_prop( 4, blappyProps1 );
    set_sprite_prop( 5, blappyProps1 );
    set_sprite_prop( 6, blappyProps1 );
    set_sprite_prop( 7, blappyProps2 );

    move_sprite_abs( &level_ctx.sprites[0], 0, 103 );
    move_sprite_abs( &level_ctx.sprites[1], -16, 106 );

    uint8_t text_props = SpriteProperty( true, false, false, 0, 1, 0 );
    set_sprite_tile( 8,  0xB3 ); // S
    set_sprite_tile( 9,  0xB4 ); // T
    set_sprite_tile( 10, 0xA1 ); // A
    set_sprite_tile( 11, 0xB2 ); // R
    set_sprite_tile( 12, 0xB4 ); // T
    set_sprite_tile( 13, 0x9A ); // :
    set_sprite_tile( 14, 0xB0 ); // P
    set_sprite_tile( 15, 0xAC ); // L
    set_sprite_tile( 16, 0xA1 ); // A
    set_sprite_tile( 17, 0xB9 ); // Y

    set_sprite_tile( 18, 0xB3 ); // S
    set_sprite_tile( 19, 0xA5 ); // E
    set_sprite_tile( 20, 0xAC ); // L
    set_sprite_tile( 21, 0x9A ); // :
    set_sprite_tile( 22, 0xA3 ); // C
    set_sprite_tile( 23, 0xAF ); // O
    set_sprite_tile( 24, 0xA4 ); // D
    set_sprite_tile( 25, 0xA5 ); // E

    set_sprite_prop( 8,  text_props ); // S
    set_sprite_prop( 9,  text_props ); // T
    set_sprite_prop( 10, text_props ); // A
    set_sprite_prop( 11, text_props ); // R
    set_sprite_prop( 12, text_props ); // T
    set_sprite_prop( 13, text_props ); // :
    set_sprite_prop( 14, text_props ); // P
    set_sprite_prop( 15, text_props ); // L
    set_sprite_prop( 16, text_props ); // A
    set_sprite_prop( 17, text_props ); // Y

    set_sprite_prop( 18, text_props ); // S
    set_sprite_prop( 19, text_props ); // E
    set_sprite_prop( 20, text_props ); // L
    set_sprite_prop( 21, text_props ); // :
    set_sprite_prop( 22, text_props ); // C
    set_sprite_prop( 23, text_props ); // O
    set_sprite_prop( 24, text_props ); // D
    set_sprite_prop( 25, text_props ); // E

    move_control_text( 169 );

    SHOW_SPRITES;
}

bool update_level_0( GameContext* ctx ){
    switch ( level_ctx.state ) {
        case LEVEL0STATE_INTRO_CHARACTERS: {
            sprite_animation_update( &level_ctx.sprites[ 0 ] );
            sprite_animation_update( &level_ctx.sprites[ 1 ] );
            static uint8_t cnt = 3;
            if ( --cnt == 0 ) {
                cnt = 3;
                move_sprite_rel( &level_ctx.sprites[ 0 ], 1, 0 );
                move_sprite_rel( &level_ctx.sprites[ 1 ], 1, 0 );
            }

            if ( level_ctx.sprites[0].x == 73 ) {
                level_ctx.state = LEVEL0STATE_INTRO_SCROLL_LOGO;
                CRITICAL {
                    STAT_REG = STAT_REG | STATF_LYC;
                    add_LCD( lcdc_isr );
                }
                LYC_REG = parallax_scanlines[0];
                set_interrupts( VBL_IFLAG | LCD_IFLAG );
            }
            break;
        }
        case LEVEL0STATE_INTRO_SCROLL_LOGO: {
            if ( level_ctx.parallax_scroll_offsets[ 0 ] == 0x89 ) {
                level_ctx.state = LEVEL0STATE_IDLE;
                //show_control_text();
            }
            else {
                static uint8_t text_pos = 169;
                static uint8_t text_cnt = 1;
                if ( --text_cnt == 0 ) {
                    text_cnt = 2;
                    if ( text_pos > 50 ) {
                        move_control_text( --text_pos );
                    }
                }
                sprite_animation_update( &level_ctx.sprites[ 0 ] );
                sprite_animation_update( &level_ctx.sprites[ 1 ] );
                for ( int i = 0; i < NUM_PARALLAX; ++i ) {
                    if ( ++parallax_counters[ i ] >= parallax_counter_thresholds[ i ] ) {
                        parallax_counters[ i ] = 0;
                        level_ctx.parallax_scroll_offsets[ i ] += 1;
                    }
                }

                if ( level_ctx.parallax_scroll_offsets[ 0 ] == 89 ) {
                    static bool ran = false;
                    if ( !ran ) {
                        ran = true;
                        VBK_REG=0;
                        set_bkg_submap( 4, 1, 7, 7, splash_tilemap, splash_tilemapWidth );
                        VBK_REG=1;
                        set_bkg_submap( 4, 1, 7, 7, splash_tilemapPLN1, splash_tilemapWidth );
                    }
                }
                else if ( level_ctx.parallax_scroll_offsets[ 0 ] == 0x89 ) {
                    static bool ran = false;
                    if ( !ran ) {
                        ran = true;
                        VBK_REG=0;
                        set_bkg_submap( 11, 1, 7, 7, splash_tilemap, splash_tilemapWidth );
                        VBK_REG=1;
                        set_bkg_submap( 11, 1, 7, 7, splash_tilemapPLN1, splash_tilemapWidth );
                    }
                }
            }
            break;
        }
        case LEVEL0STATE_IDLE: {
            for ( int i = 0; i < NUM_PARALLAX; ++i ) {
                if ( ++parallax_counters[ i ] >= parallax_counter_thresholds[ i ] ) {
                    parallax_counters[ i ] = 0;
                    level_ctx.parallax_scroll_offsets[ i ] += 1;
                }
            }
            sprite_animation_update( &level_ctx.sprites[ 0 ] );
            sprite_animation_update( &level_ctx.sprites[ 1 ] );
            break;
        }
        case LEVEL0STATE_CODE_SELECT: {
            ctx->level_idx      = 1;
            ctx->level_state    = LEVELSTATE_BEGIN;
            break;
        }
    }

    return false;
}

void end_level_0(){
}

void begin_dialogue_level_0(){
}

void end_dialogue_level_0(){
}

void begin_pause_level_0(){
}

void end_pause_level_0(){
}
