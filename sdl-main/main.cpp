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
#include <src/core/SkRTree.h>
#include <SkPictureRecorder.h>
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

std::vector<SVGNative::RectT> drawSVGDocumentSNVSkia(State *state, std::string svg_doc)
{
  auto renderer = std::make_shared<SVGNative::SkiaSVGRenderer>();

  auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svg_doc.c_str(), renderer));

  renderer->SetSkCanvas(state->skCanvas);
  return doc->Render();
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
    return drawSVGDocumentSNVSkia(state, svg_doc);
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

void calculateBoundingBoxCairo(std::string filename, double *x0, double *y0, double *width, double *height)
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

void calculateBoundingBoxSkia(std::string filename, double *x0, double *y0, double *width, double *height)
{
  std::ifstream svg_file(filename);
  std::string svg_doc = "";
  std::string line;
  while (std::getline(svg_file, line)) {
    svg_doc += line;
  }

  SkRTreeFactory factory;
  SkPictureRecorder skPictureRecorder;
  SkRect cull = {-1000, -1000, 10000, 10000};
  sk_sp<SkBBoxHierarchy> bbh = factory();
  SkCanvas *canvas = skPictureRecorder.beginRecording(cull, bbh);

  auto renderer = std::make_shared<SVGNative::SkiaSVGRenderer>();
  auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svg_doc.c_str(), renderer));
  renderer->SetSkCanvas(canvas);
  doc->Render();

  SkRect rect;
  sk_sp<SkPicture> pic = skPictureRecorder.finishRecordingAsPicture();
  rect = pic->cullRect();

  *x0 = rect.x();
  *y0 = rect.y();
  *width = rect.width();
  *height = rect.height();

  printf("final skia rec -> [%f %f %f %f]\n", *x0, *y0, *x0 + *width - 1, *y0 + *height - 1);
}

void calculateBoundingBox(std::string filename, double *x0, double *y0, double *width, double *height)
{
  calculateBoundingBoxSkia(filename, x0, y0, width, height);
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
  double x0, y0, width, height, x1, y1;
  calculateBoundingBox(filename, &x0, &y0, &width, &height);
  x1 = x0 + width - 1;
  y1 = y0 + height - 1;
  std::vector<SVGNative::RectT> boxes = drawSVGDocument(state, filename);
  drawRectangle(state, x0, y0, x0 + width - 1, y0 + height - 1, Color{0.0, 1.0, 0.0});
  /*
  for(auto box: boxes) {
    drawRectangle(state, box.x0, box.y0, box.x1, box.y1, Color{0.0, 1.0, 0.0});
    printf("pushed -> [%f %f %f %f]\n", box.x0, box.y0, box.x1, box.y1);
  }
  */
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
        else if(ke.keysym.scancode == 87)
        {
          clearCanvas(&state);
          zoomInTransform(&state);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 86)
        {
          clearCanvas(&state);
          zoomOutTransform(&state);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 82)
        {
          /* up */
          clearCanvas(&state);
          moveTransform(&state, 0, -1);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 80)
        {
          /* left */
          clearCanvas(&state);
          moveTransform(&state, -1, 0);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 81)
        {
          /* down */
          clearCanvas(&state);
          moveTransform(&state, 0, 1);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 79)
        {
          /* right */
          clearCanvas(&state);
          moveTransform(&state, 1, 0);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 15)
        {
          clearCanvas(&state);
          setTransform(&state);
          drawing(&state, std::string(argv[1]));
          unsigned char* c_data = cairo_image_surface_get_data(state.cairo_surface);
          int c_width = cairo_image_surface_get_width(state.cairo_surface);
          int c_height = cairo_image_surface_get_height(state.cairo_surface);
          int c_pitch = cairo_image_surface_get_stride(state.cairo_surface);
          int c_offset = c_pitch / c_width;
          unsigned char* g_data = gdk_pixbuf_get_pixels(state.saved_pixbuf);
          int g_width = gdk_pixbuf_get_width(state.saved_pixbuf);
          int g_height = gdk_pixbuf_get_height(state.saved_pixbuf);
          int g_pitch = gdk_pixbuf_get_rowstride(state.saved_pixbuf);
          int g_offset = g_pitch / g_width;
          for(int i = 0; i < c_height; i++) {
            for(int j = 0; j < c_width; j++){
              *(g_data + (i * g_pitch) + j*g_offset) = *(c_data + (i * c_pitch) + j*4);
              *(g_data + (i * g_pitch) + j*g_offset + 1) = *(c_data + (i * c_pitch) + j*4 + 1);
              *(g_data + (i * g_pitch) + j*g_offset + 2) = *(c_data + (i * c_pitch) + j*4 + 2);
            }
          }
          state.render_recording = true;
          state.x0 = 0;
          state.y0 = 0;
          state.x1 = state.width - 1;
          state.y1 = state.height - 1;
          clearCanvas(&state);
          setTransform(&state);
          drawRecording(&state);
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 24)
        {
          state.render_recording = false;
          state.x0 = 0;
          state.y0 = 0;
          state.x1 = state.width - 1;
          state.y1 = state.height - 1;
          clearCanvas(&state);
          setTransform(&state);
          drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 98)
        {
          clearCanvas(&state);
          resetTransform(&state);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 21)
        {
          if (state.renderer == SNV)
            state.renderer = LIBRSVG;
          else
            state.renderer = SNV;
          clearCanvas(&state);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 23)
        {
          if (state.renderer == SNV)
            state.engine = state.engine == CAIRO ? SKIA : CAIRO;
          clearCanvas(&state);
          setTransform(&state);
          if (state.render_recording)
            drawRecording(&state);
          else
            drawing(&state, std::string(argv[1]));
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 22)
        {
          cairo_surface_write_to_png(state.cairo_surface, "output.png");
          printf("done\n");
        }
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
