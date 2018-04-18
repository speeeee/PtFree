#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// NOTE: if a set of points is to represent some kind of opaque object (i.e., depth-testing should
//     : refer to the entire object), then a polygon approximating the object can be drawn with
//     : the same color as clear color and place slightly behind the set of points.

// TODO: test PRNG.
// TODO: create compute shader.
// TODO: change vertex shader to use V4f struct instead.

static const GLchar *dvs = "#version 430\n"
  "layout (location = 0) in vec3 position;\n"
  //"layout (location = 1) in vec3 color;\n"
  //"layout (location = 2) in vec3 norm;\n"
  "out vec3 frag_norm; out vec3 frag_col;\n"
  //"uniform vec3 disp;\n"
  "uniform mat4 model; uniform mat4 view; uniform mat4 projection; uniform mat4 mvp;\n"
  "uniform vec3 pos; uniform float tht; uniform int seed; uniform int vert_amt;\n"
  // seed will be the current time + vertex_amt*instance_id + vertex_id.
  "vec3 rot(vec3 a,float tht) { return a*mat3("
  "  cos(tht),-sin(tht),0, sin(tht),cos(tht),0, 0,0,1); }\n"
  // PRNG using Galoi LFSR [1,2^32-1].
  "int lcg_rand(int x) { return (x >> 1)^(-(x & 1) & 0x80200003); }\n"
  "void main() { vec3 np = rot(vec3(gl_InstanceID/500.,0,0),gl_InstanceID/3.14159265)+position;\n"
  //"gl_Position = projection*view*model*vec4(rot(np,tht)+pos,1.); }\0";
  "gl_Position = mvp*vec4(position+pos,1.); }\0";
static const GLchar *dfs = "#version 430\n"
  "out vec4 scolor;\n"
  //"in vec3 frag_norm; in vec3 frag_col;\n"
  "void main() { vec3 light = -vec3(0.,-1.,0.); vec3 col = vec3(0.1,0.5,0.7);\n"
  "  scolor = vec4(1.,0,0,1.); }\0";

// NOTE: MAX_COMPUTE_WORK_GROUP_INVOCATIONS = 1792
static const GLchar *dcs = "#version 430\n"
  "layout (std140, binding = 0) buffer Pos {\n"
  "  vec4 pos[]; };\n"
  "layout (local_size_x = 16) in;\n"
  "uniform float seed; uniform int time; uniform int p_amt;\n"
  "int lcg_rand(int x) { return (x >> 1)^(-(x & 1) & 0x80200003); }\n"
  "float randf(float x) { return fract(sin(x*12.9898)*43758.5453); }\n"
  "float randv(vec2 co) { return fract(sin(dot(co.xy,vec2(12.9898,78.233)))*43758.5453); }\n"
  "void main() { uint id = gl_GlobalInvocationID.x;\n"
  /*"  int rand = lcg_rand(int(gl_WorkGroupID)+seed);\n"
  "  vec4 cpos = pos[id];\n"
  "  rand = lcg_rand(rand); float tht = float(rand%360)*3.14159265/180.;\n"
  "  vec4 npos = vec4(0.01*cos(tht),0.01*sin(tht),0.,0.);\n"*/
  /*"  vec4 cpos = pos[id]; float f_id = id;\n"
  //"  float r = randv(vec2(f_id/float(gl_NumWorkGroups*gl_WorkGroupSize),seed/2147483647.));\n"
  "  float r = randv(cpos.xy+vec2(f_id/float(gl_NumWorkGroups*gl_WorkGroupSize),seed/2147483647.));\n"
  "  float tht = r*3.14159265*2;\n"
  "  vec4 npos = vec4(0.03*cos(tht),0.03*sin(tht),0.,0.);\n"
  "  pos[id] = cpos + npos; }\n";*/
  "  vec4 cpos = pos[id]; float f_id = id;\n"
  "  float r = randv(cpos.xy+vec2(f_id/float(gl_NumWorkGroups*gl_WorkGroupSize),seed/2147483647.));\n"
  "  if(cpos.y>2) { pos[id] = cpos-vec4(0.,0.02,0.,0.); }\n"
  "  else if(cpos.y<=2&&cpos.y>-1) { pos[id] = cpos+vec4(r>0.5?-0.04:0.04,-0.02,0.,0.); }\n"
  "  else if(id>399) { int b_id = clamp(int((cpos.x+2.)*100.),0,399);\n"
  "    pos[b_id].y += 0.01; pos[id].y += 5000.; } }\n";

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

GLuint create_compute_program(const GLchar *csh) { GLuint cs;
  cs = glCreateShader(GL_COMPUTE_SHADER);

  glShaderSource(cs,1,&csh,NULL); glCompileShader(cs);
  
  GLint compiled = 0;
  glGetShaderiv(cs,GL_COMPILE_STATUS,&compiled);
  if(!compiled) { printf("Compute error:\n"); GLint mlen = 0;
    glGetShaderiv(cs,GL_INFO_LOG_LENGTH, &mlen);
    std::vector<GLchar> error_log(mlen);
    glGetShaderInfoLog(cs,mlen,&mlen,&error_log[0]);
    glDeleteShader(cs); printf("%s\n",&error_log[0]); }
  GLuint prog; prog = glCreateProgram();
  glAttachShader(prog,cs);
  /*glBindAttribLocation(prog,0,"position");
  glBindAttribLocation(prog,1,"norm");*/ glLinkProgram(prog);
  glDeleteShader(cs); return prog; }

void mvp_set(GLuint prog, glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
  glUseProgram(prog);

  /*GLint _model = glGetUniformLocation(prog,"model");
  glUniformMatrix4fv(_model,1,GL_FALSE,(const float *)glm::value_ptr(model));
  GLint _view = glGetUniformLocation(prog,"view");
  glUniformMatrix4fv(_view,1,GL_FALSE,(const float *)glm::value_ptr(view));
  GLint _projection = glGetUniformLocation(prog,"projection");
  glUniformMatrix4fv(_projection,1,GL_FALSE,(const float *)glm::value_ptr(projection));*/

  glm::mat4 mvp = projection*view*model;
  GLint _mvp = glGetUniformLocation(prog,"mvp");
  glUniformMatrix4fv(_mvp,1,GL_FALSE,(const float *)glm::value_ptr(mvp)); }

// TODO: generalize.
void vec_set(GLuint prog, glm::vec3 a, std::string name) {
  glUseProgram(prog);

  GLint _name = glGetUniformLocation(prog,name.c_str());
  glUniform3f(_name,a.x,a.y,a.z); }
void float_set(GLuint prog, float a, std::string name) {
  glUseProgram(prog);

  GLint _name = glGetUniformLocation(prog,name.c_str());
  glUniform1f(_name,a); }
void int_set(GLuint prog, int a, std::string name) {
  glUseProgram(prog);

  GLint _name = glGetUniformLocation(prog,name.c_str());
  glUniform1i(_name,a); }
