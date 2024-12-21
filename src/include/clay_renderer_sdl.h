#include "SDL_events.h"
#include <stdint.h>

#include <SDL.h>
#include <clay.h>

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern Clay_Color Clay_SDL_Clear_Color;

void Clay_SDL_Initialize(int width, int height, const char *title,
                         uint32_t subsystem_flags, uint32_t window_flags);
void Clay_SDL_LoadFont(uint16_t fontId, const char *ttf_file, float pt_size);
void Clay_SDL_Render(Clay_RenderCommandArray renderCommands);
void Clay_SDL_Event(SDL_Event *event);
