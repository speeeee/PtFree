#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// NOTE: if a set of points is to represent some kind of opaque object (i.e., depth-testing should
//     : refer to the entire object), then a polygon approximating the object can be drawn with
//     : the same color as clear color and place slightly behind the set of points.

static const GLchar *dvs = "#version 330\n"
  "layout (location = 0) in vec3 position;\n"
  "layout (location = 1) in vec3 color;\n"
  "layout (location = 2) in vec3 norm;\n"
  "out vec3 frag_norm;\n"
  //"uniform vec3 disp;\n"
  "uniform mat4 model; uniform mat4 view; uniform mat4 projection;\n"
  "uniform vec3 pos; uniform float tht;\n"
  "vec3 rot(vec3 a,float tht) { return a*mat3("
  "  cos(tht),-sin(tht),0, sin(tht),cos(tht),0, 0,0,1); }\n"
  "void main() { frag_norm = norm; vec3 np = rot(vec3(gl_InstanceID/500.,0,0),gl_InstanceID/3.14159265)+position;\n"
  "gl_Position = projection*view*model*vec4(rot(np,tht)+pos,1.); }\0";
static const GLchar *dfs = "#version 330\n"
  "in vec3 frag_norm;\n"
  "void main() { vec3 light = -vec3(0.,-1.,0.); vec3 col = vec3(0.1,0.5,0.7);\n"
  "  gl_FragColor = vec4(1.,0,0,1.); }\0";

GLuint create_program(const GLchar *vsh, const GLchar *fsh) { GLuint vs;
  vs = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vs,1,&vsh,NULL); glCompileShader(vs);
  
  GLint compiled = 0;
  glGetShaderiv(vs,GL_COMPILE_STATUS,&compiled);
  if(!compiled) { printf("Vertex error:\n"); GLint mlen = 0;
    glGetShaderiv(vs,GL_INFO_LOG_LENGTH, &mlen);
    std::vector<GLchar> error_log(mlen);
    glGetShaderInfoLog(vs,mlen,&mlen,&error_log[0]);
    glDeleteShader(vs); printf("%s\n",&error_log[0]); }
  GLuint fs; fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs,1,&fsh,NULL); glCompileShader(fs);
  compiled = 0;
  glGetShaderiv(fs,GL_COMPILE_STATUS,&compiled);
  if(!compiled) { printf("Fragment error:\n"); GLint mlen = 0;
    glGetShaderiv(fs,GL_INFO_LOG_LENGTH, &mlen);
    std::vector<GLchar> error_log(mlen);
    glGetShaderInfoLog(fs,mlen,&mlen,&error_log[0]);
    glDeleteShader(fs); printf("%s\n",&error_log[0]); }

  GLuint prog; prog = glCreateProgram();
  glAttachShader(prog,vs); glAttachShader(prog,fs);
  /*glBindAttribLocation(prog,0,"position");
  glBindAttribLocation(prog,1,"norm");*/ glLinkProgram(prog);
  glDeleteShader(vs); glDeleteShader(fs); return prog; }

void mvp_set(GLuint prog, glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
  glUseProgram(prog);

  GLint _model = glGetUniformLocation(prog,"model");
  glUniformMatrix4fv(_model,1,GL_FALSE,(const float *)glm::value_ptr(model));
  GLint _view = glGetUniformLocation(prog,"view");
  glUniformMatrix4fv(_view,1,GL_FALSE,(const float *)glm::value_ptr(view));
  GLint _projection = glGetUniformLocation(prog,"projection");
  glUniformMatrix4fv(_projection,1,GL_FALSE,(const float *)glm::value_ptr(projection)); }

// TODO: generalize.
void vec_set(GLuint prog, glm::vec3 a, std::string name) {
  glUseProgram(prog);

  GLint _name = glGetUniformLocation(prog,name.c_str());
  glUniform3f(_name,a.x,a.y,a.z); }
void float_set(GLuint prog, float a, std::string name) {
  glUseProgram(prog);

  GLint _name = glGetUniformLocation(prog,name.c_str());
  glUniform1f(_name,a); }
