#include "openglwindow.hpp"

#include <imgui.h>

#include <cppitertools/itertools.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtc/matrix_inverse.hpp>

void OpenGLWindow::initializeGL() {
  abcg::glClearColor(0, 0, 0, 1);
  abcg::glEnable(GL_DEPTH_TEST);

  // Create program
  /*m_program = createProgramFromFile(getAssetsPath() + "depth.vert",
                                    getAssetsPath() + "depth.frag");*/

  // Create programs
  for (const auto& name : m_shaderNames) {
    const auto path{getAssetsPath() + "shaders/" + name};
    const auto program{createProgramFromFile(path + ".vert", path + ".frag")};
    m_programs.push_back(program);
  }

  // Load model
  loadModel("Mercury.obj", "Mercury.png", m_star);
  m_mappingMode = 3;  // "From mesh" option

  // Load cubemap
  m_star.loadCubeTexture(getAssetsPath() + "maps/cube/");

  // Camera at (0,0,0) and looking towards the negative z
  m_viewMatrix =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), 
                  glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));

  // Setup stars
  for (const auto index : iter::range(m_numStars)) {
    auto &position{m_starPositions.at(index)};
    auto &rotation{m_starRotations.at(index)};

    randomizeStar(position, rotation);
  }

  loadModel("ship.obj", "ancient_greece_vase.jpg", m_ship);
  //m_ship.loadCubeTexture(getAssetsPath() + "maps/cube/");

  cont_collisions = 5;
  m_shipPosition = glm::vec3(0.0f, 0.0f, 0.0f);

  initializeSkybox();
}

void OpenGLWindow::initializeSkybox() {
  // Create skybox program
  const auto path{getAssetsPath() + "shaders/" + m_skyShaderName};
  m_skyProgram = createProgramFromFile(path + ".vert", path + ".frag");

  // Generate VBO
  abcg::glGenBuffers(1, &m_skyVBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_skyVBO);
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(m_skyPositions),
                     m_skyPositions.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Get location of attributes in the program
  const GLint positionAttribute{abcg::glGetAttribLocation(m_skyProgram, "inPosition")};

  // Create VAO
  abcg::glGenVertexArrays(1, &m_skyVAO);

  // Bind vertex attributes to current VAO
  abcg::glBindVertexArray(m_skyVAO);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_skyVBO);
  abcg::glEnableVertexAttribArray(positionAttribute);
  abcg::glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // End of binding to current VAO
  abcg::glBindVertexArray(0);
}

void OpenGLWindow::loadModel(std::string path_obj, std::string path_text, Model &model) {
  model.terminateGL();

  model.loadDiffuseTexture(getAssetsPath() + "maps/" + path_text);
  model.loadNormalTexture(getAssetsPath() + "maps/pattern_normal.png");
  model.loadObj(getAssetsPath() + path_obj);
  model.setupVAO(m_programs.at(m_currentProgramIndex));

  // Use material properties from the loaded model
  model.m_Ka = model.getKa();
  model.m_Kd = model.getKd();
  model.m_Ks = model.getKs();
  model.m_shininess = model.getShininess();
}

void OpenGLWindow::randomizeStar(glm::vec3 &position, glm::vec3 &rotation) {
  // Get random position
  // x and y coordinates in the range [-20, 20]
  // z coordinates in the range [-100, 0]
  std::uniform_real_distribution<float> distPosXY(-20.0f, 20.0f);
  std::uniform_real_distribution<float> distPosZ(-100.0f, 0.0f);

  position = glm::vec3(distPosXY(m_randomEngine), distPosXY(m_randomEngine),
                       distPosZ(m_randomEngine));

  //  Get random rotation axis
  std::uniform_real_distribution<float> distRotAxis(-1.0f, 1.0f);

  rotation = glm::normalize(glm::vec3(distRotAxis(m_randomEngine),
                                      distRotAxis(m_randomEngine),
                                      distRotAxis(m_randomEngine)));
}

void OpenGLWindow::handleEvent(SDL_Event& ev) {
  const float deltaTime{static_cast<float>(getDeltaTime())};

  if (ev.type == SDL_KEYDOWN) {
    if (ev.key.keysym.sym == SDLK_UP || ev.key.keysym.sym == SDLK_w){
      m_shipPosition.y += deltaTime * 1.5f;
    }
    if (ev.key.keysym.sym == SDLK_DOWN || ev.key.keysym.sym == SDLK_s){
      m_shipPosition.y -= deltaTime * 1.5f;
    }
    if (ev.key.keysym.sym == SDLK_LEFT || ev.key.keysym.sym == SDLK_a){
      m_shipPosition.x -= deltaTime * 1.5f;
    }
    if (ev.key.keysym.sym == SDLK_RIGHT || ev.key.keysym.sym == SDLK_d){
      m_shipPosition.x += deltaTime * 1.5f;
    }
  }

}

void OpenGLWindow::paintGL() {
  update();

  // Clear color buffer and depth buffer
  abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);

  // Use currently selected program
  const auto program{m_programs.at(m_currentProgramIndex)};
  abcg::glUseProgram(program);

  // Get location of uniform variables (could be precomputed)
  const GLint viewMatrixLoc{abcg::glGetUniformLocation(program, "viewMatrix")};
  const GLint projMatrixLoc{abcg::glGetUniformLocation(program, "projMatrix")};
  const GLint modelMatrixLoc{abcg::glGetUniformLocation(program, "modelMatrix")};
  const GLint colorLoc{abcg::glGetUniformLocation(program, "color")};

  const GLint normalMatrixLoc{abcg::glGetUniformLocation(program, "normalMatrix")};
  const GLint lightDirLoc{abcg::glGetUniformLocation(program, "lightDirWorldSpace")};
  const GLint shininessLoc{abcg::glGetUniformLocation(program, "shininess")};
  const GLint IaLoc{abcg::glGetUniformLocation(program, "Ia")};
  const GLint IdLoc{abcg::glGetUniformLocation(program, "Id")};
  const GLint IsLoc{abcg::glGetUniformLocation(program, "Is")};
  const GLint KaLoc{abcg::glGetUniformLocation(program, "Ka")};
  const GLint KdLoc{abcg::glGetUniformLocation(program, "Kd")};
  const GLint KsLoc{abcg::glGetUniformLocation(program, "Ks")};
  const GLint diffuseTexLoc{abcg::glGetUniformLocation(program, "diffuseTex")};
  const GLint mappingModeLoc{abcg::glGetUniformLocation(program, "mappingMode")};
  const GLint normalTexLoc{abcg::glGetUniformLocation(program, "normalTex")};
  const GLint cubeTexLoc{abcg::glGetUniformLocation(program, "cubeTex")};
  const GLint texMatrixLoc{abcg::glGetUniformLocation(program, "texMatrix")};

  // Set uniform variables used by every scene object
  abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);
  abcg::glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);  // White

  abcg::glUniform1i(diffuseTexLoc, 0);
  abcg::glUniform1i(normalTexLoc, 1);
  abcg::glUniform1i(cubeTexLoc, 2);
  abcg::glUniform1i(mappingModeLoc, m_mappingMode);

  const glm::mat3 texMatrix{glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3{1.0f})};
  abcg::glUniformMatrix3fv(texMatrixLoc, 1, GL_TRUE, &texMatrix[0][0]);

  //const auto lightDirRotated{m_lightDir};
  abcg::glUniform4fv(lightDirLoc, 1, &m_star.m_lightDir.x);
  abcg::glUniform4fv(IaLoc, 1, &m_star.m_Ia.x);
  abcg::glUniform4fv(IdLoc, 1, &m_star.m_Id.x);
  abcg::glUniform4fv(IsLoc, 1, &m_star.m_Is.x);

  // Render each star
  for (const auto index : iter::range(m_numStars)) {
    const auto &position{m_starPositions.at(index)};
    const auto &rotation{m_starRotations.at(index)};

    // Compute model matrix of the current star
    glm::mat4 modelMatrix{1.0f};
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.2f));
    modelMatrix = glm::rotate(modelMatrix, m_angle, rotation);

    // Set uniform variable
    abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &modelMatrix[0][0]);

    const auto modelViewMatrix{glm::mat3(m_viewMatrix * modelMatrix)};
    glm::mat3 normalMatrix{glm::inverseTranspose(modelViewMatrix)};
    abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

    abcg::glUniform1f(shininessLoc, m_star.m_shininess);
    abcg::glUniform4fv(KaLoc, 1, &m_star.m_Ka.x);
    abcg::glUniform4fv(KdLoc, 1, &m_star.m_Kd.x);
    abcg::glUniform4fv(KsLoc, 1, &m_star.m_Ks.x);

    m_star.render();
  }

  // Render ship
  glm::mat4 modelMatrixShip{1.0f};
  modelMatrixShip = glm::translate(modelMatrixShip, m_shipPosition);
  modelMatrixShip = glm::scale(modelMatrixShip, glm::vec3(0.07f));

  abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &modelMatrixShip[0][0]);

  const auto modelViewMatrix{glm::mat3(m_viewMatrix * modelMatrixShip)};
  glm::mat3 normalMatrix{glm::inverseTranspose(modelViewMatrix)};
  abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  abcg::glUniform1f(shininessLoc, m_ship.m_shininess);
  abcg::glUniform4fv(KaLoc, 1, &m_ship.m_Ka.x);
  abcg::glUniform4fv(KdLoc, 1, &m_ship.m_Kd.x);
  abcg::glUniform4fv(KsLoc, 1, &m_ship.m_Ks.x);

  m_ship.render();

  abcg::glUseProgram(0);

  if (m_currentProgramIndex == 0 || m_currentProgramIndex == 1) {
    renderSkybox();
  }
}

void OpenGLWindow::renderSkybox() {
  
  abcg::glUseProgram(m_skyProgram);


  // Get location of uniform variables
  const GLint viewMatrixLoc{abcg::glGetUniformLocation(m_skyProgram, "viewMatrix")};
  const GLint projMatrixLoc{abcg::glGetUniformLocation(m_skyProgram, "projMatrix")};
  const GLint skyTexLoc{abcg::glGetUniformLocation(m_skyProgram, "skyTex")};

  // Set uniform variables
  const auto viewMatrix{glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3{1.0f})};
  abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &viewMatrix[0][0]);

  
  abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);
  abcg::glUniform1i(skyTexLoc, 0);

  abcg::glBindVertexArray(m_skyVAO);

  abcg::glActiveTexture(GL_TEXTURE0);
  abcg::glBindTexture(GL_TEXTURE_CUBE_MAP, m_star.getCubeTexture());

  abcg::glEnable(GL_CULL_FACE);
  abcg::glFrontFace(GL_CW);
  abcg::glDepthFunc(GL_LEQUAL);
  abcg::glDrawArrays(GL_TRIANGLES, 0, m_skyPositions.size());
  abcg::glDepthFunc(GL_LESS);

  abcg::glBindVertexArray(0);
  
  abcg::glUseProgram(0);
  
}

void OpenGLWindow::terminateSkybox() {
  abcg::glDeleteProgram(m_skyProgram);
  abcg::glDeleteBuffers(1, &m_skyVBO);
  abcg::glDeleteVertexArrays(1, &m_skyVAO);
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  {
    const auto widgetSize{ImVec2(222, 190)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5, 5));
    ImGui::SetNextWindowSize(widgetSize);
    const auto flags{ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration};
    ImGui::Begin("Widget window", nullptr, flags);

    // Projection combo box
    {
      ImGui::PushItemWidth(120);
      static std::size_t currentIndex{};
      const std::vector<std::string> comboItems{"Perspective", "Orthographic"};

      if (ImGui::BeginCombo("Projection",
                            comboItems.at(currentIndex).c_str())) {
        for (const auto index : iter::range(comboItems.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::PushItemWidth(170);
      const auto aspect{static_cast<float>(m_viewportWidth) /
                        static_cast<float>(m_viewportHeight)};
      if (currentIndex == 0) {
        m_projMatrix =
            glm::perspective(glm::radians(m_FOV), aspect, 0.01f, 100.0f);

        ImGui::SliderFloat("FOV", &m_FOV, 5.0f, 179.0f, "%.0f degrees");
      } else {
        m_projMatrix = glm::ortho(-20.0f * aspect, 20.0f * aspect, -20.0f,
                                  20.0f, 0.01f, 100.0f);
      }
      ImGui::PopItemWidth();
    }

    // Shader combo box
    {
      static std::size_t currentIndex{};

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("Shader", m_shaderNames.at(currentIndex))) {
        for (auto index : iter::range(m_shaderNames.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(m_shaderNames.at(index), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      // Set up VAO if shader program has changed
      if (static_cast<int>(currentIndex) != m_currentProgramIndex) {
        m_currentProgramIndex = currentIndex;
        m_star.setupVAO(m_programs.at(m_currentProgramIndex));
      }
    }

    if (!m_star.isUVMapped()) {
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Mesh has no UV coords.");
    }

    // UV mapping box
    {
      std::vector<std::string> comboItems{"Triplanar", "Cylindrical",
                                          "Spherical"};

      if (m_star.isUVMapped()) comboItems.emplace_back("From mesh");

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("UV mapping",
                            comboItems.at(m_mappingMode).c_str())) {
        for (auto index : iter::range(comboItems.size())) {
          const bool isSelected{m_mappingMode == static_cast<int>(index)};
          if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
            m_mappingMode = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::Text("Colisões restantes: %d", cont_collisions);
      ImGui::PopItemWidth();
    }

    ImGui::End();
  }

  {
    const auto size{ImVec2(150, 150)};
    const auto position{ImVec2((m_viewportWidth - size.x) / 2.0f,
                               (m_viewportHeight - size.y) / 2.0f)};
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGuiWindowFlags flags{ImGuiWindowFlags_NoBackground |
                           ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoInputs};
    ImGui::Begin(" ", nullptr, flags);
    //ImGui::PushFont(m_font);
    if(isLose)
      {
        ImGui::Text(" *Lose!* ");
      }
    //ImGui::PopFont();
    ImGui::End();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;
}

void OpenGLWindow::terminateGL() {
  m_star.terminateGL();
  m_ship.terminateGL();
  for (const auto& program : m_programs) {
    abcg::glDeleteProgram(program);
  }
}

void OpenGLWindow::update() {

  if(isLose && m_restartWaitTimer.elapsed() > 5){
    restart();
    return;
  }

  // Animate angle by 90 degrees per second
  const float deltaTime{static_cast<float>(getDeltaTime())};
  m_angle = glm::wrapAngle(m_angle + glm::radians(90.0f) * deltaTime);

  // Update stars
  for (const auto index : iter::range(m_numStars)) {
    auto &position{m_starPositions.at(index)};
    auto &rotation{m_starRotations.at(index)};

    // Z coordinate increases by 10 units per second
    position.z += deltaTime * 13.0f;

    if(!isLose){
      // If this star is behind the camera, select a new random position and
      // orientation, and move it back to -100
      if (position.z > 0.1f) {
        randomizeStar(position, rotation);
        position.z = -100.0f;  // Back to -100
      }

      // Check Colisions
      if (  (m_shipPosition.x <= position.x + 1.2f && m_shipPosition.x >= position.x - 1.2f)
            && (m_shipPosition.y <= position.y + 1.2f && m_shipPosition.y >= position.y - 1.2f) 
            && (m_shipPosition.z <= position.z + 1.2f && m_shipPosition.z >= position.z - 1.2f)) 
      {
        if(m_collisionTimer.elapsed() > 1){
          cont_collisions = cont_collisions - 1;

          if(cont_collisions == 0){
            isLose = true;
            m_shipPosition.z = 20.0f;
            m_restartWaitTimer.restart();
          }

          m_collisionTimer.restart();
        }
      }
    }else{
      position.z = 20.0f;
    }
  }    
  
}

void OpenGLWindow::restart() {
    isLose = false;
    // terminateGL();
    initializeGL();
}