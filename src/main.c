#include "common.h"

#define DECL_LEVEL( level_num )                     \
    void begin_level_##level_num();                 \
    bool update_level_##level_num( GameContext* );  \
    void end_level_##level_num();                   \
    void begin_dialogue_level_##level_num();        \
    void end_dialogue_level_##level_num();          \
    void begin_pause_level_##level_num();           \
    void end_pause_level_##level_num();

#define DEF_LEVEL_STRUCT( level_num )                       \
    {                                                       \
        .begin          = begin_level_##level_num,          \
        .update         = update_level_##level_num,         \
        .end            = end_level_##level_num,            \
        .begin_dialogue = begin_dialogue_level_##level_num, \
        .end_dialogue   = end_dialogue_level_##level_num,   \
        .begin_pause    = begin_pause_level_##level_num,    \
        .end_pause      = end_pause_level_##level_num,      \
    }

DECL_LEVEL( 0 ) // Splash screen
DECL_LEVEL( 1 ) // Blappy (test)
#define MAX_LEVEL 1

Level levels[] = {
    DEF_LEVEL_STRUCT( 0 ),
    DEF_LEVEL_STRUCT( 1 )
};

GameContext context = {
    .state          = GAMESTATE_LEVEL,
    .level_idx      = 1,
    .level_state    = LEVELSTATE_BEGIN,
    .input          = { .state_changed = 0, .button_pressed = 0 }
};

static inline void level_state_update() {
    Level *level = &levels[ context.level_idx ];
    switch ( context.level_state ) {
        case LEVELSTATE_BEGIN: {
            level->begin();
            context.level_state = LEVELSTATE_UPDATE;
            break;
        }
        case LEVELSTATE_UPDATE: {
            level->update( &context );
            break;
        }
        case LEVELSTATE_DIALOGUE_BEGIN: {
            break;
        }
        case LEVELSTATE_DIALOGUE_UPDATE: {
            break;
        }
    }
}

void main() {
    // Game code

    // Splash-specific handling


    // Main loop
    while ( 1 ) {
        wait_vbl_done();
        // Input state update, check for Pause condition

        // Game state update
        switch ( context.state ) {
            case GAMESTATE_LEVEL: {
                level_state_update();
                break;
            }

            case GAMESTATE_PAUSED: {
                break;
            }
        }
    }
}