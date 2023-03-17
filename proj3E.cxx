#include <fstream>
#include <iostream>

#include <GL/glew.h>    // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>   // glm::vec3
#include <glm/vec4.hpp>   // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale



#include "shaders.h"
#include "window.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"
using nlohmann::json;


#define BUFFER_OFFSET(i) ((char *)NULL + (i))

bool loadModel(tinygltf::Model &model, const char *filename) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "ERR: " << err << std::endl;
  }

  if (!res)
    std::cout << "Failed to load glTF: " << filename << std::endl;
  else
    std::cout << "Loaded glTF: " << filename << std::endl;

  return res;
}

void bindMesh(std::map<int, GLuint>& vbos,
              tinygltf::Model &model, tinygltf::Mesh &mesh) {
  for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    const tinygltf::BufferView &bufferView = model.bufferViews[i];
    if (bufferView.target == 0) {  // TODO impl drawarrays
      std::cout << "WARN: bufferView.target is zero" << std::endl;
      continue;  // Unsupported bufferView.
                 /*
                   From spec2.0 readme:
                   https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
                            ... drawArrays function should be used with a count equal to
                   the count            property of any of the accessors referenced by the
                   attributes            property            (they are all equal for a given
                   primitive).
                 */
    }

    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
    std::cout << "bufferview.target " << bufferView.target << std::endl;

    GLuint vbo;
    glGenBuffers(1, &vbo);
    vbos[i] = vbo;
    glBindBuffer(bufferView.target, vbo);

    std::cout << "buffer.data.size = " << buffer.data.size()
              << ", bufferview.byteOffset = " << bufferView.byteOffset
              << std::endl;

    glBufferData(bufferView.target, bufferView.byteLength,
                 &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
  }

  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    for (auto &attrib : primitive.attributes) {
      tinygltf::Accessor accessor = model.accessors[attrib.second];
      int byteStride =
          accessor.ByteStride(model.bufferViews[accessor.bufferView]);
      glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

      int size = 1;
      if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        size = accessor.type;
      }

      int vaa = -1;
      if (attrib.first.compare("POSITION") == 0) vaa = 0;
      if (attrib.first.compare("NORMAL") == 0) vaa = 1;
      if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
      if (attrib.first.compare("TANGENT") == 0) vaa = 3;
      
      if (vaa > -1) {
        glEnableVertexAttribArray(vaa);
        glVertexAttribPointer(vaa, size, accessor.componentType,
                              accessor.normalized ? GL_TRUE : GL_FALSE,
                              byteStride, BUFFER_OFFSET(accessor.byteOffset));
      } else
        std::cout << "vaa missing: " << attrib.first << std::endl;
    }



  }
}

// bind models
void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model &model,
                    tinygltf::Node &node) {
  if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
    bindMesh(vbos, model, model.meshes[node.mesh]);
  }

  for (size_t i = 0; i < node.children.size(); i++) {
    assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
    bindModelNodes(vbos, model, model.nodes[node.children[i]]);
  }
}

std::pair<GLuint, std::map<int, GLuint>> bindModel(tinygltf::Model &model) {
  std::map<int, GLuint> vbos;
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  const tinygltf::Scene &scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
    bindModelNodes(vbos, model, model.nodes[scene.nodes[i]]);
  }

  glBindVertexArray(0);
  // cleanup vbos but do not delete index buffers yet
  for (auto it = vbos.cbegin(); it != vbos.cend();) {
    tinygltf::BufferView bufferView = model.bufferViews[it->first];
    if (bufferView.target != GL_ELEMENT_ARRAY_BUFFER) {
      glDeleteBuffers(1, &vbos[it->first]);
      vbos.erase(it++);
    }
    else {
      ++it;
    }
  }

  return {vao, vbos};
}

void drawMesh(const std::map<int, GLuint>& vbos,
              tinygltf::Model &model, tinygltf::Mesh &mesh) {
  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

    glDrawElements(primitive.mode, indexAccessor.count,
                   indexAccessor.componentType,
                   BUFFER_OFFSET(indexAccessor.byteOffset));
  }
}

// recursively draw node and children nodes of model
void drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos,
                    tinygltf::Model &model, tinygltf::Node &node) {
  if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
    drawMesh(vaoAndEbos.second, model, model.meshes[node.mesh]);
  }
  for (size_t i = 0; i < node.children.size(); i++) {
    drawModelNodes(vaoAndEbos, model, model.nodes[node.children[i]]);
  }
}
void drawModel(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos,
               tinygltf::Model &model) {
  glBindVertexArray(vaoAndEbos.first);

  const tinygltf::Scene &scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    drawModelNodes(vaoAndEbos, model, model.nodes[scene.nodes[i]]);
  }

  glBindVertexArray(0);
}

void dbgModel(tinygltf::Model &model) {
  for (auto &mesh : model.meshes) {
    std::cout << "mesh : " << mesh.name << std::endl;
    for (auto &primitive : mesh.primitives) {
      const tinygltf::Accessor &indexAccessor =
          model.accessors[primitive.indices];

      std::cout << "indexaccessor: count " << indexAccessor.count << ", type "
                << indexAccessor.componentType << std::endl;

      tinygltf::Material &mat = model.materials[primitive.material];
      for (auto &mats : mat.values) {
        std::cout << "mat : " << mats.first.c_str() << std::endl;
      }

      for (auto &image : model.images) {
        std::cout << "image name : " << image.uri << std::endl;
        std::cout << "  size : " << image.image.size() << std::endl;
        std::cout << "  w/h : " << image.width << "/" << image.height
                  << std::endl;
      }

      std::cout << "indices : " << primitive.indices << std::endl;
      std::cout << "mode     : "
                << "(" << primitive.mode << ")" << std::endl;

      for (auto &attrib : primitive.attributes) {
        std::cout << "attribute : " << attrib.first.c_str() << std::endl;
      }
    }
  }
}

glm::mat4 genView(glm::vec3 pos, glm::vec3 lookat) {
  // Camera matrix
  glm::mat4 view = glm::lookAt(
      pos,                // Camera in World Space
      lookat,             // and looks at the origin
      glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
  );

  return view;
}

glm::mat4 genMVP(glm::mat4 view_mat, glm::mat4 model_mat, float fov, int w,
                 int h) {
  glm::mat4 Projection =
      glm::perspective(glm::radians(fov), (float)w / (float)h, 0.01f, 1000.0f);

  // Or, for an ortho camera :
  // glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f);
  // // In world coordinates

  glm::mat4 mvp = Projection * view_mat * model_mat;

  return mvp;
}


// NOTE: Global variables! Ugly, but helpful in a quick fix. You may want to change things around. 
// Remember, modifying them anywhere changes their state across all of the program.
// We use this property to change the camera.


float camera_yaw = 0.0f;
float camera_pitch = 0.0f;
float camera_sensitivity = 0.1f;
float distance = 1;
// Change this in config if you want to use displacement maps
int displacement_tex = 0;

glm::vec3 rc;
glm::vec3 camera;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = xpos, lastY = ypos;
    static bool first = true;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (first) {
            first = false;
        } else {
            double xoffset = xpos - lastX;
            double yoffset = lastY - ypos;
            xoffset *= camera_sensitivity;
            yoffset *= camera_sensitivity;
            camera_yaw += xoffset;
            camera_pitch += yoffset;

            if (camera_pitch > 89.0f)
                camera_pitch = 89.0f;
            if (camera_pitch < -89.0f)
                camera_pitch = -89.0f;

            camera = glm::vec3(rc.x+distance*cos(glm::radians(camera_pitch))*sin(glm::radians(camera_yaw)),
                               rc.y+distance*sin(glm::radians(camera_pitch)),
                               rc.z+distance*cos(glm::radians(camera_pitch))*cos(glm::radians(camera_yaw)));
        }
    } else {
        first = true;
    }

    lastX = xpos;
    lastY = ypos;
}


void displayLoop(Window &window, const std::string &filename) {
  Shaders shader = Shaders();
  glUseProgram(shader.pid);

  // grab uniforms to modify
  GLuint MVP_u = glGetUniformLocation(shader.pid, "MVP");
  GLuint lightpos_u = glGetUniformLocation(shader.pid, "lightpos");
  GLuint camerapos_u = glGetUniformLocation(shader.pid, "camerapos");
  
  // change light direction, camera distance, rotation center without compiling
  // We are using https://github.com/nlohmann/json
  // NOTE: you can add more in the config file, as long as you make the changes in the function here
  // A good addition would be to take light intensity as a parameter. 
  // For that, you would also need to add a new uniform.
  
  json config;
  std::ifstream configstream("config.json");
  configstream >> config;
  rc = glm::make_vec3(config["rc"].get<std::vector<float>>().data());
  distance = config["camera_distance"].get<float>();
  displacement_tex = config["displacement"].get<int>();
  camera = glm::vec3(rc.x+distance*cos(glm::radians(camera_pitch))*sin(glm::radians(camera_yaw)),
                               rc.y+distance*sin(glm::radians(camera_pitch)),
                               rc.z+distance*cos(glm::radians(camera_pitch))*cos(glm::radians(camera_yaw)));

  tinygltf::Model model;
  if (!loadModel(model, filename.c_str())) return;

  std::pair<GLuint, std::map<int, GLuint>> vaoAndEbos = bindModel(model);
  // dbgModel(model); return;

  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 model_mat = glm::mat4(1.0f);
  glm::mat4 model_rot = glm::mat4(1.0f);
  glm::vec3 model_pos = glm::vec3(0, 0, 0);
  glm::vec3 light_pos = glm::make_vec3(config["light_pos"].get<std::vector<float>>().data());
  glm::vec3 clearcolor = glm::make_vec3(config["clearcolor"].get<std::vector<float>>().data());

  GLuint texid[32];


  // generate a camera view, based on eye-position and lookAt world-position
  glfwSetCursorPosCallback(window.window, cursor_position_callback);


  // NOTE: we are loading the textures here. 
  if (model.textures.size() > 0) {
      // fixme: Use material's baseColor

      for(size_t tex_index =0;tex_index < model.textures.size();tex_index++)
      {
        glGenTextures(1, texid+tex_index);

        tinygltf::Texture &tex = model.textures[tex_index];

        if (tex.source > -1) {


          tinygltf::Image &image = model.images[tex.source];

          glBindTexture(GL_TEXTURE_2D, texid[tex_index]);
          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

          GLenum format = GL_RGBA;

          if (image.component == 1) {
            format = GL_RED;
          } else if (image.component == 2) {
            format = GL_RG;
          } else if (image.component == 3) {
            format = GL_RGB;
          } else {
            // ???
          }

          GLenum type = GL_UNSIGNED_BYTE;
          if (image.bits == 8) {
            // ok
          } else if (image.bits == 16) {
            type = GL_UNSIGNED_SHORT;
          } else {
            // ???
          }

          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                      format, type, &image.image.at(0));
        }
    }
  }


  // Setting texture uniforms
  glUniform1i( glGetUniformLocation( shader.pid, "nor_tex" ), 0 );
  glUniform1i( glGetUniformLocation( shader.pid, "diff_tex" ), 1 );
  glUniform1i( glGetUniformLocation( shader.pid, "arm_tex" ), 2 );
  if(displacement_tex == 1)
    glUniform1i( glGetUniformLocation( shader.pid, "disp_tex" ), 3 );
  

  while (!window.Close()) {
    window.Resize();
    glClearColor(clearcolor.x,clearcolor.y,clearcolor.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view_mat = genView(camera, model_pos);

    // build a model-view-projection
    GLint w, h;
    glfwGetWindowSize(window.window, &w, &h);
    
    // mess around with the MVP matrix to get your own projection
    glm::mat4 mvp = genMVP(view_mat, model_mat, 45.0f, w, h);
    
    //setting uniforms
    glUniformMatrix4fv(MVP_u, 1, GL_FALSE, &mvp[0][0]);
    glUniform3fv(lightpos_u, 1, &light_pos[0]);
    glUniform3fv(camerapos_u, 1, &camera[0]);
    
    //activating textures
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texid[0] );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, texid[1] );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, texid[2] );
    if(displacement_tex == 1)
    {
        glActiveTexture( GL_TEXTURE3 );
        glBindTexture( GL_TEXTURE_2D, texid[3] );
    }  
    drawModel(vaoAndEbos, model);
    glfwSwapBuffers(window.window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vaoAndEbos.first);
}

static void error_callback(int error, const char *description) {
  (void)error;
  fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char **argv) {
  std::string filename = "../../../models/Cube/Cube.gltf";

  if (argc > 1) {
    filename = argv[1];
  }

  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) return -1;

  // Force create OpenGL 3.3
  // NOTE(syoyo): Linux + NVIDIA driver segfaults for some reason? commenting out glfwWindowHint will work.
  // Note (PE): On laptops with intel hd graphics card you can overcome the segfault by enabling experimental, see below (tested on lenovo thinkpad)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glewExperimental = GL_TRUE;

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  Window window = Window(800, 600, "TinyGLTF basic example");
  glfwMakeContextCurrent(window.window);

#ifdef __APPLE__
  // https://stackoverflow.com/questions/50192625/openggl-segmentation-fault
  glewExperimental = GL_TRUE;
#endif

  glewInit();
  std::cout << glGetString(GL_RENDERER) << ", " << glGetString(GL_VERSION)
            << std::endl;

  if (!GLEW_VERSION_3_3) {
    std::cerr << "OpenGL 3.3 is required to execute this app." << std::endl;
    return EXIT_FAILURE;
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  displayLoop(window, filename);

  glfwTerminate();
  return 0;
}

