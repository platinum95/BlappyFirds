#include <gb/gb.h>
#include <gb/cgb.h> 
#include <stdbool.h>
#include <stdio.h>

#include "tile_data.h"
#include "pipes_map.h"

#define INTERPIPE_DISTANCE 64

#define SpriteProperty( priority, vflip, hflip, bwPalette, bank, colourPalette ) \
    (uint8_t) ( \
        ( ( priority ? 0x00 : 0x01 ) << 7 ) | \
        ( ( vflip ? 0x01 : 0x0 ) << 6 ) | \
        ( ( hflip ? 0x01 : 0x0 ) << 5 ) | \
        ( ( bwPalette & 0x01 ) << 4 ) | \
        ( ( bank & 0x01 ) << 3 ) | \
        ( colourPalette & 0x07 ) \
    )

typedef struct Vec2d {
    uint8_t x, y;
} Vec2d;

typedef struct BlappyState {
    Vec2d position;
    Vec2d speed;
    Vec2d acceleration;
} BlappyState;

typedef enum GameState {
    SplashScreen,
    StartingGame,
    InGame,
    GameOver
} GameState;

// Sprite position offset is (8, 16), so setting pos (0,0) ends up with position (-8,-16).
// Screen.coord = Sprite.coord - (8, 16)
// Sprite.coord = Screen.coord + (8, 16)
// Sprite is offscreen after screen coord 160 (sprite coord 168)
// Wraparound is not dependable, make sure to clamp position to either on-screen or off-screen, keep away from uint8 upper boundary.

void MoveBlappy( uint8_t x, uint8_t y ) {
    move_sprite( 0, x + 0, y + 0 );
    move_sprite( 1, x + 8, y + 0 );
    move_sprite( 2, x + 0, y + 8 );
    move_sprite( 3, x + 8, y + 8 );
}

void UpdateBlappy( BlappyState *blappyState ) {
    static uint8_t gravityDiv = 0;
    if ( ++gravityDiv == 12 ) {
        gravityDiv = 0;
        blappyState->speed.y += blappyState->acceleration.y;
    }
    blappyState->speed.x += blappyState->acceleration.x;
    blappyState->position.x += blappyState->speed.x;
    blappyState->position.y += blappyState->speed.y;
}

void InitBlappy( BlappyState *blappyState ) {
    uint16_t blappyPalette[] = {
        TileDataCGBPal1c0, TileDataCGBPal1c1, TileDataCGBPal1c2, TileDataCGBPal1c3,
        TileDataCGBPal2c0, TileDataCGBPal2c1, TileDataCGBPal2c2, TileDataCGBPal2c3,
    };
    uint8_t blappyProps1 = SpriteProperty( true, false, false, 0, 0, 0 );
    uint8_t blappyProps2 = SpriteProperty( true, false, false, 0, 0, 1 );

    set_sprite_palette( 0, 2, blappyPalette );
    set_sprite_data( 0, 4, TileData + ( 13 * 16 ) );

    set_sprite_tile( 0, 0 );
    set_sprite_tile( 1, 1 );
    set_sprite_tile( 2, 2 );
    set_sprite_tile( 3, 3 );

    set_sprite_prop( 0, blappyProps1 );
    set_sprite_prop( 1, blappyProps1 );
    set_sprite_prop( 2, blappyProps1 );
    set_sprite_prop( 3, blappyProps2 );

    blappyState->acceleration.x = 0;
    blappyState->acceleration.y = 1;
    blappyState->speed.x = 0;
    blappyState->speed.y = 0;
    blappyState->position.x = 70;
    blappyState->position.y = 40;
}

void InitPipes() {
    uint16_t backgroundPalette[] = { 
        TileDataCGBPal0c0, TileDataCGBPal0c1, TileDataCGBPal0c2, TileDataCGBPal0c3
    };

    set_bkg_palette( 0, 1, backgroundPalette );
    set_bkg_data( 0, 13, TileData );

    VBK_REG = 1;
    VBK_REG = 0;
    set_bkg_tiles( 0, 0, 10, 10, PipesTilemap );


    VBK_REG = 1;
    //set_bkg_tiles( 0, 0, 10, 10, palette_map );

    VBK_REG = 0;
}

typedef struct PipeObstacle {
    uint8_t gapSize;
} PipeObstacle;

bool CheckCollision( BlappyState* blappyState ) {
    if ( blappyState->position.y + 16 >= 144 ) {
        return true;
    }
    // TODO - pipe collision
    return false;
}

void GameLoop( BlappyState *blappyState ) {
    GameState gameState = SplashScreen;
    uint8_t frameUpdateCounter = 0;

    while( true ) {
        // TODO - wait for VBL to start
        wait_vbl_done();
        switch ( gameState ) {
            case SplashScreen: {
                gameState = StartingGame;
                break;
            }
            case StartingGame: {
                gameState = InGame;
                break;
            }
            case InGame: {
                static bool aState = false;
                if ( ++frameUpdateCounter == 1 ) {
                    frameUpdateCounter = 0;

                    uint8_t inputState = joypad();
                    bool aPressed = inputState & J_A;
                    if ( aState != aPressed ) {
                        if ( aPressed ) {
                            blappyState->speed.y = -3;
                        }
                        aState = aPressed;
                    }

                    if ( inputState & J_START ) {
                        // TODO - pause
                    }
                    UpdateBlappy( blappyState );

                    if ( CheckCollision( blappyState ) ) {
                        //gameState = GameOver;
                        blappyState->position.y = 160;
                        blappyState->speed.y = 0;
                        break;
                    }
                    MoveBlappy( blappyState->position.x, blappyState->position.y );
                }
                break;
            }
            case GameOver: {
                gameState = SplashScreen;
                break;
            }
            default: {
                gameState = SplashScreen;
                break;
            }
        }
    }
}

int main() {
    InitPipes();

    SPRITES_8x8;
    BlappyState blappyState;
    InitBlappy( &blappyState );

    SHOW_SPRITES;
    SHOW_BKG;
    DISPLAY_ON;

    GameLoop( &blappyState );

    return 0;
}