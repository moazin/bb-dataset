#include <iostream>
#include <fstream>
#include <memory>

#include <SDL2/SDL.h>
#include <cairo.h>

#include <SVGRenderer.h>
#include <SVGDocument.h>
#include <CairoSVGRenderer.h>

typedef struct _State {
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
  cairo_surface_t *recording_surface;
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
  state->recording_surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR, NULL);
  state->cr = cairo_create(state->cairo_surface);
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

void drawSVGDocument(State *state, std::string filename)
{
  auto renderer = std::make_shared<SVGNative::CairoSVGRenderer>();

  std::ifstream svg_file(filename);

  std::string svg_doc = "";
  std::string line;
  while (std::getline(svg_file, line)) {
    svg_doc += line;
  }

  auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(svg_doc.c_str(), renderer));
  renderer->SetCairo(state->cr);
  doc->Render();

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

void zoomOutTransform(State *state)
{
  double width_box = state->x1 - state->x0 + 1;
  double height_box = state->y1 - state->y0 + 1;
  double new_width = width_box * 1.2;
  double new_height = height_box * 1.2;
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
}

void drawRecording(State *state)
{
  cairo_set_source_surface(state->cr, state->recording_surface, 0.0, 0.0);
  cairo_paint(state->cr);
  cairo_surface_flush(state->cairo_surface);
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
  cairo_move_to(state->cr, 10, 20);
  char characters[100];
  sprintf(characters, "Viewbox: %f %f %f %f", state->x0, state->y0, state->x1, state->y1);
  cairo_show_text(state->cr, characters);
  cairo_surface_flush(state->cairo_surface);
  SDL_UpdateWindowSurface(state->window);
  cairo_restore(state->cr);
}

void drawing(State *state, std::string filename){
  double x0, y0, width, height, x1, y1;
  calculateBoundingBox(filename, &x0, &y0, &width, &height);
  x1 = x0 + width - 1;
  y1 = y0 + height - 1;
  drawSVGDocument(state, filename);
  drawRectangle(state, x0, y0, x1, y1, Color{1.0, 0.0, 0.0});
  drawInfoBox(state);
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

  if (initialize(&state, width, height))
    return 1;

  clearCanvas(&state);
  setTransform(&state);
  drawing(&state, std::string(argv[1]));

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
        }
        else if(ke.keysym.scancode == 15)
        {
          unsigned char* data = cairo_image_surface_get_data(state.cairo_surface);
          int width = cairo_image_surface_get_width(state.cairo_surface);
          int height = cairo_image_surface_get_height(state.cairo_surface);
          int pitch = cairo_image_surface_get_stride(state.cairo_surface);
          cairo_t *ct = cairo_create(state.recording_surface);
          printf("%d %d %d\n", width, height, pitch);
          cairo_set_antialias(ct, CAIRO_ANTIALIAS_NONE);
          for(int i = 0; i < height; i++) {
            for(int j = 0; j < width; j++){
              Color color;
              color.r = float(*(data + (i * pitch) + j*4))/255.0;
              color.g = float(*(data + (i * pitch) + j*4 + 1))/255.0;
              color.b = float(*(data + (i * pitch) + j*4 + 2))/255.0;
              cairo_set_source_rgb(ct, color.r, color.g, color.b);
              cairo_set_line_width(ct, 0);
              cairo_rectangle(ct, j, i, 1, 1);
              cairo_fill(ct);
            }
          }
          state.render_recording = true;
          state.x0 = 0;
          state.y0 = 0;
          state.x1 = state.width - 1;
          state.y1 = state.height - 1;
          cairo_destroy(ct);
          clearCanvas(&state);
          setTransform(&state);
          drawRecording(&state);
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
  cairo_surface_destroy(state.recording_surface);
  SDL_DestroyWindow(state.window);
  SDL_Quit();

  return 0;
}
