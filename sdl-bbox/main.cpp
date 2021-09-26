#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>

#include <SDL2/SDL.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <librsvg/rsvg.h>

#include <svgnative/SVGRenderer.h>
#include <svgnative/SVGDocument.h>
#include <svgnative/ports/cairo/CairoSVGRenderer.h>
#include <svgnative/ports/skia/SkiaSVGRenderer.h>
#include <core/SkData.h>
#include <core/SkImage.h>
#include <core/SkStream.h>
#include <core/SkSurface.h>
#include <core/SkCanvas.h>
#include <src/core/SkRTree.h>
#include <SkPictureRecorder.h>

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
  std::vector<std::string> test_names;
  int total_tests;
  int current_test;
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

  state->test_names.push_back(std::string("Cairo test 1 - Simple Rectangle Fill"));
  state->test_names.push_back(std::string("Cairo test 2 - Simple Rectangle Stroke"));
  state->test_names.push_back(std::string("Cairo test 3 - Simple Cubic Bezier Fill"));
  state->test_names.push_back(std::string("Cairo test 4 - Simple Cubic Bezier Stroke Bevel"));
  state->test_names.push_back(std::string("Cairo test 5 - Simple Cubic Bezier Stroke Round"));
  state->test_names.push_back(std::string("Cairo test 6 - Simple Cubic Bezier Stroke Miter (20)"));
  state->test_names.push_back(std::string("Cairo test 7 - Simple Arc Fill"));
  state->test_names.push_back(std::string("Cairo test 8 - Rectangle Rotate Fill"));
  state->test_names.push_back(std::string("Cairo test 9 - Clipping of Shape Simple"));
  state->test_names.push_back(std::string("Cairo test 10 - Bounds of Curves with thick strokes Butt"));
  state->test_names.push_back(std::string("Cairo test 11 - Bounds of Curves with thick strokes Square"));
  state->test_names.push_back(std::string("Cairo test 12 - Bounds of Curves with thick strokes Round"));
  state->test_names.push_back(std::string("Skia test 1 - Simple Rectangle Fill"));
  state->test_names.push_back(std::string("Skia test 2(a) - Simple Rectangle Stroke Miter"));
  state->test_names.push_back(std::string("Skia test 2(b) - Simple Rectangle Stroke Round"));
  state->test_names.push_back(std::string("Skia test 3 - Simple Cubic Bezier Fill"));
  state->test_names.push_back(std::string("Skia test 4 - Simple Cubic Bezier Stroke Bevel"));
  state->test_names.push_back(std::string("Skia test 5 - Simple Cubic Bezier Stroke Round"));
  state->test_names.push_back(std::string("Skia test 6 - Simple Cubic Bezier Stroke Miter (20)"));
  state->test_names.push_back(std::string("Skia test 7 - Simple Arc Fill"));
  state->test_names.push_back(std::string("Skia test 8 - Rectangle Rotate Fill"));
  state->test_names.push_back(std::string("Skia test 9 - Rectangle Rotate Stroke"));
  state->test_names.push_back(std::string("Skia test 10 - Clipping of Shape Simple"));
  state->test_names.push_back(std::string("Skia test 11 - Bounds of Curves with thick strokes Butt"));
  state->test_names.push_back(std::string("Skia test 12 - Bounds of Curves with thick strokes Square"));
  state->test_names.push_back(std::string("Skia test 13 - Bounds of Curves with thick strokes Round"));
  state->total_tests = 26;
  state->current_test = 0;
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

  sprintf(characters, "Test: %s", state->test_names[state->current_test].c_str());
  cairo_move_to(state->cr, 10, 20);
  cairo_show_text(state->cr, characters);

  sprintf(characters, "Test Number: %d", state->current_test);
  cairo_move_to(state->cr, 10, 35);
  cairo_show_text(state->cr, characters);

  sprintf(characters, "Total Tests: %d", state->total_tests);
  cairo_move_to(state->cr, 10, 50);
  cairo_show_text(state->cr, characters);

  cairo_surface_flush(state->cairo_surface);
  SDL_UpdateWindowSurface(state->window);
  cairo_restore(state->cr);
}

void SkiaTestRectangleFill(State *state){
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.lineTo(400, 400);
  path.lineTo(100, 400);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);

  SkRect tightBounds = path.computeTightBounds();

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestRectangleStrokeMiter(State *state){
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.lineTo(400, 400);
  path.lineTo(100, 400);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);
  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(40);
  stroke_paint.setStrokeJoin(SkPaint::kDefault_Join);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();

  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestRectangleStrokeRound(State *state){
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.lineTo(400, 400);
  path.lineTo(100, 400);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);
  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(40);
  stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();

  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestCubicFill(State *state)
{
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.cubicTo(800, 100, 800, 400, 400, 400);
  path.lineTo(800, 400);
  path.cubicTo(400, 400, 400, 800, 800, 800);
  path.cubicTo(800, 900, 100, 900, 100, 800);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);

  SkRect tightBounds = path.computeTightBounds();

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestCubicStrokeBevel(State *state)
{
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.cubicTo(800, 100, 800, 400, 400, 400);
  path.lineTo(800, 400);
  path.cubicTo(400, 400, 400, 800, 800, 800);
  path.cubicTo(800, 900, 100, 900, 100, 800);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);
  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(50);
  stroke_paint.setStrokeJoin(SkPaint::kBevel_Join);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();

  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestCubicStrokeRound(State *state)
{
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.cubicTo(800, 100, 800, 400, 400, 400);
  path.lineTo(800, 400);
  path.cubicTo(400, 400, 400, 800, 800, 800);
  path.cubicTo(800, 900, 100, 900, 100, 800);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);
  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(50);
  stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();

  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestCubicStrokeMiter(State *state)
{
  SkPath path;
  path.moveTo(100, 100);
  path.lineTo(400, 100);
  path.cubicTo(800, 100, 800, 400, 400, 400);
  path.lineTo(800, 400);
  path.cubicTo(400, 400, 400, 800, 800, 800);
  path.cubicTo(800, 900, 100, 900, 100, 800);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);
  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(20);
  stroke_paint.setStrokeMiter(10);
  stroke_paint.setStrokeJoin(SkPaint::kMiter_Join);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();

  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestArcFill(State *state)
{
  SkPath path;
  path.moveTo(100, 100);
  path.arcTo(50, 100, 0, SkPath::kLarge_ArcSize, SkPathDirection::kCW, 100, 280);
  path.close();
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(path, fill_paint);

  SkRect tightBounds = path.computeTightBounds();

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestRectangleRotateFill(State *state)
{
  // prepare a rotate transform matrix
  SkMatrix matrix;
  matrix.setRotate(45, 500, 500);

  SkPath path;
  SkPath new_path;
  path.moveTo(400, 400);
  path.lineTo(600, 400);
  path.lineTo(600, 600);
  path.lineTo(400, 600);
  path.close();
  path.transform(matrix, &new_path);
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(new_path, fill_paint);

  SkRect tightBounds = new_path.computeTightBounds();

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestRectangleRotateStroke(State *state)
{
  // prepare a rotate transform matrix
  SkMatrix matrix;
  matrix.setRotate(45, 500, 500);

  SkPath path;
  SkPath new_path;
  path.moveTo(400, 400);
  path.lineTo(600, 400);
  path.lineTo(600, 600);
  path.lineTo(400, 600);
  path.close();
  path.transform(matrix, &new_path);
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->drawPath(new_path, fill_paint);
  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(20);
  stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
  state->skCanvas->drawPath(new_path, stroke_paint);

  SkRect tightBounds = new_path.computeTightBounds();
  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }


  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestClippingSimple(State *state)
{
  SkPath clip_path;
  clip_path.moveTo(100, 100);
  clip_path.cubicTo(700, 100, 700, 500, 100, 500);
  clip_path.close();
  SkPath path;
  path.moveTo(700, 100);
  path.cubicTo(100, 100, 100, 500, 700, 500);
  path.close();


  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SK_ColorRED);
  fill_paint.setStyle(SkPaint::kFill_Style);
  state->skCanvas->save();
  state->skCanvas->clipPath(clip_path, true);
  state->skCanvas->drawPath(path, fill_paint);
  state->skCanvas->restore();

  SkRect tightBounds = path.computeTightBounds();
  SkRect tightBoundsClipPath = clip_path.computeTightBounds();


  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);
  state->skCanvas->drawRect(tightBoundsClipPath, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestStrokedCurveButt(State *state)
{
  SkPath path;
  path.moveTo(100, 500);
  path.cubicTo(300, 400, 300, 600, 400, 500);

  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(40);
  stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
  stroke_paint.setStrokeCap(SkPaint::kButt_Cap);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();
  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestStrokedCurveSquare(State *state)
{
  SkPath path;
  path.moveTo(100, 500);
  path.cubicTo(300, 400, 300, 600, 400, 500);

  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(40);
  stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
  stroke_paint.setStrokeCap(SkPaint::kSquare_Cap);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();
  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

void SkiaTestStrokedCurveRound(State *state)
{
  SkPath path;
  path.moveTo(100, 500);
  path.cubicTo(300, 400, 300, 600, 400, 500);

  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setColor(SK_ColorGREEN);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(40);
  stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
  stroke_paint.setStrokeCap(SkPaint::kRound_Cap);
  state->skCanvas->drawPath(path, stroke_paint);

  SkRect tightBounds = path.computeTightBounds();
  if (stroke_paint.canComputeFastBounds()) {
    tightBounds = stroke_paint.computeFastBounds(tightBounds, &tightBounds);
  }

  SkPaint bbox_paint;
  bbox_paint.setAntiAlias(true);
  bbox_paint.setColor(SK_ColorBLUE);
  bbox_paint.setStyle(SkPaint::kStroke_Style);
  state->skCanvas->drawRect(tightBounds, bbox_paint);

  SDL_UpdateWindowSurface(state->window);
}

// Simple Rectangle from (100, 100) -> (399, 399)
// filled with color Red
void CairoTestRectangleFill(State *state){
  cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 1, 0, 0);
  cairo_pattern_add_color_stop_rgb(pattern, 0, 1.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgb(pattern, 1, 0.0, 0.0, 1.0);
  cairo_rectangle(state->cr, 100, 100, 300, 300);
  cairo_set_source(state->cr, pattern);

  double x0, y0, x1, y1;
  cairo_identity_matrix(state->cr);
  cairo_fill_extents(state->cr, &x0, &y0, &x1, &y1);
  printf("%f %f %f %f\n", x0, y0, x1, y1);
  cairo_fill(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// Simple Rectangle from (100, 100) -> (399, 399)
// Filled with Red color
// Stroked with green color of width 40
void CairoTestRectangleStroke(State *state){
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_rectangle(state->cr, 100, 100, 300, 300);
  cairo_fill(state->cr);
  cairo_set_line_width(state->cr, 40);
  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_rectangle(state->cr, 100, 100, 300, 300);
  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);
  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// Cubic Bezier Curve (100, 100) l (400, 100) c (800, 100) (800, 400) (400, 400)
// l (800, 400) c (400, 400) (400, 800) (800, 800) c (800, 900) (100, 900) (100, 800) Z
// Filled with Red, no stroke
void CairoTestCubicFill(State *state)
{
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_move_to(state->cr, 100, 100);
  cairo_line_to(state->cr, 400, 100);
  cairo_curve_to(state->cr, 800, 100, 800, 400, 400, 400);
  cairo_line_to(state->cr, 800, 400);
  cairo_curve_to(state->cr, 400, 400, 400, 800, 800, 800);
  cairo_curve_to(state->cr, 800, 900, 100, 900, 100, 800);
  cairo_close_path(state->cr);

  double x0, y0, x1, y1;
  cairo_fill_extents(state->cr, &x0, &y0, &x1, &y1);

  cairo_fill(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// Same Cubic Bezier but with a stroke of width 50 and join being
// bevel and color being green
void CairoTestCubicStrokeBevel(State *state)
{
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_move_to(state->cr, 100, 100);
  cairo_line_to(state->cr, 400, 100);
  cairo_curve_to(state->cr, 800, 100, 800, 400, 400, 400);
  cairo_line_to(state->cr, 800, 400);
  cairo_curve_to(state->cr, 400, 400, 400, 800, 800, 800);
  cairo_curve_to(state->cr, 800, 900, 100, 900, 100, 800);
  cairo_close_path(state->cr);
  cairo_fill(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 50);
  cairo_set_line_join(state->cr, CAIRO_LINE_JOIN_BEVEL);
  cairo_move_to(state->cr, 100, 100);
  cairo_line_to(state->cr, 400, 100);
  cairo_curve_to(state->cr, 800, 100, 800, 400, 400, 400);
  cairo_line_to(state->cr, 800, 400);
  cairo_curve_to(state->cr, 400, 400, 400, 800, 800, 800);
  cairo_curve_to(state->cr, 800, 900, 100, 900, 100, 800);
  cairo_close_path(state->cr);

  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);

  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// Same Cubic Bezier but with a stroke of width 50 and join being
// round and color being green
void CairoTestCubicStrokeRound(State *state)
{
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_move_to(state->cr, 100, 100);
  cairo_line_to(state->cr, 400, 100);
  cairo_curve_to(state->cr, 800, 100, 800, 400, 400, 400);
  cairo_line_to(state->cr, 800, 400);
  cairo_curve_to(state->cr, 400, 400, 400, 800, 800, 800);
  cairo_curve_to(state->cr, 800, 900, 100, 900, 100, 800);
  cairo_close_path(state->cr);
  cairo_fill(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 50);
  cairo_set_line_join(state->cr, CAIRO_LINE_JOIN_ROUND);
  cairo_move_to(state->cr, 100, 100);
  cairo_line_to(state->cr, 400, 100);
  cairo_curve_to(state->cr, 800, 100, 800, 400, 400, 400);
  cairo_line_to(state->cr, 800, 400);
  cairo_curve_to(state->cr, 400, 400, 400, 800, 800, 800);
  cairo_curve_to(state->cr, 800, 900, 100, 900, 100, 800);
  cairo_close_path(state->cr);

  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);

  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// A simple triangle that's filled and stroked with a miter join
// (500, 500) to (550, 200) to (600, 500) filled with Red
// stroked with Miter join with miter limit being 100 and stroke width
// being 20 and stroke color bieng green
void CairoTestCubicStrokeMiter(State *state)
{
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_move_to(state->cr, 500, 500);
  cairo_line_to(state->cr, 550, 200);
  cairo_line_to(state->cr, 600, 500);
  cairo_close_path(state->cr);
  cairo_fill(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 20);
  cairo_set_miter_limit(state->cr, 100);
  cairo_set_line_join(state->cr, CAIRO_LINE_JOIN_MITER);
  cairo_move_to(state->cr, 500, 500);
  cairo_line_to(state->cr, 550, 200);
  cairo_line_to(state->cr, 600, 500);
  cairo_close_path(state->cr);

  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);

  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// An arc with a red fill formed by doing a circular arc and then
// doing a scale transform on it
void CairoTestArcFill(State *state)
{
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_scale(state->cr, 1.5, 0.8);
  cairo_arc(state->cr, 500, 500, 100, M_PI/4, M_PI + M_PI/8);
  cairo_close_path(state->cr);
  double x0, y0, x1, y1;
  cairo_fill_extents(state->cr, &x0, &y0, &x1, &y1);
  cairo_fill(state->cr);


  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

// A rectangle that's filled with red and rotated to demonstrate
// the problem with Cairo when a rotate transform is applied
void CairoTestRectangleRotateFill(State *state)
{
  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_translate(state->cr, 500, 500);
  cairo_rotate(state->cr, M_PI / 4);
  cairo_translate(state->cr, -500, -500);
  cairo_rectangle(state->cr, 400, 400, 200, 200);
  double x0, y0, x1, y1;
  cairo_identity_matrix(state->cr);
  cairo_fill_extents(state->cr, &x0, &y0, &x1, &y1);
  cairo_fill(state->cr);

  printf("bounds -> [%f %f %f %f]\n", x0, y0, x1, y1);
  cairo_identity_matrix(state->cr);
  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

void CairoTestClippingSimple(State *state)
{
  cairo_reset_clip(state->cr);
  cairo_move_to(state->cr, 100, 100);
  cairo_curve_to(state->cr, 700, 100, 700, 500, 100, 500);
  cairo_close_path(state->cr);
  cairo_clip(state->cr);

  cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
  cairo_move_to(state->cr, 700, 100);
  cairo_curve_to(state->cr, 100, 100, 100, 500, 700, 500);
  cairo_close_path(state->cr);

  double path_x0, path_y0, path_x1, path_y1;
  cairo_fill_extents(state->cr, &path_x0, &path_y0, &path_x1, &path_y1);
  double clip_x0, clip_y0, clip_x1, clip_y1;
  cairo_clip_extents(state->cr, &clip_x0, &clip_y0, &clip_x1, &clip_y1);

  cairo_fill(state->cr);

  cairo_reset_clip(state->cr);
  cairo_set_source_rgb(state->cr, 0.0, 0.0, 1.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, clip_x0, clip_y0, clip_x1 - clip_x0 + 1, clip_y1 - clip_y0 + 1);
  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_rectangle(state->cr, path_x0, path_y0, path_x1 - path_x0 + 1, path_y1 - path_y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

void CairoTestStrokedCurveButt(State *state)
{
  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 40);
  cairo_set_line_join(state->cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(state->cr, CAIRO_LINE_CAP_BUTT);
  cairo_move_to(state->cr, 100, 500);
  cairo_curve_to(state->cr, 300, 400, 300, 600, 400, 500);

  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);
  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

void CairoTestStrokedCurveSquare(State *state)
{
  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 40);
  cairo_set_line_join(state->cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(state->cr, CAIRO_LINE_CAP_SQUARE);
  cairo_move_to(state->cr, 100, 500);
  cairo_curve_to(state->cr, 300, 400, 300, 600, 400, 500);

  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);
  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

void CairoTestStrokedCurveRound(State *state)
{
  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 40);
  cairo_set_line_join(state->cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(state->cr, CAIRO_LINE_CAP_ROUND);
  cairo_move_to(state->cr, 100, 500);
  cairo_curve_to(state->cr, 300, 400, 300, 600, 400, 500);

  double x0, y0, x1, y1;
  cairo_stroke_extents(state->cr, &x0, &y0, &x1, &y1);
  cairo_stroke(state->cr);

  cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
  cairo_set_line_width(state->cr, 1);
  cairo_rectangle(state->cr, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  cairo_stroke(state->cr);

  SDL_UpdateWindowSurface(state->window);
}

void drawing(State *state){
  switch(state->current_test) {
    case 0:
      CairoTestRectangleFill(state);
      break;
    case 1:
      CairoTestRectangleStroke(state);
      break;
    case 2:
      CairoTestCubicFill(state);
      break;
    case 3:
      CairoTestCubicStrokeBevel(state);
      break;
    case 4:
      CairoTestCubicStrokeRound(state);
      break;
    case 5:
      CairoTestCubicStrokeMiter(state);
      break;
    case 6:
      CairoTestArcFill(state);
      break;
    case 7:
      CairoTestRectangleRotateFill(state);
      break;
    case 8:
      CairoTestClippingSimple(state);
      break;
    case 9:
      CairoTestStrokedCurveButt(state);
      break;
    case 10:
      CairoTestStrokedCurveSquare(state);
      break;
    case 11:
      CairoTestStrokedCurveRound(state);
      break;
    case 12:
      SkiaTestRectangleFill(state);
      break;
    case 13:
      SkiaTestRectangleStrokeMiter(state);
      break;
    case 14:
      SkiaTestRectangleStrokeRound(state);
      break;
    case 15:
      SkiaTestCubicFill(state);
      break;
    case 16:
      SkiaTestCubicStrokeBevel(state);
      break;
    case 17:
      SkiaTestCubicStrokeRound(state);
      break;
    case 18:
      SkiaTestCubicStrokeMiter(state);
      break;
    case 19:
      SkiaTestArcFill(state);
      break;
    case 20:
      SkiaTestRectangleRotateFill(state);
      break;
    case 21:
      SkiaTestRectangleRotateStroke(state);
      break;
    case 22:
      SkiaTestClippingSimple(state);
      break;
    case 23:
      SkiaTestStrokedCurveButt(state);
      break;
    case 24:
      SkiaTestStrokedCurveSquare(state);
      break;
    case 25:
      SkiaTestStrokedCurveRound(state);
      break;
  }
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
  drawing(&state);
  drawInfoBox(&state);

  SDL_Event event;
  while(1){
    if (SDL_PollEvent(&event)){
      if (event.type == SDL_KEYDOWN)
      {
        SDL_KeyboardEvent ke = event.key;
        if(ke.keysym.scancode == 20)
          break;
        else if(ke.keysym.scancode == 79)
        {
          // right
          state.current_test = (state.current_test + 1) % state.total_tests;
          clearCanvas(&state);
          setTransform(&state);
          drawing(&state);
          drawInfoBox(&state);
        }
        else if(ke.keysym.scancode == 80)
        {
          // left
          state.current_test -= 1;
          if (state.current_test < 0)
            state.current_test = state.total_tests - 1;
          clearCanvas(&state);
          setTransform(&state);
          drawing(&state);
          drawInfoBox(&state);
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
