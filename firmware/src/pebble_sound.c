#include "pebble_sound.h"
#include "tal_api.h"
#include "tdl_audio_manage.h"

/* A short two-note chirp in the board codec's 16 kHz, mono PCM format. */
bool pb_sound_play(unsigned reaction)
{
    static const unsigned notes[][2] = {{620, 830}, {784, 988}, {523, 392}};
    static const int16_t wave[16] = {
        0, 1254, 2317, 3027, 3276, 3027, 2317, 1254,
        0, -1254, -2317, -3027, -3276, -3027, -2317, -1254
    };
    TDL_AUDIO_HANDLE_T codec = NULL;
    if (reaction >= 3 || tdl_audio_find(AUDIO_CODEC_NAME, &codec) != OPRT_OK) return false;
    /* The board codec opens input and output together. Discard input, and
     * close both again after the speaker buffer drains. */
    bool ok = tdl_audio_open(codec, NULL) == OPRT_OK;
    if (ok) ok = tdl_audio_volume_set(codec, 32) == OPRT_OK;
    for (unsigned note = 0; ok && note < 2; ++note) {
        unsigned phase = 0;
        for (unsigned frame = 0; ok && frame < 4; ++frame) {
            int16_t pcm[320];
            for (unsigned i = 0; i < 320; ++i) {
                unsigned pos = frame * 320 + i;
                unsigned gain = pos < 80 ? pos : pos >= 1200 ? 1279 - pos : 80;
                pcm[i] = (int16_t)(wave[phase / 1000] * (int)gain / 80);
                phase = (phase + notes[reaction][note]) % 16000;
            }
            ok = tdl_audio_play(codec, (uint8_t *)pcm, sizeof(pcm)) == OPRT_OK;
        }
    }
    if (ok) tal_system_sleep(180);
    return tdl_audio_close(codec) == OPRT_OK && ok;
}
