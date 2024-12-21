#include "SDL_render.h"
#define CLAY_IMPLEMENTATION
#include <clay.h>

#include <stdint.h>
#include <time.h>

#include "SDL_events.h"
#include "SDL_image.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include "clay_renderer_sdl.h"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720
#define FONT_HACK_24 0
#define FONT_HACK_12 1

// https://rosepinetheme.com/
Clay_Color Base = {.r = 25, .g = 23, .b = 36, .a = 255};
Clay_Color Surface = {.r = 31, .g = 29, .b = 46, .a = 255};
Clay_Color OverLay = {.r = 38, .g = 35, .b = 58, .a = 255};
Clay_Color Text = {.r = 224, .g = 222, .b = 244, .a = 255};
Clay_Color Iris = {.r = 196, .g = 167, .b = 231, .a = 255};

Clay_String lorum_long = CLAY_STRING(
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
    "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
    "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
    "mollit anim id est laborum.");
Clay_String lorum_short = CLAY_STRING("Lorem ipsum dolor sit amet");

Clay_String TopLeft[] = {CLAY_STRING("FILE"), CLAY_STRING("EDIT")};
int TopLeft_Len = sizeof(TopLeft) / sizeof(Clay_String);
Clay_String TopRight[] = {CLAY_STRING("ABOUT"), CLAY_STRING("SOMETHING"),
                          CLAY_STRING("ELSE")};
int TopRight_Len = sizeof(TopRight) / sizeof(Clay_String);

int main(int argc, char *argv[]) {

  SDL_Event event;
  uint64_t clayMemSize = Clay_MinMemorySize();
  Clay_Arena arena =
      Clay_CreateArenaWithCapacityAndMemory(clayMemSize, malloc(clayMemSize));
  Clay_SDL_Initialize(DEFAULT_WIDTH, DEFAULT_HEIGHT, "SDL Clay Test App", 0,
                      SDL_WINDOW_RESIZABLE);

  Clay_Initialize(arena, (Clay_Dimensions){.width = DEFAULT_WIDTH,
                                           .height = DEFAULT_HEIGHT});

  Clay_SDL_LoadFont(FONT_HACK_24, "resources/Hack-Regular.ttf", 24);
  Clay_SDL_LoadFont(FONT_HACK_12, "resources/Hack-Regular.ttf", 12);

  IMG_Init(IMG_INIT_PNG);
  SDL_Texture *texture = IMG_LoadTexture(renderer, "resources/SDL_logo.png");
  if (texture == NULL) {
    printf("Failed to load resources/SDL_logo.png\n");
  }

  Clay_SDL_Clear_Color = Base;
  int oldtime = clock();
  while (1) {
    int time = clock();
    float dt = (1.0f / (time - oldtime)) * 1000;
    oldtime = time;
    Clay_Vector2 wheelMove = {0};

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_MOUSEWHEEL: {
        wheelMove.x = event.wheel.x;
        wheelMove.y = event.wheel.y;
        break;
      }
      }
      Clay_SDL_Event(&event);
    }
    Clay_UpdateScrollContainers(true, wheelMove, dt);

    Clay_BeginLayout();

    CLAY(CLAY_ID("Root"),
         CLAY_LAYOUT({
             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
             .padding = {16, 16},
             .childGap = 16,
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
         }),
         CLAY_RECTANGLE(
             {.color = Surface, .cornerRadius = CLAY_CORNER_RADIUS(16)})) {
      CLAY(CLAY_ID("TopBar"),
           CLAY_LAYOUT(
               {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(0.1f)},
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                .childGap = 16}),
           CLAY_RECTANGLE(
               {.color = OverLay, .cornerRadius = CLAY_CORNER_RADIUS(16)})) {
        CLAY(
            CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}})) {
        }
        for (int i = 0; i < TopLeft_Len; i++) {
          CLAY_TEXT(TopLeft[i], CLAY_TEXT_CONFIG({.fontSize = 12,
                                                  .fontId = FONT_HACK_12,
                                                  .textColor = Text}));
        }
        CLAY(
            CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}})) {
        }
        for (int i = 0; i < TopRight_Len; i++) {
          CLAY_TEXT(TopRight[i], CLAY_TEXT_CONFIG({.fontSize = 12,
                                                   .fontId = FONT_HACK_12,
                                                   .textColor = Text}));
        }

        CLAY(
            CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}})) {
        }
      }
      CLAY(CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childGap = 16})) {
        CLAY(CLAY_ID("SideBar"),
             CLAY_LAYOUT(
                 {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                  .sizing = {CLAY_SIZING_PERCENT(0.2f), CLAY_SIZING_GROW()}}),
             CLAY_RECTANGLE(
                 {.color = OverLay, .cornerRadius = CLAY_CORNER_RADIUS(16)})) {
          CLAY(CLAY_ID("Image"),
               CLAY_LAYOUT({.sizing = {.width = CLAY_SIZING_FIXED(179),
                                       .height = CLAY_SIZING_FIXED(99)}}),
               CLAY_IMAGE(
                   {.imageData = texture, {.width = 179, .height = 99}})) {}
        }
        CLAY(CLAY_ID("MainContentOuter"),
             CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}),
             CLAY_RECTANGLE(
                 {.color = OverLay, .cornerRadius = CLAY_CORNER_RADIUS(16)})) {
          CLAY(CLAY_ID("MainContent"),
               CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .padding = {16, 16},
                            .childGap = 16}),
               CLAY_BORDER({.cornerRadius = CLAY_CORNER_RADIUS(16),
                            .right = {.width = 4, .color = Iris},
                            .left = {.width = 4, .color = Iris},
                            .top = {.width = 0, .color = Iris},
                            .bottom = {.width = 0, .color = Iris}})) {
            CLAY_TEXT(lorum_short, CLAY_TEXT_CONFIG({.fontSize = 24,
                                                     .fontId = FONT_HACK_24,
                                                     .textColor = Text}));
            CLAY_TEXT(lorum_long,
                      CLAY_TEXT_CONFIG({.wrapMode = CLAY_TEXT_WRAP_WORDS,
                                        .fontSize = 12,
                                        .fontId = FONT_HACK_12,
                                        .textColor = Text}));
          }
        }
      }
    }

    Clay_RenderCommandArray renderCommands = Clay_EndLayout();
    Clay_SDL_Render(renderCommands);
  }

  return EXIT_SUCCESS;
}
