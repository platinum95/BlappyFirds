#ifndef L2D_COMMON_H
#define L2D_COMMON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GAMESTATE_LEVEL,
    GAMESTATE_PAUSED
} GameState;

typedef enum {
    LEVELSTATE_BEGIN,
    LEVELSTATE_UPDATE,
    LEVELSTATE_DIALOGUE_BEGIN,
    LEVELSTATE_DIALOGUE_UPDATE,
} LevelState;

typedef struct {
    uint8_t state_changed;
    uint8_t button_pressed;
} InputContext;

typedef struct {
    const char    **dialogues;
    uint8_t         num_dialogues;
    uint8_t         current_dialogue;
} DialogueContext;

typedef struct {
    GameState       state;
    uint8_t         level_idx;
    LevelState      level_state;
    InputContext    input;
    DialogueContext dialogue;
} GameContext;

typedef struct {
    void    (*begin)            (void);
    bool    (*update)           (GameContext*);
    void    (*end)              (void);

    void    (*begin_dialogue)   (void);
    void    (*end_dialogue)     (void);

    void    (*begin_pause)      (void);
    void    (*end_pause)        (void);
} Level;

#endif //L2D_COMMON_H