#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>

#include <SDL2/SDL.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <librsvg/rsvg.h>

#include <SVGRenderer.h>
#include <SVGDocument.h>
#include <CairoSVGRenderer.h>
#include <core/SkData.h>
#include <core/SkImage.h>
#include <core/SkStream.h>
#include <core/SkSurface.h>
#include <core/SkCanvas.h>
#include <SVGDocument.h>
#include <SkiaSVGRenderer.h>

typedef enum _SVGRenderer {
  SNV = 0,
  LIBRSVG = 1
} SVGRenderer;

typedef enum _GraphicsEngine {
  CAIRO = 0,
  SKIA = 1
} GraphicsEngine;

typedef struct _State {
  std::string filename;
  SVGRenderer renderer;
  GraphicsEngine engine;
  SDL_Window *window;
  SDL_Surface *sdl_surface;
  cairo_surface_t *cairo_surface;
  cairo_t *cr;
  int width;
  int height;
  double x0;
  double y0;
  double x1;
  double y1;
  double scale_x;
  double scale_y;
  bool render_recording;
  GdkPixbuf *pixbuf;
  GdkPixbuf *saved_pixbuf;
  sk_sp<SkSurface> skSurface;
  SkCanvas* skCanvas;
} State;

typedef struct _Color {
  double r;
  double g;
  double b;
} Color;

int initialize(State *state, int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    fprintf(stderr, "SDL failed to initialize: %s\n", SDL_GetError());
    return 1;
  }

  state->window = SDL_CreateWindow("SDL Example",
                            0,
                            0,
                            width,
                            height,
                            0);
  if (state->window == NULL)
  {
    fprintf(stderr, "SDL window failed to initialize: %s\n", SDL_GetError());
    return 1;
  }

  state->sdl_surface = SDL_GetWindowSurface(state->window);
  state->cairo_surface = cairo_image_surface_create_for_data((unsigned char*)state->sdl_surface->pixels,
                                                             CAIRO_FORMAT_RGB24,
                                                             state->sdl_surface->w,
                                                             state->sdl_surface->h,
                                                             state->sdl_surface->pitch);
  state->pixbuf = gdk_pixbuf_new_from_data((const unsigned char*)state->sdl_surface->pixels, GDK_COLORSPACE_RGB, true, 8, state->sdl_surface->w,
                            state->sdl_surface->h, state->sdl_surface->pitch, NULL, NULL);
  unsigned char* data = (unsigned char*)malloc(state->sdl_surface->h * state->sdl_surface->pitch);
  state->saved_pixbuf = gdk_pixbuf_new_from_data((const unsigned char*)data, GDK_COLORSPACE_RGB, true, 8, state->sdl_surface->w,
                                                 state->sdl_surface->h, state->sdl_surface->pitch, NULL, NULL);
  state->cr = cairo_create(state->cairo_surface);

  SkImageInfo skImageInfo = SkImageInfo::Make(state->width, state->height, kBGRA_8888_SkColorType, kOpaque_SkAlphaType, nullptr);
  state->skSurface = SkSurface::MakeRasterDirect(skImageInfo, state->sdl_surface->pixels, state->sdl_surface->pitch, nullptr);
  state->skCanvas = state->skSurface->getCanvas();

  cairo_set_source_rgb(state->cr, 1.0, 1.0, 1.0);
  cairo_move_to(state->cr, 0, 0);
  cairo_line_to(state->cr, 0, 999);
  cairo_line_to(state->cr, 999, 999);
  cairo_line_to(state->cr, 999, 0);
  cairo_close_path(state->cr);
  cairo_fill(state->cr);
  cairo_surface_flush(state->cairo_surface);
  SDL_UpdateWindowSurface(state->window);

  return 0;
}

void clearCanvas(State *state)
{
  cairo_identity_matrix(state->cr);
  cairo_set_source_rgb(state->cr, 1.0, 1.0, 1.0);
  cairo_move_to(state->cr, 0, 0);
  cairo_line_to(state->cr, 0, 999);
  cairo_line_to(state->cr, 999, 999);
  cairo_line_to(state->cr, 999, 0);
  cairo_close_path(state->cr);
  cairo_fill(state->cr);
  cairo_surface_flush(state->cairo_surface);
  SDL_UpdateWindowSurface(state->window);
}

std::vector<SVGNative::RectT> drawSVGDocumentSNVCairo(State *state, std::string svg_doc)
{
  auto renderer = std::make_shared<SVGNative::CairoSVGRenderer>();
  auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svg_doc.c_str(), renderer));
  renderer->SetCairo(state->cr);
  std::vector<SVGNative::RectT> bounding_boxes = doc->Render();
  cairo_surface_flush(state->cairo_surface);
  return bounding_boxes;
}

void drawSVGDocumentSNVSkia(State *state, std::string svg_doc)
{
  auto renderer = std::make_shared<SVGNative::SkiaSVGRenderer>();

  auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svg_doc.c_str(), renderer));

  renderer->SetSkCanvas(state->skCanvas);
  doc->Render();
}

std::vector<SVGNative::RectT> drawSVGDocumentSNV(State *state, std::string svg_doc)
{
  std::vector<SVGNative::RectT> dumb;
  if (state->engine == CAIRO)
  {
    return drawSVGDocumentSNVCairo(state, svg_doc);
  }
  else if(state->engine == SKIA)
  {
    drawSVGDocumentSNVSkia(state, svg_doc);
    return dumb;
  }
  return dumb;
}

std::vector<SVGNative::RectT> drawSVGDocumentLibrsvg(State *state, std::string svg_doc)
{
  std::vector<SVGNative::RectT> dumb;
  GError *error = nullptr;
  RsvgHandle *handle = rsvg_handle_new_from_data((const unsigned char*)svg_doc.c_str(), strlen(svg_doc.c_str()), &error);
  rsvg_handle_render_cairo(handle, state->cr);
  cairo_surface_flush(state->cairo_surface);
  return dumb;
}

std::vector<SVGNative::RectT> drawSVGDocument(State *state, std::string filename)
{
  std::vector<SVGNative::RectT> boxes;
  std::ifstream svg_file(filename);

  std::string svg_doc = "";
  std::string line;

  while (std::getline(svg_file, line)) {
    svg_doc += line;
  }

  if (state->renderer == SNV)
    boxes = drawSVGDocumentSNV(state, svg_doc);
  else if(state->renderer == LIBRSVG)
    boxes = drawSVGDocumentLibrsvg(state, svg_doc);

  svg_file.close();
  SDL_UpdateWindowSurface(state->window);
  return boxes;
}

void drawRectangle(State *state, double x0, double y0, double x1, double y1, Color color) {
  cairo_set_source_rgb(state->cr, color.r, color.g, color.b);
  cairo_set_line_width(state->cr, 0.5);
  cairo_move_to(state->cr, x0, y0);
  cairo_line_to(state->cr, x1, y0);
  cairo_line_to(state->cr, x1, y1);
  cairo_line_to(state->cr, x0, y1);
  cairo_close_path(state->cr);
  cairo_stroke(state->cr);
  cairo_surface_flush(state->cairo_surface);
  SDL_UpdateWindowSurface(state->window);
}

void zoomInTransform(State *state)
{
  double width_box = state->x1 - state->x0 + 1;
  double height_box = state->y1 - state->y0 + 1;
  double new_width = width_box * 0.8;
  double new_height = height_box * 0.8;
  double x_diff = (width_box - new_width)/2.0;
  double y_diff = (height_box - new_height)/2.0;
  state->x0 = state->x0 + x_diff;
  state->y0 = state->y0 + y_diff;
  state->x1 = state->x1 - x_diff;
  state->y1 = state->y1 - y_diff;
}

void resetTransform(State *state)
{
  state->x0 = 0;
  state->y0 = 0;
  state->x1 = state->x0 + state->width - 1;
  state->y1 = state->y0 + state->height - 1;
}

void zoomOutTransform(State *state)
{
  double width_box = state->x1 - state->x0 + 1;
  double height_box = state->y1 - state->y0 + 1;
  double new_width = width_box / 0.8;
  double new_height = height_box / 0.8;
  double x_diff = (new_width - width_box)/2.0;
  double y_diff = (new_height - height_box)/2.0;
  state->x0 = state->x0 - x_diff;
  state->y0 = state->y0 - y_diff;
  state->x1 = state->x1 + x_diff;
  state->y1 = state->y1 + y_diff;
}

void moveTransform(State *state, int x, int y)
{
  double width_box = state->x1 - state->x0 + 1;
  double height_box = state->y1 - state->y0 + 1;
  double dx = width_box * 0.1;
  double dy = height_box * 0.1;
  if (x == 1)
  {
    state->x0 += dx;
    state->x1 += dx;
  }
  if (x == -1)
  {
    state->x0 -= dx;
    state->x1 -= dx;
  }
  if (y == -1)
  {
    state->y0 -= dy;
    state->y1 -= dy;
  }
  if (y == 1)
  {
    state->y0 += dy;
    state->y1 += dy;
  }
}

void setTransform(State *state)
{
  double width_box = state->x1 - state->x0 + 1;
  double height_box = state->y1 - state->y0 + 1;
  double scale_x = state->width/width_box;
  double scale_y = state->height/height_box;
  cairo_scale(state->cr, scale_x, scale_y);
  cairo_translate(state->cr, -1 * state->x0, -1 * state->y0);
  state->skCanvas->resetMatrix();
  state->skCanvas->scale(scale_x, scale_y);
  state->skCanvas->translate(-1 * state->x0, -1 * state->y0);
}

void drawRecording(State *state)
{
  double width_box = state->x1 - state->x0 + 1;
  double height_box = state->y1 - state->y0 + 1;
  double scale_x = state->width/width_box;
  double scale_y = state->height/height_box;
  gdk_pixbuf_scale(state->saved_pixbuf, state->pixbuf, 0, 0, state->width, state->height, -1 * state->x0 * scale_x, -1 * state->y0 * scale_y, scale_x, scale_y, GDK_INTERP_NEAREST);
  SDL_UpdateWindowSurface(state->window);
}

void calculateBoundingBox(std::string filename, double *x0, double *y0, double *width, double *height)
{
  cairo_surface_t *recording_surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR, NULL);
  cairo_t* ct = cairo_create(recording_surface);

  auto renderer = std::make_shared<SVGNative::CairoSVGRenderer>();

  std::ifstream svg_file(filename);

  std::string svg_doc = "";
  std::string line;
  while (std::getline(svg_file, line)) {
    svg_doc += line;
  }

  auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svg_doc.c_str(), renderer));
  renderer->SetCairo(ct);
  doc->Render();

  cairo_recording_surface_ink_extents(recording_surface, x0, y0, width, height);
  cairo_destroy(ct);
  cairo_surface_flush(recording_surface);
  cairo_surface_destroy(recording_surface);
}

void drawInfoBox(State *state)
{
  cairo_save(state->cr);
  cairo_identity_matrix(state->cr);
  cairo_set_source_rgb(state->cr, 0, 0, 0);
  cairo_set_font_size(state->cr, 13);
  char characters[500];

  sprintf(characters, "Viewbox: %f %f %f %f", state->x0, state->y0, state->x1, state->y1);
  cairo_move_to(state->cr, 10, 20);
  cairo_show_text(state->cr, characters);
  if (state->render_recording)
    sprintf(characters, "Rendering Mode: Raster (frozen)");
  else
    sprintf(characters, "Rendering Mode: Vector");
  cairo_move_to(state->cr, 10, 35);
  cairo_show_text(state->cr, characters);

  sprintf(characters, "Filename: %s", state->filename.c_str());
  cairo_move_to(state->cr, 10, 50);
  cairo_show_text(state->cr, characters);

  if (state->renderer == SNV)
    sprintf(characters, "Renderer: SNV");
  else
    sprintf(characters, "Renderer: LIBRSVG");
  cairo_move_to(state->cr, 10, 65);
  cairo_show_text(state->cr, characters);

  if (state->renderer == SNV)
  {
    if (state->engine == CAIRO)
      sprintf(characters, "Graphics Engine: Cairo");
    else
      sprintf(characters, "Graphics Engine: Skia");
    cairo_move_to(state->cr, 10, 80);
    cairo_show_text(state->cr, characters);
  }


  cairo_surface_flush(state->cairo_surface);
  SDL_UpdateWindowSurface(state->window);
  cairo_restore(state->cr);
}

void drawing(State *state, std::string filename){
  /*
  double x0, y0, width, height, x1, y1;
  calculateBoundingBox(filename, &x0, &y0, &width, &height);
  x1 = x0 + width - 1;
  y1 = y0 + height - 1;
  std::vector<SVGNative::RectT> boxes = drawSVGDocument(state, filename);
  for(auto box: boxes) {
    drawRectangle(state, box.x0, box.y0, box.x1, box.y1, Color{1.0, 0.0, 0.0});
    printf("[%f %f %f %f]\n", box.x0, box.y0, box.x1, box.y1);
  }
  */
  cairo_set_source_rgb(state->cr, 0.5, 1.0, 0.5);
  //cairo_identity_matrix(state->cr);
  cairo_rotate(state->cr, -0.2);
  cairo_rectangle(state->cr, 400, 400, 100, 100);
  SVGNative::RectT bbox;
  cairo_fill_extents(state->cr, &bbox.x0, &bbox.y0, &bbox.x1, &bbox.y1);
  cairo_fill(state->cr);
  cairo_identity_matrix(state->cr);
  drawRectangle(state, bbox.x0, bbox.y0, bbox.x1, bbox.y1, Color{1.0, 0.0, 0.0});
  cairo_surface_flush(state->cairo_surface);
}


int main(int argc, char** argv)
{
  if (argc != 2)
    return 1;

  int width = 1000;
  int height = 1000;

  State state;
  state.x0 = 0;
  state.y0 = 0;
  state.x1 = width - 1;
  state.y1 = height - 1;
  state.width = width;
  state.height = height;
  state.scale_x = 1;
  state.scale_y = 1;
  state.render_recording = false;
  state.filename = std::string(argv[1]);
  state.renderer = SNV;
  state.engine = CAIRO;

  if (initialize(&state, width, height))
    return 1;

  clearCanvas(&state);
  setTransform(&state);
  drawing(&state, std::string(argv[1]));
  drawInfoBox(&state);

  SDL_Event event;
  while(1){
    if (SDL_PollEvent(&event)){
      if (event.type == SDL_KEYDOWN)
      {
        SDL_KeyboardEvent ke = event.key;
        if(ke.keysym.scancode == 20)
          break;
        else
          printf("%d\n", ke.keysym.scancode);
      }
    }
  }

  cairo_destroy(state.cr);
  cairo_surface_destroy(state.cairo_surface);
  SDL_DestroyWindow(state.window);
  SDL_Quit();

  return 0;
}
