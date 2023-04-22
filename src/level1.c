#include "common.h"

#include "level_select_tiles.h"
#include "level_select_arrows_tilemap.h"

#include <assert.h>
#include <string.h>
#include <gb/cgb.h>
#include <gb/gb.h>
#include <gbdk/font.h>

#define ARROW_FLASH_FRAME_PERIOD 30;

typedef enum Level1State {
    LEVEL1_STATE_LOAD_0,
    LEVEL1_STATE_LOAD_1
} Level1State;

struct Level1Context {
    Level1State state;
    uint8_t dial_values[3];
    uint8_t selected_dial;

    uint8_t frame_counter;
    uint8_t arrow_palette;
    uint8_t jp_state;
} level1_ctx;

#define SpriteProperty( priority, vflip, hflip, bwPalette, bank, colourPalette ) \
    (uint8_t) ( \
        ( ( priority ? 0x00 : 0x01 ) << 7 ) | \
        ( ( vflip ? 0x01 : 0x0 ) << 6 ) | \
        ( ( hflip ? 0x01 : 0x0 ) << 5 ) | \
        ( ( bwPalette & 0x01 ) << 4 ) | \
        ( ( bank & 0x01 ) << 3 ) | \
        ( colourPalette & 0x07 ) \
    )

static inline void reset_layout() {
    // TODO -- Optimise memory writes
    uint16_t pal[] = { 
        level_select_tilesCGBPal0c0,
        level_select_tilesCGBPal0c1,
        level_select_tilesCGBPal0c2,
        level_select_tilesCGBPal0c3,
        level_select_tilesCGBPal1c0,
        level_select_tilesCGBPal1c1,
        level_select_tilesCGBPal1c2,
        level_select_tilesCGBPal1c3
    };

    set_bkg_palette( 0, 2, pal );
    VBK_REG=0;
    set_bkg_tiles( 2, 4, 4, 2, level_select_arrows_tilemap );
    set_bkg_tiles( 8, 4, 4, 2, level_select_arrows_tilemap );
    set_bkg_tiles( 14, 4, 4, 2, level_select_arrows_tilemap );
    set_bkg_tiles( 2, 12, 4, 2, level_select_arrows_tilemap + ( 4 * 2 ) );
    set_bkg_tiles( 8, 12, 4, 2, level_select_arrows_tilemap + ( 4 * 2 ) );
    set_bkg_tiles( 14, 12, 4, 2, level_select_arrows_tilemap + ( 4 * 2 ) );


    VBK_REG=1;
    set_bkg_tiles( 2, 4, 4, 2, level_select_arrows_tilemapPLN1 );
    set_bkg_tiles( 8, 4, 4, 2, level_select_arrows_tilemapPLN1 );
    set_bkg_tiles( 14, 4, 4, 2, level_select_arrows_tilemapPLN1 );
    set_bkg_tiles( 2, 12, 4, 2, level_select_arrows_tilemapPLN1 + ( 4 * 2 ) );
    set_bkg_tiles( 8, 12, 4, 2, level_select_arrows_tilemapPLN1 + ( 4 * 2 ) );
    set_bkg_tiles( 14, 12, 4, 2, level_select_arrows_tilemapPLN1 + ( 4 * 2 ) );

    VBK_REG=1;
    set_sprite_tile( 19, 0xA9 ); // I
    set_sprite_tile( 20, 0xAE ); // N
    set_sprite_tile( 21, 0xB6 ); // V
    set_sprite_tile( 22, 0xA1 ); // A
    set_sprite_tile( 23, 0xAC ); // L
    set_sprite_tile( 24, 0xA9 ); // I
    set_sprite_tile( 25, 0xA4 ); // D

    set_sprite_tile( 26, 0xA3 ); // C
    set_sprite_tile( 27, 0xAF ); // O
    set_sprite_tile( 28, 0xA4 ); // D
    set_sprite_tile( 28, 0xA5 ); // E

    VBK_REG=0;
    {
        const uint8_t enter_map[] = {0xA5, 0xAE, 0xB4, 0xA5, 0xB2};
        set_bkg_tiles( 4, 1, 5, 1, enter_map );
    }
    {
        const uint8_t code_map[] = {0xA3, 0xAF, 0xA4, 0xA5};
        set_bkg_tiles( 11, 1, 4, 1, code_map );
    }
    {
        const uint8_t ago_map[] = {0xA1, 0x9A, 0xA7, 0xAF};
        set_bkg_tiles( 8, 15, 4, 1, ago_map );
    }
    {
        const uint8_t bback_map[] = {0xA2, 0x9A, 0xA2, 0xA1, 0xA3, 0xAB};
        set_bkg_tiles( 7, 16, 6, 1, bback_map );
    }
    VBK_REG=1;
    {
        const uint8_t text_prop = SpriteProperty( 0, 0, 0, 0, 1, 0 );
        const uint8_t text_props[] = {text_prop, text_prop, text_prop, text_prop, text_prop, text_prop};
        set_bkg_tiles( 4, 1, 5, 1, text_props );
        set_bkg_tiles( 11, 1, 4, 1, text_props );
        set_bkg_tiles( 8, 15, 4, 1, text_props );
        set_bkg_tiles( 7, 16, 6, 1, text_props );
    }
}

static void set_dial_tiles( uint8_t dial_index, uint8_t dial_value ) {
    // TODO - actual art
    assert( dial_index < 3 );
    assert( dial_value < 10 );
    const uint8_t offset_left = ( dial_index * 6 ) + 2;

    VBK_REG = 0;
    const uint8_t temp_map[] = { 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99 };
    set_bkg_tiles( offset_left, 7, 1, 1, temp_map + ( dial_value * 1 ) );

    VBK_REG = 1;
    const uint8_t temp_props[] = { SpriteProperty( 0, 0, 0, 0, 1, 0 ) };
    set_bkg_tiles( offset_left, 7, 1, 1, temp_props );
    //VBK_REG = 0;
}

static void set_arrow_palette( uint8_t dial_index, uint8_t palette ) {
    assert( dial_index < 3 );
    assert( palette <= 1 );
    const uint8_t offset_left = ( dial_index * 6 ) + 2;

    uint8_t palette2[ 4 * 2 * 2 ];
    uint8_t *plane_data = level_select_arrows_tilemapPLN1;

    if ( palette == 1 ) {
        memcpy( palette2, plane_data, 4 * 2 * 2 );
        for ( uint8_t i = 0; i < 4 * 2 * 2; ++i ) {
            palette2[ i ] |= 1;
        }
        plane_data = palette2;
    }

    VBK_REG = 1;
    set_bkg_tiles( offset_left, 4, 4, 2, plane_data );
    set_bkg_tiles( offset_left, 12, 4, 2, plane_data + ( 4 * 2 ) );
    //VBK_REG = 0;
}


void begin_level_1(){
    level1_ctx.state = LEVEL1_STATE_LOAD_0;
    level1_ctx.dial_values[0] = 1;
    level1_ctx.dial_values[1] = 2;
    level1_ctx.dial_values[2] = 3;
    level1_ctx.selected_dial = 1;
    level1_ctx.frame_counter = ARROW_FLASH_FRAME_PERIOD;
    level1_ctx.arrow_palette = 0;
    level1_ctx.jp_state = joypad();

    VBK_REG=1;
    set_sprite_1bpp_data( 0x80, 96, font_ibm + 130 );
    wait_vbl_done();
    VBK_REG = 0;
    set_bkg_data( 0, level_select_tilesLen, level_select_tiles );
    wait_vbl_done();
    reset_layout();

    set_dial_tiles( 0, 1 );
    set_dial_tiles( 1, 2 );
    set_dial_tiles( 2, 3 );
}

static bool update_joy_state ( uint8_t new_jp_state, uint8_t button ) {
    static uint8_t state = 0;
    const uint8_t new_button_state = new_jp_state & button;
    if ( new_button_state != ( state & button ) ) {
        state = ( state & ~button ) | new_button_state;
        return new_button_state;
    }
    return false;
}

#define DIALS_TO_COMBO( a, b, c ) ((((uint16_t)c) << 8) | (((uint16_t)b) << 4) | ((uint16_t)a))

static uint8_t level_code_map( uint8_t dials[3] ) {
    const uint16_t combination = DIALS_TO_COMBO( dials[0], dials[1], dials[2] );

    switch( combination ) {
        case DIALS_TO_COMBO( 4, 2, 2 ) : {
            return 0;
        }
        case DIALS_TO_COMBO( 1, 2, 0 ) : {
            return 1;
        }
        case DIALS_TO_COMBO( 2, 1, 5 ) : {
            return 2;
        }
        default : {
            return 255;
        }
    }
}

bool update_level_1( GameContext* ctx ) {

    const uint8_t new_jp_state = joypad();
    const uint8_t jp_state_changed = new_jp_state ^ level1_ctx.jp_state;
    const uint8_t jp_new_pressed = jp_state_changed & new_jp_state;
    level1_ctx.jp_state = new_jp_state;

    //level1_ctx.state = load_state_step( level1_ctx.state );
    if ( jp_new_pressed & J_UP ) {
        if ( ++level1_ctx.dial_values[ level1_ctx.selected_dial ] == 10 ) {
            level1_ctx.dial_values[ level1_ctx.selected_dial ] = 0;
        }
        set_dial_tiles( level1_ctx.selected_dial, level1_ctx.dial_values[ level1_ctx.selected_dial ] );
    }
    else if ( jp_new_pressed & J_DOWN ) {
        if ( --level1_ctx.dial_values[ level1_ctx.selected_dial ] == 255 ) {
            level1_ctx.dial_values[ level1_ctx.selected_dial ] = 9;
        }
        set_dial_tiles( level1_ctx.selected_dial, level1_ctx.dial_values[ level1_ctx.selected_dial ] );
    }
    else if ( jp_new_pressed & J_RIGHT ) {
        set_arrow_palette( level1_ctx.selected_dial, 0 );
        if ( ++level1_ctx.selected_dial == 3 ) {
            level1_ctx.selected_dial = 0;
        }
        level1_ctx.arrow_palette = 1;
        set_arrow_palette( level1_ctx.selected_dial, level1_ctx.arrow_palette );
        level1_ctx.frame_counter = ARROW_FLASH_FRAME_PERIOD;
    }
    else if ( jp_new_pressed & J_LEFT ) {
        set_arrow_palette( level1_ctx.selected_dial, 0 );
        if ( --level1_ctx.selected_dial == 255 ) {
            level1_ctx.selected_dial = 2;
        }
        level1_ctx.arrow_palette = 1;
        set_arrow_palette( level1_ctx.selected_dial, level1_ctx.arrow_palette );
        level1_ctx.frame_counter = ARROW_FLASH_FRAME_PERIOD;
    }
    else if ( jp_new_pressed & J_B ) {
        ctx->level_idx = 0;
        ctx->level_state = LEVELSTATE_BEGIN;
    }
    else if ( jp_new_pressed & J_A ) {
        const uint8_t level_code = level_code_map( level1_ctx.dial_values );
        if ( level_code == 255 ) {
            // TODO - "Invalid code" state
        }
        else {
            ctx->level_idx = level_code;
            ctx->level_state = LEVELSTATE_BEGIN;
        }

    }
    else if ( --level1_ctx.frame_counter == 0 ) {
        level1_ctx.frame_counter = ARROW_FLASH_FRAME_PERIOD;
        if ( ++level1_ctx.arrow_palette == 2 ) {
            level1_ctx.arrow_palette = 0;
        }
        set_arrow_palette( level1_ctx.selected_dial, level1_ctx.arrow_palette );
    }

    return false;
}

void end_level_1(){}

void begin_dialogue_level_1(){}

void end_dialogue_level_1(){}

void begin_pause_level_1(){}


void end_pause_level_1(){}
