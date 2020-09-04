// stdlib
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <fstream>
#include <exception>
#include <functional>

// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>

// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Shaders
#include "shader_program.h"

//Global constants
static const int num_params = 18;
static const int iters = 800;
static const int steps_per_frame = 500;
static const double delta_per_step = 1e-5;
static const double delta_minimum = 1e-7;
static const double t_start = -3.0;
static const double t_end = 3.0;
static const bool fullscreen = false;

//Global variables
static int window_w = 1600;
static int window_h = 900;
static int window_bits = 24;
static float plot_scale = 0.25f;
static float plot_x = 0.0f;
static float plot_y = 0.0f;
static std::mt19937 rand_gen;
static std::string equ_code;
static std::function<void(int, int)> keyhandler;

#define IMGUI_BOX (                         \
      ImGuiWindowFlags_NoDecoration         \
    | ImGuiWindowFlags_AlwaysAutoResize     \
    | ImGuiWindowFlags_NoSavedSettings      \
    | ImGuiWindowFlags_NoFocusOnAppearing   \
    | ImGuiWindowFlags_NoNav)

struct VertexColour {
  GLfloat r, g, b, a;
  VertexColour() = default;
  VertexColour(int r, int g, int b, int a)
    : r{r/255.f}, g{g/255.f}, b{b/255.f}, a{a/255.f} {}
} __attribute__((__packed__));

struct VertexPos {
  GLfloat x, y;
} __attribute__((__packed__));

static VertexColour GetRandColor(int i) {
  i += 1;
  int r = std::min(255, 50 + (i * 11909) % 256);
  int g = std::min(255, 50 + (i * 52973) % 256);
  int b = std::min(255, 50 + (i * 44111) % 256);
  return VertexColour(r, g, b, 16);
}

static VertexPos ToScreen(double x, double y) {
  const float s = plot_scale * float(window_h / 2);
  const float nx = float(window_w) * 0.5f + (float(x) - plot_x) * s;
  const float ny = float(window_h) * 0.5f + (float(y) - plot_y) * s;
  return VertexPos{nx, ny};
}

static void RandParams(double* params) {
  std::uniform_int_distribution<int> rand_int(0, 3);
  for (int i = 0; i < num_params; ++i) {
    const int r = rand_int(rand_gen);
    if (r == 0) {
      params[i] = 1.0f;
    } else if (r == 1) {
      params[i] = -1.0f;
    } else {
      params[i] = 0.0f;
    }
  }
}

static std::string ParamsToString(const double* params) {
  const char base27[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static_assert(num_params % 3 == 0, "Params must be a multiple of 3");
  int a = 0;
  int n = 0;
  std::string result;
  for (int i = 0; i < num_params; ++i) {
    a = a*3 + int(params[i]) + 1;
    n += 1;
    if (n == 3) {
      result += base27[a];
      a = 0;
      n = 0;
    }
  }
  return result;
}

static void StringToParams(const std::string& str, double* params) {
  for (int i = 0; i < num_params/3; ++i) {
    int a = 0;
    const char c = (i < str.length() ? str[i] : '_');
    if (c >= 'A' && c <= 'Z') {
      a = int(c - 'A') + 1;
    } else if (c >= 'a' && c <= 'z') {
      a = int(c - 'a') + 1;
    }
    params[i*3 + 2] = double(a % 3) - 1.0;
    a /= 3;
    params[i*3 + 1] = double(a % 3) - 1.0;
    a /= 3;
    params[i*3 + 0] = double(a % 3) - 1.0;
  }
}

#define SIGN_OR_SKIP(i, x)                  \
  if (params[i] != 0.0) {                   \
    if (isFirst) {                          \
      if (params[i] == -1.0) ss << "-";     \
    } else {                                \
      if (params[i] == -1.0) ss << " - ";   \
      else ss << " + ";                     \
    }                                       \
    ss << x;                                \
    isFirst = false;                        \
  }

static std::string MakeEquationStr(double* params) {
  std::stringstream ss;
  bool isFirst = true;
  SIGN_OR_SKIP(0, "x\u00b2");
  SIGN_OR_SKIP(1, "y\u00b2");
  SIGN_OR_SKIP(2, "t\u00b2");
  SIGN_OR_SKIP(3, "xy");
  SIGN_OR_SKIP(4, "xt");
  SIGN_OR_SKIP(5, "yt");
  SIGN_OR_SKIP(6, "x");
  SIGN_OR_SKIP(7, "y");
  SIGN_OR_SKIP(8, "t");
  return ss.str();
}

static void ResetPlot() {
  plot_scale = 0.25f;
  plot_x = 0.0f;
  plot_y = 0.0f;
}

static std::function<void()> GenerateNew(GLFWwindow* window, double& t, double* params) {
  t = t_start;
  return [=]{
    equ_code = ParamsToString(params);
    const std::string equation_str =
      "x' = " + MakeEquationStr(params) + "\n"
      "y' = " + MakeEquationStr(params + num_params / 2) + "\n"
      "Code: " + equ_code;
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
    bool open = true;
    ImGui::Begin("Equ", &open, IMGUI_BOX);
    ImGui::Text(equation_str.c_str());
    ImGui::End();
  };
}

static void MakeTText(double t) {
  bool open = true;
  ImGui::Begin("Time", &open, IMGUI_BOX);
  ImGui::Text("t = %f", t);
  ImGui::SetWindowPos(ImVec2(window_w - ImGui::GetWindowWidth() - 10.0f, 10.0f));
  ImGui::End();
}

static GLFWwindow* CreateRenderWindow() {
  //Setup OpenGL
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  //Create the window
  GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Chaos Equations", NULL, NULL);
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
    window_w = width;
    window_h = height;
    glViewport(0, 0, window_w, window_h);
    //Bound texture should be the framebuffer
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_w, window_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  });
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Failed to init GLAD");
  }

  return window;
}

static void CenterPlot(const std::vector<glm::vec2>& history) {
  float min_x = FLT_MAX;
  float max_x = -FLT_MAX;
  float min_y = FLT_MAX;
  float max_y = -FLT_MAX;
  for (size_t i = 0; i < history.size(); ++i) {
    min_x = std::fmin(min_x, history[i].x);
    max_x = std::fmax(max_x, history[i].x);
    min_y = std::fmin(min_y, history[i].y);
    max_y = std::fmax(max_y, history[i].y);
  }
  max_x = std::fmin(max_x, 4.0f);
  max_y = std::fmin(max_y, 4.0f);
  min_x = std::fmax(min_x, -4.0f);
  min_y = std::fmax(min_y, -4.0f);
  plot_x = (max_x + min_x) * 0.5f;
  plot_y = (max_y + min_y) * 0.5f;
  plot_scale = 1.0f / std::max(std::max(max_x - min_x, max_y - min_y) * 0.6f, 0.1f);
}

static void ImguiSetup(GLFWwindow* window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");
}

static void NewFrame() {
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

int main(int argc, char *argv[]) {
  std::cout << "=========================================================" << std::endl;
  std::cout << std::endl;
  std::cout << "                      Chaos Equations" << std::endl;
  std::cout << std::endl;
  std::cout << "    These are plots of random recursive equations, which" << std::endl;
  std::cout << "often produce chaos, and results in beautiful patterns." << std::endl;
  std::cout << "For every time t, a point (x,y) is initialized to (t,t)." << std::endl;
  std::cout << "The equation is applied to the point many times, and each" << std::endl;
  std::cout << "iteration is drawn in a unique color." << std::endl;
  std::cout << std::endl;
  std::cout << "=========================================================" << std::endl;
  std::cout << std::endl;
  std::cout << "Controls:" << std::endl;
  std::cout << "      'A' - Automatic Mode (randomize equations)" << std::endl;
  std::cout << "      'R' - Repeat Mode (keep same equation)" << std::endl;
  std::cout << std::endl;
  std::cout << "      'C' - Center points" << std::endl;
  std::cout << "      'D' - Dot size Toggle" << std::endl;
  std::cout << "      'I' - Iteration Limit Toggle" << std::endl;
  std::cout << "      'T' - Trail Toggle" << std::endl;
  std::cout << std::endl;
  std::cout << "      'P' - Pause" << std::endl;
  std::cout << " 'LShift' - Slow Down" << std::endl;
  std::cout << " 'RShift' - Speed Up" << std::endl;
  std::cout << "  'Space' - Reverse" << std::endl;
  std::cout << std::endl;
  std::cout << "     'N' - New Equation (random)" << std::endl;
  std::cout << "     'L' - Load Equation" << std::endl;
  std::cout << "     'S' - Save Equation" << std::endl;
  std::cout << std::endl;

  //Set random seed
  rand_gen.seed((unsigned int)time(0));

  //Create the window
  GLFWwindow* window = CreateRenderWindow();

  //Simulation variables
  double t = t_start;
  std::vector<glm::vec2> history(iters);
  double rolling_delta = delta_per_step;
  double params[num_params];
  double speed_mult = 1.0;
  bool paused = false;
  int trail_type = 0;
  int dot_type = 0;
  bool load_started = false;
  bool shuffle_equ = true;
  bool iteration_limit = false;

  //Setup the vertex array
  auto vertex_count = iters*steps_per_frame;

  std::vector<VertexPos> vertex_pos(vertex_count);
  std::vector<VertexColour> vertex_colour(vertex_count);
  for (size_t i = 0; i < vertex_colour.size(); ++i) {
    vertex_colour[i] = GetRandColor(i % iters);
  }

  GLuint vertices;
  glGenVertexArrays(1, &vertices);
  glBindVertexArray(vertices);

  GLuint vertices_pos_buffer;
  glGenBuffers(1, &vertices_pos_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertices_pos_buffer);
  glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(VertexPos), nullptr, GL_STREAM_DRAW);

  GLuint vertices_colour_buffer;
  glGenBuffers(1, &vertices_colour_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertices_colour_buffer);
  glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(VertexColour), nullptr, GL_STREAM_DRAW);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertices_pos_buffer);
  glVertexAttribPointer(0, sizeof(VertexPos)/sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, nullptr);

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, vertices_colour_buffer);
  glVertexAttribPointer(1, sizeof(VertexColour)/sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, nullptr);

  auto UpdateVertexBuffers = [&]() {
    // Update pos buffer (with buffer orphaning)
    glBindBuffer(GL_ARRAY_BUFFER, vertices_pos_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(VertexPos), nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(VertexPos), reinterpret_cast<GLfloat*>(vertex_pos.data()));

    // Update colour buffer in the same way
    glBindBuffer(GL_ARRAY_BUFFER, vertices_colour_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(VertexColour), nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(VertexColour), reinterpret_cast<GLfloat*>(vertex_colour.data()));
  };

  //ImGui
  ImguiSetup(window);

  //Framebuffer
  GLuint fb;
  glGenFramebuffers(1, &fb);
  glBindFramebuffer(GL_FRAMEBUFFER, fb);

  GLuint fb_texture;
  glGenTextures(1, &fb_texture);
  glBindTexture(GL_TEXTURE_2D, fb_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_w, window_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb_texture, 0);

  GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, draw_buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

  static GLfloat fb_quad[] = {
    // positions   // texture coords
     1.0f,  1.0f,  1.0f, 1.0f,   // top right
     1.0f, -1.0f,  1.0f, 0.0f,   // bottom right
    -1.0f, -1.0f,  0.0f, 0.0f,   // bottom left
    -1.0f,  1.0f,  0.0f, 1.0f    // top left
  };
  //Probably makes more sense to use a triangle strip
  static GLuint fb_idxs[] = {
    0, 1, 3,
    1, 2, 3
  };

  GLuint trails;
  glGenVertexArrays(1, &trails);
  glBindVertexArray(trails);

  GLuint fb_vertices;
  glGenBuffers(1, &fb_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, fb_vertices);
  glBufferData(GL_ARRAY_BUFFER, sizeof(fb_quad), fb_quad, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

  GLuint fb_tris;
  glGenBuffers(1, &fb_tris);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fb_tris);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fb_idxs), fb_idxs, GL_STATIC_DRAW);

  //Initialize shaders
  {
    ShaderProgram point_shader(
      "./shaders/point_vertex.glsl",
      "./shaders/point_frag.glsl", "");
    ShaderProgram trail_shader(
      "./shaders/trail_vertex.glsl",
      "./shaders/trail_frag.glsl",
      "");

    //Initialize random parameters
    ResetPlot();
    RandParams(params);
    auto RenderEquation = GenerateNew(window, t, params);

    //Keyhandler
    keyhandler = [&](int key, int action) {
      if (action != GLFW_PRESS) return;
      if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
        return;
      } else if (key == GLFW_KEY_A) {
        shuffle_equ = true;
      } else if (key == GLFW_KEY_C) {
        CenterPlot(history);
      } else if (key == GLFW_KEY_D) {
        dot_type = (dot_type + 1) % 3;
      } else if (key == GLFW_KEY_I) {
        iteration_limit = !iteration_limit;
      } else if (key == GLFW_KEY_L) {
        shuffle_equ = false;
        load_started = true;
        paused = false;
        return;
      } else if (key == GLFW_KEY_N) {
        ResetPlot();
        RandParams(params);
        RenderEquation = GenerateNew(window, t, params);
      } else if (key == GLFW_KEY_P) {
        paused = !paused;
      } else if (key == GLFW_KEY_R) {
        shuffle_equ = false;
      } else if (key == GLFW_KEY_S) {
        std::ofstream fout("saved.txt", std::ios::app);
        fout << equ_code << std::endl;
        std::cout << "Saved: " << equ_code << std::endl;
      } else if (key == GLFW_KEY_T) {
        trail_type = (trail_type + 1) % 4;
      }
    };

    //Main Loop
    glfwSetKeyCallback(window,
      [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        keyhandler(key, action);
      });

    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      //Change simulation speed if using shift modifiers
      if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed_mult = 0.1;
      } else if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        speed_mult = 10.0;
      } else {
        speed_mult = 1.0;
      }
      if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        speed_mult = -speed_mult;
      }

      //Skip all drawing if paused
      if (paused) {
        continue;
      }

      //Automatic restart
      if (t > t_end) {
        if (shuffle_equ) {
          ResetPlot();
          RandParams(params);
        }
        RenderEquation = GenerateNew(window, t, params);
      }

      //Smooth out the stepping speed.
      const int steps = steps_per_frame;
      const double delta = delta_per_step * speed_mult;
      rolling_delta = rolling_delta*0.99 + delta*0.01;

      //Apply chaos
      for (int step = 0; step < steps; ++step) {
        bool isOffScreen = true;
        double x = t;
        double y = t;

        for (int iter = 0; iter < iters; ++iter) {
          const double xx = x * x;
          const double yy = y * y;
          const double tt = t * t;
          const double xy = x * y;
          const double xt = x * t;
          const double yt = y * t;
          const double nx = xx*params[0] + yy*params[1] + tt*params[2] + xy*params[3] + xt*params[4] + yt*params[5] + x*params[6] + y*params[7] + t*params[8];
          const double ny = xx*params[9] + yy*params[10] + tt*params[11] + xy*params[12] + xt*params[13] + yt*params[14] + x*params[15] + y*params[16] + t*params[17];
          x = nx;
          y = ny;
          VertexPos screenPt = ToScreen(x, y);
          if (iteration_limit && iter < 100) {
            screenPt.x = FLT_MAX;
            screenPt.y = FLT_MAX;
          }
          vertex_pos[step*iters + iter] = screenPt;

          //Check if dynamic delta should be adjusted
          if (screenPt.x > 0.0f && screenPt.y > 0.0f && screenPt.x < window_w && screenPt.y < window_h) {
            const float dx = history[iter].x - float(x);
            const float dy = history[iter].y - float(y);
            const double dist = double(500.0f * std::sqrt(dx*dx + dy*dy));
            rolling_delta = std::min(rolling_delta, std::max(delta / (dist + 1e-5), delta_minimum*speed_mult));
            isOffScreen = false;
          }
          history[iter].x = float(x);
          history[iter].y = float(y);
        }

        //Update the t variable
        if (isOffScreen) {
          t += 0.01;
        } else {
          t += rolling_delta;
        }
      }

      //Draw new points
      static const float dot_sizes[] = { 1.0f, 3.0f, 10.0f };
      // glEnable(GL_POINT_SMOOTH); // not supported
      glPointSize(dot_sizes[dot_type]);

      NewFrame();
      //Draw to buffer
      glBindFramebuffer(GL_FRAMEBUFFER, fb);
      glViewport(0, 0, window_w, window_h);

      //Draw previous frame (darked a little)
      trail_shader.use();
      float fade_speeds[] = { 10/255.f,2/255.f, 0.0f , 1.0f };
      trail_shader.uniformf("colour_scale", {fade_speeds[trail_type]});
      glBindVertexArray(trails);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

      //Draw current points
      glEnable(GL_BLEND);
      glEnable(GL_PROGRAM_POINT_SIZE);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      point_shader.use();
      point_shader.uniformi("window_w", {window_w});
      point_shader.uniformi("window_h", {window_h});
      UpdateVertexBuffers();
      glBindVertexArray(vertices);
      glDrawArrays(GL_POINTS, 0, vertex_pos.size());

      //Draw to screen
      glDisable(GL_BLEND);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      trail_shader.use();
      trail_shader.uniformf("colour_scale", {0.0f});
      glViewport(0, 0, window_w, window_h);
      glBindVertexArray(trails);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

      //Draw the equation
      RenderEquation();

      //Draw the current t-value
      MakeTText(t);

      //Render UI
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      //Flip the screen buffer
      glfwSwapBuffers(window);

      if (load_started) {
        std::string code;
        std::cout << "Enter 6 letter code:" << std::endl;
        std::cin >> code;
        ResetPlot();
        StringToParams(code, params);
        GenerateNew(window, t, params);
        load_started = false;
      }
    }
  }
  glfwTerminate();
  return 0;
}
