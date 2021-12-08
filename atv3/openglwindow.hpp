#ifndef OPENGLWINDOW_HPP_
#define OPENGLWINDOW_HPP_

#include <random>

#include "abcg.hpp"
#include "model.hpp"

class OpenGLWindow : public abcg::OpenGLWindow {
 protected:
  void handleEvent(SDL_Event& ev) override;
  void initializeGL() override;
  void paintGL() override;
  void paintUI() override;
  void resizeGL(int width, int height) override;
  void terminateGL() override;

 private:
  static const int m_numStars{1200};

  //GLuint m_program{};

  int m_viewportWidth{};
  int m_viewportHeight{};

  std::default_random_engine m_randomEngine;

  Model m_star;
  Model m_ship;

  std::array<glm::vec3, m_numStars> m_starPositions;
  std::array<glm::vec3, m_numStars> m_starRotations;
  float m_angle{};

  glm::mat4 m_viewMatrix{1.0f};
  glm::mat4 m_projMatrix{1.0f};
  float m_FOV{125.0f};

  int cont_collisions{2};
  bool isLose{false};
  abcg::ElapsedTimer m_collisionTimer;
  abcg::ElapsedTimer m_restartWaitTimer;

  glm::vec3 m_shipPosition{glm::vec3(0.0f, 0.0f, 0.0f)};
  
  //NOVOS
  //glm::mat4 m_modelMatrix{1.0f};

  // Shaders
  std::vector<const char*> m_shaderNames{"cubereflect", "cuberefract", "normalmapping", "texture", "blinnphong",
      "phong",       "gouraud",     "normal",        "depth"};
  std::vector<GLuint> m_programs;
  int m_currentProgramIndex{};

  // Mapping mode
  int m_mappingMode{};

  // Skybox
  const std::string m_skyShaderName{"skybox"};
  GLuint m_skyVAO{};
  GLuint m_skyVBO{};
  GLuint m_skyProgram{};

  // clang-format off
  const std::array<glm::vec3, 36>  m_skyPositions{
    // Front
    glm::vec3{-1, -1, +1}, glm::vec3{+1, -1, +1}, glm::vec3{+1, +1, +1},
    glm::vec3{-1, -1, +1}, glm::vec3{+1, +1, +1}, glm::vec3{-1, +1, +1},
    // Back
    glm::vec3{+1, -1, -1}, glm::vec3{-1, -1, -1}, glm::vec3{-1, +1, -1},
    glm::vec3{+1, -1, -1}, glm::vec3{-1, +1, -1}, glm::vec3{+1, +1, -1},
    // Right
    glm::vec3{+1, -1, -1}, glm::vec3{+1, +1, -1}, glm::vec3{+1, +1, +1},
    glm::vec3{+1, -1, -1}, glm::vec3{+1, +1, +1}, glm::vec3{+1, -1, +1},
    // Left
    glm::vec3{-1, -1, +1}, glm::vec3{-1, +1, +1}, glm::vec3{-1, +1, -1},
    glm::vec3{-1, -1, +1}, glm::vec3{-1, +1, -1}, glm::vec3{-1, -1, -1},
    // Top
    glm::vec3{-1, +1, +1}, glm::vec3{+1, +1, +1}, glm::vec3{+1, +1, -1},
    glm::vec3{-1, +1, +1}, glm::vec3{+1, +1, -1}, glm::vec3{-1, +1, -1},
    // Bottom
    glm::vec3{-1, -1, -1}, glm::vec3{+1, -1, -1}, glm::vec3{+1, -1, +1},
    glm::vec3{-1, -1, -1}, glm::vec3{+1, -1, +1}, glm::vec3{-1, -1, +1}
  };
  // clang-format on

  void initializeSkybox();
  void renderSkybox();
  void terminateSkybox();
  void loadModel(std::string path_obj, std::string path_text, Model &model);
  void randomizeStar(glm::vec3 &position, glm::vec3 &rotation);
  void update();
  void restart();
};

#endif