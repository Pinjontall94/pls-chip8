#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <float.h>
#include "chip8.c"

static const int SAMPLE_RATE = 44100;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;
static int current_sine_sample = 0;

static void SDLCALL FeedTheAudioStreamMore(void *userdata, SDL_AudioStream *astream, int additional_amount, int total_amount)
{
    additional_amount /= sizeof (float);  /* convert from bytes to samples */
    while (additional_amount > 0) {
        float samples[128];  /* this will feed 128 samples each iteration until we have enough. */
        const int total = SDL_min(additional_amount, SDL_arraysize(samples));
        float dphase, phase;
        int i, freq;

        freq = 220;
        phase = 0.00;
        dphase = (float)freq / SAMPLE_RATE; /* 440 Hz / 44.1 kHz = 0.009977 */

        /* generate a 440Hz tone */
        for (i = 0; i < total; i++) {
            samples[i] = SDL_sinf(2 * SDL_PI_F * phase);
            phase += dphase;
            if (phase >= 1.0)
                phase -= 1.0;
            printf("%f\t", phase);
        }

        /* wrapping around to avoid floating-point errors */
        current_sine_sample %= SAMPLE_RATE;

        /* feed the new data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
        SDL_PutAudioStreamData(astream, samples, total * sizeof (float));
        additional_amount -= total;  /* subtract what we've just fed the stream. */
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_AudioSpec spec;

    SDL_SetAppMetadata("Example Simple Audio Playback Callback", "1.0", "com.example.audio-simple-playback-callback");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/audio/simple-playback-callback", 640, 480, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    spec.channels = 1;
    spec.format = SDL_AUDIO_F32LE;
    spec.freq = SAMPLE_RATE;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedTheAudioStreamMore, NULL);
    if (!stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ResumeAudioStreamDevice(stream);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    printf("\n");
}
