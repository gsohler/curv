// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is:
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#ifndef LIBCURV_GEOM_VIEWER_VIEWER_H
#define LIBCURV_GEOM_VIEWER_VIEWER_H

#include <libcurv/geom/frag.h>
#include <gl/shader.h>
#include <gl/vbo.h>
#include <glm/glm.hpp>

namespace curv { namespace geom {

struct Shape_Program;

namespace viewer {

struct Viewer_Config : public Frag_Export
{
    bool verbose_ = false;
};

struct Viewer
{
    /*--- PUBLIC API ---*/
    Viewer();
    Viewer(const Viewer_Config&);

    // Set the current shape. May be called at any time, before opening the
    // window, or while the window is open.
    void set_shape(const Shape_Program&);
    void set_shape(const Shape_Program&, const Frag_Export&);

    bool is_open() { return window_ != nullptr; }

    // Open window: initialize OpenGL, create the window. Idempotent.
    void open();

    // Called repeatedly in a loop once the window is open.
    // Returns false when it is time to exit the loop (user has closed window).
    bool draw_frame();

    // Close window. Idempotent.
    void close();

    ~Viewer();

    // Open a Viewer window on the current shape, and run until the window
    // is closed by the user. It is equivalent to:
    //     open();
    //     while (draw_frame());
    //     close();
    void run();

    /*--- PARAMETER STATE, can set before window is opened ---*/
    Viewer_Config config_;
    glm::ivec4 window_pos_and_size_{0.,0.,500.,500.};
    bool headless_{false};

    /*--- DEPRECATED API ---*/

    // Can only be called when open, to change the frag shader.
    // Use set_shape() instead.
    void set_frag(const std::string&);

    /*--- INTERNAL STATE ---*/

    std::string fragsrc_{};
    Shader shader_{};
    std::string vertSource_{};
    GLFWwindow* window_ = nullptr;
    glm::ivec4 viewport_;
    float fPixelDensity_ = 1.0;
    Vbo* vbo_ = nullptr;
    double current_time_ = 0.0;
    double fFPS_ = 0.0;

    glm::mat3 u_view2d_ = glm::mat3(1.);
    // These are the 'view3d' uniforms.
    // Note: the up3d vector must be orthogonal to (eye3d - centre3d),
    // or rotation doesn't work correctly.
    glm::vec3 u_centre3d_ = glm::vec3(0.,0.,0.);
    // The following initial value for 'eye3d' is derived by starting with
    // [0,0,6], then rotating 30 degrees around the X and Y axes.
    glm::vec3 u_eye3d_ = glm::vec3(2.598076,3.0,4.5);
    // The initial value for up3d is derived by starting with [0,1,0], then
    // applying the same rotations as above, so that up3d is orthogonal to eye3d.
    glm::vec3 u_up3d_ = glm::vec3(-0.25,0.866025,-0.433013);

    typedef struct {
        float     x,y;
        int       button;
        glm::vec2 velocity;
        glm::vec2 drag;
    } Mouse;
    Mouse mouse_;

    // INTERNAL FUNCTIONS
    void initGL(glm::ivec4 &_viewport, bool _headless = false);
    void setup();
    void onKeyPress(int);
    void onMouseMove(double, double);
    void onScroll(float);
    void onMouseDrag(float, float, int);
    void render();
    void swap_buffers();
    void poll_events();
    void measure_time();
    void debounceSetWindowTitle(std::string);
    void closeGL();
    float getPixelDensity();
    void onExit();
    void setWindowSize(int, int);
    int getWindowWidth();
    int getWindowHeight();
};

}}} // namespace
#endif // header guard
