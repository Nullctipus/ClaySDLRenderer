#include "SDL.h"
#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_mouse.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_ttf.h"
#include "SDL_version.h"
#include "SDL_video.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CLAY_OVERFLOW_TRAP
#include "signal.h"
#endif

#ifndef NDEBUG
#define DEBUG
#endif

#ifndef CLAY_SDL_MAX_FONTS
#define CLAY_SDL_MAX_FONTS 10
#endif

#include <clay.h>

typedef struct {
  uint32_t fontId;
  TTF_Font *font;
} SDL_Font;

SDL_Font SDL_Fonts[CLAY_SDL_MAX_FONTS];
typedef enum {
  TOP_RIGHT = 0b11,
  TOP_LEFT = 0b01,
  BOTTOM_RIGHT = 0b10,
  BOTTOM_LEFT = 0b00,
} Quarter;

typedef union {
  struct {
    uint8_t a, b, g, r;
  };
  uint32_t color;

} SurfaceColor;

#define CLAY_RECTANGLE_TO_SDL_RECT(rectangle)                                  \
  (SDL_Rect) {                                                                 \
    .x = rectangle.x, .y = rectangle.y, .w = rectangle.width,                  \
    .h = rectangle.height                                                      \
  }
#define SDL_RECT_TO_CLAY_RECTANGLE(rect)                                       \
  (Rectangle) { .x = rect.x, .y = rect.y, .width = rect.w, .height = rect.h }
#define CLAY_COLOR_TO_SDL_COLOR(color)                                         \
  (SDL_Color) {                                                                \
                                                                               \
    .r = (uint8_t)roundf(color.r), .g = (uint8_t)roundf(color.g),              \
    .b = (uint8_t)roundf(color.b), .a = (uint8_t)roundf(color.a)               \
  }
#define CLAY_COLOR_TO_SURFACE_COLOR(color)                                     \
  (SurfaceColor) {                                                             \
                                                                               \
    .r = (uint8_t)roundf(color.r), .g = (uint8_t)roundf(color.g),              \
    .b = (uint8_t)roundf(color.b), .a = (uint8_t)roundf(color.a)               \
  }
#define SDL_COLOR_TO_CLAY_COLOR(color)                                         \
  (Clay_Color) {                                                               \
                                                                               \
    .r = (float)color.r, .g = (float)color.g, .b = (float)color.b,             \
    .a = (float)color.a                                                        \
  }
#define TEXT_TO_SURFACE_WRAPPED TTF_RenderUTF8_Blended_Wrapped

static inline Clay_Dimensions SDL_MeasureText(Clay_String *text,
                                              Clay_TextElementConfig *config) {
  char *clone;
  TTF_Font *font;
  int extents, count;
  clone = malloc(text->length + 1);
  memcpy(clone, text->chars, text->length);
  clone[text->length] = 0;
  font = SDL_Fonts[config->fontId].font;

  TTF_MeasureUTF8(font, clone, 0x7fffffff, &extents, &count);

  free(clone);
  return (Clay_Dimensions){.width = extents, .height = config->fontSize};
}

SDL_Window *window;
SDL_Renderer *renderer;
void WriteSurfacePixel(SDL_Surface *surface, int32_t x, int32_t y,
                       uint32_t color) {
  if (x < 0 || x > surface->w || y < 0 || y > surface->h) {
    printf("oob: %d %d of %d %d\n", x, y, surface->w, surface->h);
    return;
  }
  uint32_t *target =
      (uint32_t *)((uint8_t *)surface->pixels + y * surface->pitch +
                   x * surface->format->BytesPerPixel);
  *target = color;
}
// https://en.wikipedia.org/w/index.php?title=Midpoint_circle_algorithm&oldid=889172082#C_example
void DrawSurfaceQuarterCircle(SDL_Surface *surface, uint32_t color, int32_t x0,
                              int32_t y0, int32_t radius, Quarter quarter) {
  int32_t xmul = quarter & 0b10 ? 1 : -1;
  int32_t ymul = quarter & 0b01 ? -1 : 1;

  int32_t diameter = radius * 2;
  int32_t x = radius - 1;
  int32_t y = 0;
  int32_t dx = 1;
  int32_t dy = 1;
  int32_t err = dx - (radius << 1);
  while (x >= y) {
    WriteSurfacePixel(surface, x0 + x * xmul, y0 + y * ymul, color);
    WriteSurfacePixel(surface, x0 + y * xmul, y0 + x * ymul, color);

    if (err <= 0) {
      ++y;
      err += dy;
      dy += 2;
    } else {
      --x;
      dx += 2;
      err += (dx - diameter);
    }
  }
}
void DrawSurfaceRect(SDL_Surface *surface, uint32_t color, SDL_Rect rect) {
  for (int32_t x = rect.x; x < rect.w + rect.x; x++)
    for (int32_t y = rect.y; y < rect.h + rect.y; y++) {
      WriteSurfacePixel(surface, x, y, color);
    }
}
void DrawRoundedRect(SDL_Renderer *renderer, SDL_Rect *rect, uint32_t color,
                     int radius_top_left, int radius_top_right,
                     int radius_bottom_left, int radius_bottom_right) {
  SDL_Surface *surface;
  SDL_Texture *texture;
  surface = SDL_CreateRGBSurface(0, rect->w, rect->h, 32, 0xFF000000,
                                 0x00FF0000, 0x0000FF00, 0x000000FF);
  if (surface == NULL) {
    printf("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
    exit(-1);
  }
  if (surface->pixels == NULL) {
    printf("Surface pixels are null\n");
    exit(-1);
  }
  int halfWidth = rect->w / 2;

  DrawSurfaceRect(
      surface, color,
      (SDL_Rect){.x = 0,
                 .y = radius_top_left,
                 .w = halfWidth,
                 .h = rect->h - radius_top_left - radius_bottom_left});
  DrawSurfaceRect(
      surface, color,
      (SDL_Rect){.x = halfWidth,
                 .y = radius_top_right,
                 .w = halfWidth,
                 .h = rect->h - radius_top_right - radius_bottom_right});
  DrawSurfaceRect(
      surface, color,
      (SDL_Rect){.x = radius_top_left,
                 .y = 0,
                 .w = rect->w - radius_top_left - radius_top_right,
                 .h = CLAY__MAX(radius_top_left, radius_top_right)});
  DrawSurfaceRect(
      surface, color,
      (SDL_Rect){.x = radius_bottom_left,
                 .y = rect->h -
                      CLAY__MAX(radius_bottom_left, radius_bottom_right),
                 .w = rect->w - radius_bottom_left - radius_bottom_right,
                 .h = CLAY__MAX(radius_bottom_left, radius_bottom_right)});
  for (int r = radius_top_left; r > 0; r--)
    DrawSurfaceQuarterCircle(surface, color, radius_top_left - 1,
                             radius_top_left - 1, r, TOP_LEFT);
  for (int r = radius_top_right; r > 0; r--)
    DrawSurfaceQuarterCircle(surface, color, rect->w - radius_top_right,
                             radius_top_right - 1, r, TOP_RIGHT);
  for (int r = radius_bottom_left; r > 0; r--)
    DrawSurfaceQuarterCircle(surface, color, radius_bottom_left - 1,
                             rect->h - radius_bottom_left, r, BOTTOM_LEFT);
  for (int r = radius_bottom_right; r > 0; r--)
    DrawSurfaceQuarterCircle(surface, color, rect->w - radius_bottom_right,
                             rect->h - radius_bottom_right, r, BOTTOM_RIGHT);
  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_RenderCopy(renderer, texture, NULL, rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}
void DrawSurfaceBorder(SDL_Renderer *renderer, SDL_Rect *rect,
                       uint32_t top_color, uint32_t left_color,
                       uint32_t bottom_color, uint32_t right_color,
                       int32_t top_width, int32_t left_width,
                       int32_t bottom_width, int32_t right_width,
                       int radius_top_left, int radius_top_right,
                       int radius_bottom_left, int radius_bottom_right) {
  SDL_Surface *surface;
  SDL_Texture *texture;
  surface = SDL_CreateRGBSurface(0, rect->w, rect->h, 32, 0xFF000000,
                                 0x00FF0000, 0x0000FF00, 0x000000FF);
  if (surface == NULL) {
    printf("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
    exit(-1);
  }
  if (surface->pixels == NULL) {
    printf("Surface pixels are null\n");
    exit(-1);
  }
  if (left_width > 0)
    DrawSurfaceRect(
        surface, left_color,
        (SDL_Rect){.x = 0,
                   .y = radius_top_left,
                   .w = left_width,
                   .h = rect->h - radius_top_left - radius_bottom_left});
  if (right_width > 0)
    DrawSurfaceRect(
        surface, right_color,
        (SDL_Rect){.x = rect->w - right_width,
                   .y = radius_top_right,
                   .w = right_width,
                   .h = rect->h - radius_top_right - radius_bottom_right});
  if (top_width > 0)
    DrawSurfaceRect(
        surface, top_color,
        (SDL_Rect){.x = radius_top_left,
                   .y = 0,
                   .w = rect->w - radius_top_left - radius_top_right,
                   .h = top_width});
  if (bottom_width > 0)
    DrawSurfaceRect(
        surface, bottom_color,
        (SDL_Rect){.x = radius_bottom_left,
                   .y = rect->h - bottom_width,
                   .w = rect->w - radius_bottom_left - radius_bottom_right,
                   .h = bottom_width});
  for (int r = radius_top_left;
       r > radius_top_left - CLAY__MAX(top_width, left_width); r--)
    DrawSurfaceQuarterCircle(surface, top_color, radius_top_left,
                             radius_top_left, r,

                             TOP_LEFT);
  for (int r = radius_top_right;
       r > radius_top_right - CLAY__MAX(top_width, right_width); r--)
    DrawSurfaceQuarterCircle(surface, top_color, rect->w - radius_top_right,
                             radius_top_right, r, TOP_RIGHT);
  for (int r = radius_bottom_left;
       r > radius_bottom_left - CLAY__MAX(bottom_width, left_width); r--)
    DrawSurfaceQuarterCircle(surface, bottom_color, radius_bottom_left,
                             rect->h - radius_bottom_left, r, BOTTOM_LEFT);
  for (int r = radius_bottom_right;
       r > radius_bottom_right - CLAY__MAX(bottom_width, right_width); r--)
    DrawSurfaceQuarterCircle(surface, bottom_color,
                             rect->w - radius_bottom_right,
                             rect->h - radius_bottom_right, r,

                             BOTTOM_RIGHT);

  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_RenderCopy(renderer, texture, NULL, rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void Clay_SDL_Initialize(int width, int height, const char *title,
                         uint32_t subsystem_flags, uint32_t window_flags) {
  if (SDL_Init(subsystem_flags | SDL_INIT_VIDEO) < 0)
    goto SDL_ERROR_OUT;
  window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       width, height, window_flags);
  if (window == NULL) {
    printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    exit(-1);
  }
  renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());

    exit(-1);
  }

  TTF_Init();
  Clay_SetMeasureTextFunction(&SDL_MeasureText);
  SDL_version version;
  SDL_VERSION(&version);
  printf("SDL Loaded %d.%d.%d\n", version.major, version.minor, version.patch);
  return;

SDL_ERROR_OUT:

  printf("%s\n", SDL_GetError());
  exit(EXIT_FAILURE);
}
void Clay_SDL_LoadFont(uint16_t fontId, const char *ttf_file, float pt_size) {
  TTF_Font *font = TTF_OpenFont(ttf_file, pt_size);
  if (font == NULL) {
    printf("TTF_OpenFont Error: %s\n", SDL_GetError());
    exit(-1);
  }
  SDL_Fonts[fontId] = (SDL_Font){.fontId = fontId, .font = font};
}
void SetColor(Clay_Color color) {

  SDL_SetRenderDrawColor(renderer, (uint8_t)roundf(color.r),
                         (uint8_t)roundf(color.g), (uint8_t)roundf(color.b),
                         (uint8_t)roundf(color.a));
}
Clay_Color Clay_SDL_Clear_Color = {0};
void Clay_SDL_Render(Clay_RenderCommandArray renderCommands) {
  SDL_SetRenderDrawColor(renderer, Clay_SDL_Clear_Color.r,
                         Clay_SDL_Clear_Color.g, Clay_SDL_Clear_Color.b,
                         Clay_SDL_Clear_Color.a);
  SDL_RenderClear(renderer);

  for (int32_t i = 0; i < renderCommands.length; i++) {
    Clay_RenderCommand *renderCommand =
        Clay_RenderCommandArray_Get(&renderCommands, i);
    Clay_BoundingBox boundingBox = renderCommand->boundingBox;
    switch (renderCommand->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      SDL_Rect rect;
      SDL_Texture *texture;
      TTF_Font *font;
      char *text;

      text = malloc(renderCommand->text.length + 1);
      memcpy(text, renderCommand->text.chars, renderCommand->text.length);
      text[renderCommand->text.length] = 0;
      font = SDL_Fonts[renderCommand->config.textElementConfig->fontId].font;
      SDL_Surface *surface;

      surface = TEXT_TO_SURFACE_WRAPPED(
          font, text,
          CLAY_COLOR_TO_SDL_COLOR(
              renderCommand->config.textElementConfig->textColor),
          renderCommand->config.textElementConfig->wrapMode ==
                  CLAY_TEXT_WRAP_NEWLINES
              ? 0
              : boundingBox.width);

      texture = SDL_CreateTextureFromSurface(renderer, surface);
      rect.w = surface->w;
      rect.h = surface->h;
      SDL_FreeSurface(surface);
      rect.x = boundingBox.x;
      rect.y = boundingBox.y;
      free(text);
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
      SDL_RenderCopy(renderer, texture, NULL, &rect);
      SDL_DestroyTexture(texture);
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
      SDL_Texture *texture =
          (SDL_Texture *)renderCommand->config.imageElementConfig->imageData;
      SDL_RenderCopy(renderer, texture, NULL,
                     &CLAY_RECTANGLE_TO_SDL_RECT(boundingBox));
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {

      SDL_RenderSetClipRect(renderer, &CLAY_RECTANGLE_TO_SDL_RECT(boundingBox));
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {

      SDL_RenderSetClipRect(renderer, NULL);
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
      Clay_RectangleElementConfig *config =
          renderCommand->config.rectangleElementConfig;
      SetColor(config->color);
      int isrect = config->cornerRadius.topLeft == 0 &&
                   config->cornerRadius.topRight == 0 &&
                   config->cornerRadius.bottomLeft == 0 &&
                   config->cornerRadius.bottomRight == 0;
      if (isrect) {

        SDL_RenderFillRect(renderer, &CLAY_RECTANGLE_TO_SDL_RECT(boundingBox));
        break;
      }

      DrawRoundedRect(renderer, &CLAY_RECTANGLE_TO_SDL_RECT(boundingBox),
                      CLAY_COLOR_TO_SURFACE_COLOR(config->color).color,
                      (int32_t)roundf(config->cornerRadius.topLeft),
                      (int32_t)roundf(config->cornerRadius.topRight),
                      (int32_t)roundf(config->cornerRadius.bottomLeft),
                      (int32_t)roundf(config->cornerRadius.bottomRight));
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_BORDER: {

      Clay_BorderElementConfig *config =
          renderCommand->config.borderElementConfig;

      DrawSurfaceBorder(renderer, &CLAY_RECTANGLE_TO_SDL_RECT(boundingBox),
                        CLAY_COLOR_TO_SURFACE_COLOR(config->top.color).color,
                        CLAY_COLOR_TO_SURFACE_COLOR(config->left.color).color,
                        CLAY_COLOR_TO_SURFACE_COLOR(config->bottom.color).color,
                        CLAY_COLOR_TO_SURFACE_COLOR(config->right.color).color,
                        config->top.width, config->left.width,
                        config->bottom.width, config->right.width,
                        (int32_t)roundf(config->cornerRadius.topLeft),
                        (int32_t)roundf(config->cornerRadius.topRight),
                        (int32_t)roundf(config->cornerRadius.bottomLeft),
                        (int32_t)roundf(config->cornerRadius.bottomRight));
    }
    case CLAY_RENDER_COMMAND_TYPE_NONE: {
      // pass
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
      printf("no custom def\n");
      break;
    }
    default: {
      printf("unhandled render command %d\n", renderCommand->commandType);
#ifdef CLAY_OVERFLOW_TRAP
      raise(SIGTRAP);
#endif
      exit(EXIT_FAILURE);
    }
    }
  }

  SDL_RenderPresent(renderer);
}
Clay_Vector2 mousePos = {0};
bool mouseDown = 0;

void Clay_SDL_Event(SDL_Event *event) {
  switch (event->type) {
  case SDL_QUIT: {

    SDL_Quit();
    break;
  }
  case SDL_WINDOWEVENT: {
    switch (event->window.type) {
    case SDL_WINDOWEVENT_RESIZED:
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
      printf("window event\n");
      Clay_SetLayoutDimensions((Clay_Dimensions){
          .width = event->window.data1, .height = event->window.data2});
      break;
    }
    }
    break;
  }
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP: {

    if (event->button.button != SDL_BUTTON_LEFT)
      break;

    mouseDown = event->type == SDL_MOUSEBUTTONDOWN;
    // FALL THROUGH
  }
  case SDL_MOUSEMOTION: {
    mousePos.x = event->button.x;
    mousePos.y = event->button.y;
    Clay_SetPointerState(mousePos, mouseDown);
    break;
  }

  default:
    break;
  }
}
