#include "game/phase/phase.h"

#include "game/phase/phase_game.h"
#include "game/phase/phase_pause.h"
#include "global/types.h"
#include "log.h"

#include <stddef.h>

static PHASER *m_Phaser = NULL;

void Phase_Set(const PHASE phase)
{
    if (m_Phaser && m_Phaser->end) {
        m_Phaser->end();
    }

    LOG_DEBUG("%d", phase);
    switch (phase) {
    case PHASE_NULL:
        m_Phaser = NULL;
        break;

    case PHASE_GAME:
        m_Phaser = &g_GamePhaser;
        break;

    case PHASE_PAUSE:
        m_Phaser = &g_PausePhaser;
        break;
    }

    if (m_Phaser && m_Phaser->start) {
        m_Phaser->start();
    }
}

GAMEFLOW_OPTION Phase_Control(int32_t nframes)
{
    if (m_Phaser && m_Phaser->control) {
        return m_Phaser->control(nframes);
    }
    return GF_NOP;
}

void Phase_Draw(void)
{
    if (m_Phaser && m_Phaser->draw) {
        m_Phaser->draw();
    }
}
