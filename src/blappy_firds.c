#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/console.h>
#include <string.h>
#include <rand.h>
#include <stdbool.h>
#include <stdio.h>

#include <gbdk/platform.h>

#include <gbdk/font.h>

#include "tile_data.h"
//#include "pipes_map.h"

// TODO:
// - Scoring
// - Splash
// - Starting Screen
// - Game Over screen
// - Adjust mechanics


#define INTERPIPE_DISTANCE 16
#define PIPE_ARRAY_LENGTH 2

// 4 tiles
#define PIPE_WIDTH 4

#define SCREENTILEHEIGHT ( SCREENHEIGHT / 8 )
#define SCREENTILEWIDTH ( SCREENWIDTH / 8 )

#define BLAPPY_PIXEL_HEIGHT 14
#if ( ( SCREENTILEWIDTH + PIPE_WIDTH + 1 ) / INTERPIPE_DISTANCE ) > PIPE_ARRAY_LENGTH
#error Pipe array not large enough to hold all possible visible pipes
#endif

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
    int8_t y_speed;
    Vec2d acceleration;
} BlappyState;

typedef enum GameState {
    SplashScreen,
    StartingScreen,
    InGame,
    TextDialog,
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
        blappyState->y_speed += blappyState->acceleration.y;
        // TODO - consider an alternative fix for the hang at arc maxima
        if ( blappyState->y_speed == 0 ) {
            blappyState->y_speed = 1;
        }
    }
    //blappyState->speed.x += blappyState->acceleration.x;
    //blappyState->position.x += blappyState->speed.x;
    blappyState->position.y += blappyState->y_speed;

    if ( blappyState->position.y <= 14 ) {
        blappyState->position.y = 14;
        blappyState->y_speed = 0;
    }

}

void InitBlappy() {
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
}


void LoadPipes() {
    uint16_t backgroundPalette[] = { 
        TileDataCGBPal0c0, TileDataCGBPal0c1, TileDataCGBPal0c2, TileDataCGBPal0c3
    };

    set_bkg_palette( 0, 1, backgroundPalette );
    set_bkg_data( 0, 13, TileData );

    VBK_REG = 0;
}

typedef struct PipeObstacle {
    uint8_t verticalOffset; // Needs to be constrained. Also needs to be in tile-space
    uint8_t gapSize;
    uint8_t position;
} PipeObstacle;

typedef struct GameContext {
    PipeObstacle pipeList[ PIPE_ARRAY_LENGTH ];
    uint8_t scrollPosition;
    uint8_t primaryPipeIndex;
    BlappyState blappyState;
} GameContext;


void SetPipeTilemap( PipeObstacle* pipe ) {
    // 5 stages:
    // 1. Top pipe body
    // 2. Top pipe head
    // 3. Gap
    // 4. Bottom pipe head
    // 5. Bottom pipe body

    // TODO - consider other methods of copying the data,
    // possibly having a static copy of pipe body/head in memory that are just DMAd as required.
    // For now, just copy one-by-one.
    const uint8_t bodyTilemap[] = { 9, 10, 11, 12 };
    const uint8_t headTopTilemap[] = { 2, 4, 6, 8, 1, 3, 5, 7 };
    const uint8_t headBottomTilemap[] = { 2, 4, 6, 8, 1, 3, 5, 7 };

    //set_bkg_tiles( pipe->position, 0, 4, SCREENTILEHEIGHT, pipeTilemap );
    for( uint8_t i = 0; i < pipe->verticalOffset; ++i ) {
        set_bkg_tiles( pipe->position, i, 4, 1, bodyTilemap );
    }
    set_bkg_tiles( pipe->position, pipe->verticalOffset, 4, 2, headTopTilemap );

    set_bkg_tiles( pipe->position, pipe->verticalOffset + pipe->gapSize + 2, 4, 2, headBottomTilemap );
    for( uint8_t i = pipe->verticalOffset + pipe->gapSize + 4; i < SCREENTILEHEIGHT; ++i ) {
        set_bkg_tiles( pipe->position, i, 4, 1, bodyTilemap );
    }
}

void ClearPipeTilemap( PipeObstacle* pipe ) {
    const uint8_t clrscr[ 4 * SCREENTILEHEIGHT ] = { 0 };
    set_bkg_tiles( pipe->position, 0, 4, SCREENTILEHEIGHT, clrscr );
}


bool CheckCollision( GameContext *context ) {
    const uint8_t blappyXScreenSpace = context->blappyState.position.x + context->scrollPosition - 8;
    const uint8_t blappyYScreenSpace = context->blappyState.position.y - 14;
    
    if ( blappyYScreenSpace + BLAPPY_PIXEL_HEIGHT >= 144 ) {
        return true;
    }

    PipeObstacle *primaryPipe = &context->pipeList[ context->primaryPipeIndex ];
    uint8_t pipeXScreenSpace = primaryPipe->position << 3;

//    if ( ( blappyXScreenSpace + 16 > pipeXScreenSpace && blappyXScreenSpace + 16 < pipeXScreenSpace + 32 )
//        || ( blappyXScreenSpace > pipeXScreenSpace && blappyXScreenSpace < pipeXScreenSpace + 32 ) ) {

    if ( blappyXScreenSpace + 16 > pipeXScreenSpace && blappyXScreenSpace + 16 < pipeXScreenSpace + 32 + 16 ) {
        // In x-range
        uint8_t offsetScreenSpaceMin = ( primaryPipe->verticalOffset + 2 ) << 3;
        uint8_t offsetScreenSpaceMax = offsetScreenSpaceMin + ( primaryPipe->gapSize << 3 );
        if ( blappyYScreenSpace < offsetScreenSpaceMin || blappyYScreenSpace + 11 > offsetScreenSpaceMax ) {
            // Collision
            return true;
        }
        
    }

    return false;
}

void GeneratePipe( PipeObstacle* pipe, uint8_t pos ) {
    // Gap size must be smaller than (SCREENTILEHEIGHT - 2 - 2) = 13
    // Also must have some min.
    pipe->gapSize = 7;

    // Offset must be smaller than (SCREENTILEHEIGHT - 1 - gap - 2) 
    const uint8_t maxOffset = SCREENTILEHEIGHT - 3 - pipe->gapSize;
    
    pipe->verticalOffset = rand();
    while ( pipe->verticalOffset > maxOffset ) {
        pipe->verticalOffset -= maxOffset;
    }
    pipe->position = pos;
}


void ScrollWorld( GameContext *context ) {
    context->scrollPosition += 1;
    PipeObstacle *primaryPipe = &context->pipeList[ context->primaryPipeIndex ];
    const uint8_t pipePosition = ( primaryPipe->position << 3 ) + 32; // *8, tile-space to screen-space

    if ( context->scrollPosition== pipePosition ) {
        // Pipe off screen, generate new one
        uint8_t lastPipeIndex = context->primaryPipeIndex;
        if ( lastPipeIndex == 0 ) lastPipeIndex = PIPE_ARRAY_LENGTH - 1;
        else lastPipeIndex -= 1;
        uint8_t lastPipePos = context->pipeList[ lastPipeIndex ].position;
        uint8_t nextPipePos = ( lastPipePos + INTERPIPE_DISTANCE - 1 ) % 31;

        ClearPipeTilemap( primaryPipe );
        GeneratePipe( primaryPipe, nextPipePos );
        SetPipeTilemap( primaryPipe );
        context->primaryPipeIndex = ( context->primaryPipeIndex + 1 ) % PIPE_ARRAY_LENGTH;
    }

    move_bkg( context->scrollPosition, 0 );
}

typedef struct TextDialogContext {
    const char** dialogs;
    uint8_t num_dialogs;
    uint8_t current_dialogs;
} TextDialogContext;

TextDialogContext BeginTextDialog( GameState *state, const char** dialogs, uint8_t num_dialogs ) {
    state = TextDialog;
    TextDialogContext context;
    context.dialogs = dialogs;
    context.num_dialogs = num_dialogs;
    context.current_dialogs = 0;
    return context;
}

bool UpdateTextDialogState( TextDialogContext* context ) {
    if ( joypad() & J_A ) {
        ++context->current_dialogs;
        if ( context->current_dialogs == context->num_dialogs ) {
            return true;
        }
        else {
            
        }
    }
}

void GameLoop() {
    GameState gameState = SplashScreen;
    uint8_t frameUpdateCounter = 0;
    uint16_t counter = 0;

    GameContext context = {
        .primaryPipeIndex = 0,
        .scrollPosition = 0
    };

    while( true ) {
        // TODO - wait for VBL to start
        wait_vbl_done();
        switch ( gameState ) {
            case SplashScreen: {
                if ( ++counter == 1 ) {
                    cls();
                    for( uint8_t i = 0; i <  SCREENTILEHEIGHT / 2; ++i ) printf( "\n" );
                    printf( "    Blappy Fird" );
                }

                if ( ++counter == 60 * 3 || joypad() & J_START ) {

                    gameState = StartingScreen;
                    counter = 0;
                }
                break;
            }
            case StartingScreen: {
                if ( ++counter == 1 ) {
                    cls();
                    for( uint8_t i = 0; i <  SCREENTILEHEIGHT / 2; ++i ) printf( "\n" );
                    printf ( "Press START to play" );
                }
                if ( joypad() & J_START ) {
                    // Initialise pipes
                    cls();
                    LoadPipes();
                    context.primaryPipeIndex = 0;

                    for ( uint8_t i = 0; i < PIPE_ARRAY_LENGTH; ++i ) {
                        PipeObstacle *cPipe = &context.pipeList[ i ];
                        GeneratePipe( cPipe, ( INTERPIPE_DISTANCE * i ) );
                        SetPipeTilemap( cPipe );
                    }

                    move_bkg( 0, 0 );
                    context.blappyState.acceleration.x = 0;
                    context.blappyState.acceleration.y = 1;
                    //context.blappyState.speed.x = 0;
                    context.blappyState.y_speed = 0;
                    context.blappyState.position.x = 70 + 8;
                    context.blappyState.position.y = 40 + 16;
                    context.primaryPipeIndex = 0;
                    context.scrollPosition = 0;

                    SHOW_SPRITES;
                    gameState = InGame;
                }
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
                            context.blappyState.y_speed = -2;
                        }
                        aState = aPressed;
                    }

                    if ( inputState & J_START ) {
                        // TODO - pause
                    }
                    UpdateBlappy( &context.blappyState );

                    if ( CheckCollision( &context ) ) {
                        gameState = GameOver;
                        context.blappyState.position.y = (uint8_t)( SCREENHEIGHT - BLAPPY_PIXEL_HEIGHT + 16 );
                        context.blappyState.y_speed = 0;
                        counter = 0;
                        break;
                    }
                    MoveBlappy( context.blappyState.position.x, context.blappyState.position.y );
                    ScrollWorld( &context );
                    //scroll_bkg( 0, 1 );
                }
                break;
            }
            case GameOver: {
                if ( ++counter == 1 ) {
                    cls();
                    move_bkg( 0, 0 );
                    HIDE_SPRITES;
                    for( uint8_t i = 0; i <  SCREENTILEHEIGHT / 2; ++i ) printf( "\n" );
                    printf( "     Game Over" );
                }
                if ( joypad() & J_START ) {
                    counter = 0;
                    gameState = StartingScreen;
                }
                break;
            }
            default: {
                gameState = SplashScreen;
                break;
            }
        }
    }
}

struct font {
    uint8_t font_tile_start;
    uint16_t font_table;
};

int func( struct font* _dont ) {

}

int main() {
    uint8_t start_tile = 0x24;
    struct font test_font;
    test_font.font_tile_start = start_tile;
    test_font.font_table = (uint16_t) font_ibm;

    font_init();
    //font_t ibm_font = font_load(font_ibm);  /* 96 tiles */
//
    //struct font *a = (struct font*) ibm_font;
    //func( a );
//
    font_set( (uint16_t) &test_font );
    //font_set(ibm_font);
    set_bkg_1bpp_data( 0x0030, 96, font_ibm + 130);
    printf( "Font demo.\n\n" );
    //printf( "Font demo.\n\n" );
    //printf( "Font demo.\n\n" );
    //printf( "Font demo.\n\n" );
    //printf( "Font demo.\n\n" );
   // while(1);
    //uint16_t seed = LY_REG;
    //seed |= (uint16_t)DIV_REG << 8;
    //initrand( seed );

    //LoadPipes();

    SPRITES_8x8;
    InitBlappy();

    SHOW_SPRITES;
    SHOW_BKG;
    DISPLAY_ON;
    uint16_t backgroundPalette[] = { 
        TileDataCGBPal0c0, TileDataCGBPal0c1, TileDataCGBPal0c2, TileDataCGBPal0c3
    };
    set_bkg_palette( 0, 1, backgroundPalette );
    //VBK_REG = 0;

    //wait_vbl_done();
    //cls();
    //printf( "Font Demo" );
    while(1);
    //GameLoop();

    return 0;
}