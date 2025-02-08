#include "game/sound.h"

#include "engine/audio.h"
#include "game/game_buf.h"
#include "game/rooms.h"

typedef enum {
    SF_FLIP = 0x40,
    SF_UNFLIP = 0x80,
} SOUND_SOURCE_FLAG;

static int32_t m_SourceCount = 0;
static OBJECT_VECTOR *m_Sources = nullptr;
static int16_t m_SampleLUT[SFX_NUMBER_OF];
static SAMPLE_INFO *m_SampleInfos = nullptr;

void Sound_InitialiseSources(const int32_t num_sources)
{
    m_SourceCount = num_sources;
    m_Sources = num_sources == 0
        ? nullptr
        : GameBuf_Alloc(
              num_sources * sizeof(OBJECT_VECTOR), GBUF_SOUND_SOURCES);
}

void Sound_InitialiseSampleInfos(const int32_t num_sample_infos)
{
    m_SampleInfos = num_sample_infos == 0
        ? nullptr
        : GameBuf_Alloc(
              sizeof(SAMPLE_INFO) * num_sample_infos, GBUF_SAMPLE_INFOS);
}

int32_t Sound_GetSourceCount(void)
{
    return m_SourceCount;
}

OBJECT_VECTOR *Sound_GetSource(const int32_t source_idx)
{
    if (m_Sources == nullptr) {
        return nullptr;
    }
    return &m_Sources[source_idx];
}

int16_t *Sound_GetSampleLUT(void)
{
    return m_SampleLUT;
}

SAMPLE_INFO *Sound_GetSampleInfo(const SOUND_EFFECT_ID sfx_num)
{
    const int16_t info_idx = m_SampleLUT[sfx_num];
    return info_idx < 0 ? nullptr : &m_SampleInfos[info_idx];
}

SAMPLE_INFO *Sound_GetSampleInfoByIdx(const int32_t info_idx)
{
    return &m_SampleInfos[info_idx];
}

void Sound_ResetSources(void)
{
    const bool flip_status = Room_GetFlipStatus();
    for (int32_t i = 0; i < m_SourceCount; i++) {
        OBJECT_VECTOR *const source = &m_Sources[i];
        if ((flip_status && (source->flags & SF_FLIP))
            || (!flip_status && (source->flags & SF_UNFLIP))) {
            Sound_Effect(source->data, &source->pos, SPM_NORMAL);
        }
    }
}

void Sound_PauseAll(void)
{
    Audio_Sample_PauseAll();
}

void Sound_UnpauseAll(void)
{
    Audio_Sample_UnpauseAll();
}

bool Sound_IsAvailable(const SOUND_EFFECT_ID sample_id)
{
    return sample_id >= 0 && sample_id < SFX_NUMBER_OF
        && m_SampleLUT[sample_id] != -1;
}
