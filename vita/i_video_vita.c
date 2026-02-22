/*
 * Chex Quest PS Vita - Video subsystem
 * Handles rendering Doom's 320x200 framebuffer to the Vita's 960x544 screen
 */

#ifdef VITA

#include <SDL2/SDL.h>
#include <string.h>
#include <stdlib.h>
#include "vita_config.h"

// Doom includes
#include "doomdef.h"
#include "v_video.h"
#include "d_event.h"

// SDL objects
static SDL_Window   *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture  *texture = NULL;

// Doom palette
static SDL_Color palette[256];
static uint32_t palette32[256];

// Frame buffer (Doom renders into this)
extern byte *screens[5];

// Pixel buffer for converting 8-bit indexed to 32-bit ARGB
static uint32_t *pixel_buffer = NULL;

/*
 * Initialize the video subsystem
 */
void I_InitGraphicsVita(void) {
    // Initialize SDL Video
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        I_Error("Could not initialize SDL: %s", SDL_GetError());
        return;
    }

    // Create window (Vita only has one display, but SDL needs a window)
    window = SDL_CreateWindow(
        "Chex Quest",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        VITA_SCREEN_W, VITA_SCREEN_H,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        I_Error("Could not create window: %s", SDL_GetError());
        return;
    }

    // Create hardware-accelerated renderer
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        I_Error("Could not create renderer: %s", SDL_GetError());
        return;
    }

    // Set scaling quality - nearest neighbor for that crispy pixel look
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // Create streaming texture for Doom's framebuffer
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREENWIDTH, SCREENHEIGHT);

    if (!texture) {
        I_Error("Could not create texture: %s", SDL_GetError());
        return;
    }

    // Allocate pixel conversion buffer
    pixel_buffer = (uint32_t *)malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(uint32_t));

    if (!pixel_buffer) {
        I_Error("Could not allocate pixel buffer");
        return;
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

/*
 * Set the palette (called when palette changes)
 */
void I_SetPaletteVita(byte *doompalette) {
    int i;
    for (i = 0; i < 256; i++) {
        palette[i].r = *doompalette++;
        palette[i].g = *doompalette++;
        palette[i].b = *doompalette++;
        palette[i].a = 255;

        // Pre-compute 32-bit ARGB values for fast blitting
        palette32[i] = (255 << 24) |
                       (palette[i].r << 16) |
                       (palette[i].g << 8) |
                       (palette[i].b);
    }
}

/*
 * Update the screen - blit Doom's framebuffer to the Vita display
 * This is called every frame
 */
void I_FinishUpdateVita(void) {
    int i;
    int num_pixels = SCREENWIDTH * SCREENHEIGHT;
    byte *src = screens[0];

    if (!pixel_buffer || !texture || !renderer) return;

    // Convert 8-bit indexed color to 32-bit ARGB using lookup table
    // This is the hot path - optimized with simple loop
    for (i = 0; i < num_pixels; i++) {
        pixel_buffer[i] = palette32[src[i]];
    }

    // Update the texture with the converted pixels
    SDL_UpdateTexture(texture, NULL, pixel_buffer, SCREENWIDTH * sizeof(uint32_t));

    // Clear the renderer (for letterboxing areas)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

#if VITA_STRETCH_FULL
    // Stretch to fill entire screen (slight vertical stretch)
    SDL_RenderCopy(renderer, texture, NULL, NULL);
#else
    // Pixel-perfect 3x scaling with letterbox
    SDL_Rect dest;
    dest.x = 0;
    dest.y = VITA_OFFSET_Y;
    dest.w = VITA_SCALED_W;
    dest.h = VITA_SCALED_H;
    SDL_RenderCopy(renderer, texture, NULL, &dest);
#endif

    // Present to screen
    SDL_RenderPresent(renderer);
}

/*
 * Start frame rendering (called at beginning of each frame)
 */
void I_StartFrameVita(void) {
    // Nothing needed - Vita handles frame timing via vsync
}

/*
 * Start tic processing
 */
void I_StartTicVita(void) {
    // Poll input
    extern void I_PollVitaInput(void);
    I_PollVitaInput();

    // Process SDL events (for compatibility)
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            I_Error("Application quit");
        }
    }
}

/*
 * Shutdown video subsystem
 */
void I_ShutdownGraphicsVita(void) {
    if (pixel_buffer) {
        free(pixel_buffer);
        pixel_buffer = NULL;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}

/*
 * Set window title (no-op on Vita but might be called)
 */
void I_SetWindowTitle(const char *title) {
    if (window) {
        SDL_SetWindowTitle(window, title);
    }
}

/*
 * Read screen data (for screenshots)
 */
void I_ReadScreenVita(byte *scr) {
    memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

#endif /* VITA */
