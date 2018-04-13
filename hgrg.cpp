#include <SFML/Graphics.hpp>
#include <RtAudio.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#define GLEW_STATIC
#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

#include <cstdio>
#include <cmath>
#include <ctime>
#include <cfloat>

#include "shaders.hpp"

#define PI 3.1415926535
#define STRIDE (7)

#define PARTICLE_AMT (385024)

#define SAMPLE_RATE (44100)
#define FRAME_RATE (60)
//#define FRAME_COUNT (15)
//#define SAMP_PER_BLOCK (SAMPLE_RATE/FRAME_RATE*FRAME_COUNT)
#define SAMPLES_PER_FRAME (SAMPLE_RATE/FRAME_RATE)

//float signum(float a) { (a>0)-(a<0); }

struct V4f { float pos[4]; /*float col[4];*/ };
struct VAOdat { GLuint vao; int disp; VAOdat(GLuint _v, int _d) { vao=_v; disp=_d; } };
struct PState { glm::vec3 pos; glm::vec3 vel; glm::vec3 acc;
  glm::vec3 av; glm::vec3 torque;
  glm::vec3 head; glm::vec3 left;
  PState(glm::vec3 _p) { pos=_p; vel=glm::vec3(0,0,0); acc=glm::vec3(0,0,0);
   head=glm::vec3(0,0,0); left=glm::vec3(1,0,0); } };
void update(PState &s) { s.pos += s.vel; s.vel += s.acc; }

void d_rows(VAOdat vd, int xnsteps, int znsteps, GLenum mode) {
  glBindVertexArray(vd.vao);
  for(int i=0;i<znsteps;i++) { glDrawArrays(mode,vd.disp+i*xnsteps*2,xnsteps*2); } }

glm::mat4 gl_init(sf::Window *window) { glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glClearDepth(1.f);
  glDepthFunc(GL_LESS); glEnable(GL_CULL_FACE);
  glDisable(GL_LIGHTING); /* temporary */ glViewport(0,0,window->getSize().x, window->getSize().y);
  float ratio = window->getSize().x/window->getSize().y;
  return glm::perspective(glm::radians(45.f),4.f/3.f,0.1f,100.f); }

void paint(GLuint prog, GLuint compute_prog, int pc, VAOdat vd, GLuint ssbo) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(compute_prog);
  glDispatchCompute(pc/32,1,1);
  glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  glUseProgram(prog); glGetError();
  //glBindVertexArray(vd.vao);
  glBindBuffer(GL_ARRAY_BUFFER,ssbo); 
  GLuint pos_attrib = 0; //GLuint col_attrib = 1; //GLuint norm_attrib = 2;
  glEnableVertexAttribArray(pos_attrib);
  glVertexAttribPointer(pos_attrib,4,GL_FLOAT,GL_FALSE,0,0);
  //glPointSize(16);
  /*d_rows(vd,3,1,GL_POINTS);*/ glDrawArrays(GL_POINTS,0,pc); }

int saw(void *o_buf, void *, unsigned int n_frames, double, RtAudioStreamStatus status
       ,void *data) {
  float *buffer = (float *)o_buf;
  double *d = (double *)data;
  if(status) { printf("stream underflow!\n"); }
  *buffer += sin(2*M_PI*(*d)/SAMPLE_RATE); buffer++; return 0; }
void e_call(RtAudioError::Type t, const std::string &errorText) { return; }

void reset_ssbo(int p_amt) {
  V4f *pos_v = (V4f *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,p_amt*sizeof(V4f),GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  for(int i=0;i<p_amt;i++) { pos_v[i].pos[0] = pos_v[i].pos[1] = pos_v[i].pos[2] = 0.f; }
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); }

void report_ssbo(int p_amt) {
  V4f *pos_v = (V4f *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,p_amt*sizeof(V4f),GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  for(int i=0;i<16;i++) {
    printf("pos %i: %g,%g,%g\n", i,pos_v[i].pos[0],pos_v[i].pos[1],pos_v[i].pos[2]); }
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); }

// WARNING: edits velocity of object rather than acceleration.
void handle_input(PState &s, glm::mat4x4 *view) { glm::vec3 head = glm::vec3(0,0,0);
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    head += glm::vec3(0,1,0); }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    head += glm::vec3(-1,0,0); }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    head += glm::vec3(0,-1,0); }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    head += glm::vec3(1,0,0); }
  glm::vec3 nhead = s.head+head;
  if(length(nhead)>0) { s.head = glm::normalize(s.head+head); } else { s.head = nhead; }

  if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
    *view = rotate(glm::mat4(1.),(float)0.01,glm::vec3(1,0,0))*(*view); }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
    *view = rotate(glm::mat4(1.),(float)-0.01,glm::vec3(1,0,0))*(*view); }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
    *view = rotate(glm::mat4(1.),(float)0.01,glm::vec3(0,1,0))*(*view); }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
    *view = rotate(glm::mat4(1.),(float)-0.01,glm::vec3(0,1,0))*(*view); }
  if(length(head)) { s.vel += 0.001f*s.head; }
  update(s); }

int main() { sf::ContextSettings settings;
  settings.depthBits = 24; settings.stencilBits = 8; settings.antialiasingLevel = 0;
  settings.majorVersion = 4; settings.minorVersion = 3;
  sf::Window window(sf::VideoMode(1280,800), "rgrg", sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true); glewExperimental = GL_TRUE;
  glm::mat4 projection = gl_init(&window);
  glewInit();

  glm::mat4 model = glm::mat4(1.f);
  glm::mat4 view = glm::translate(glm::mat4(1.f),glm::vec3(0.f,0.f,-15.5f));


  double d = 0.f;
  RtAudio dac; if(dac.getDeviceCount() < 1) { printf("No audio devices found.\n"); exit(1); }

  unsigned int buffer_frames = SAMPLE_RATE/SAMPLES_PER_FRAME;
  RtAudio::StreamParameters params;
  params.deviceId = 0; params.nChannels = 2; params.firstChannel = 0;
  if(!params.deviceId) { params.deviceId = dac.getDefaultOutputDevice(); }
  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED;
  try {
    dac.openStream(&params,NULL,RTAUDIO_FLOAT32,SAMPLE_RATE,&buffer_frames,&saw,(void *)&d,&options,&e_call);
    dac.startStream(); }
  catch(RtAudioError &e) { e.printMessage();
    if(dac.isStreamOpen()) { dac.closeStream(); } };

  GLuint default_program = create_program(dvs,dfs);
  mvp_set(default_program,model,view,projection);
  GLuint compute_program = create_compute_program(dcs);

  /*float datarr[] {
    0,0,0, 1,0,0, 0,0,-1
   ,1,0,0, 1,0,0, 0,0,-1
   ,0,1,0, 1,0,0, 0,0,-1 };
  std::vector<float> dat(datarr, datarr+sizeof(datarr)/sizeof(float));*/
  std::vector<V4f> dat(PARTICLE_AMT,(V4f){ {0,0,0,0} });

  // cannot use VAO since buffer must be rebound as GL_VERTEX_ARRAY in draw calls.
  GLuint vao; glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);
  GLuint buf; glGenBuffers(1,&buf);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,buf);
  glBufferData(GL_SHADER_STORAGE_BUFFER,dat.size()*sizeof(V4f),NULL,GL_STATIC_DRAW);
  reset_ssbo(dat.size());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf);

  /*GLuint pos_attrib = 0; //GLuint col_attrib = 1; //GLuint norm_attrib = 2;
  glEnableVertexAttribArray(pos_attrib);
  glVertexAttribPointer(pos_attrib,3,GL_FLOAT,GL_FALSE,0,0);*/
  
  /*glEnableVertexAttribArray(col_attrib);
  glVertexAttribPointer(col_attrib,4,GL_FLOAT,GL_FALSE,sizeof(V4f)
    ,(const GLvoid *)(3*sizeof(GL_FLOAT)));*/
  
  /*glEnableVertexAttribArray(norm_attrib);
  glVertexAttribPointer(norm_attrib,3,GL_FLOAT,GL_FALSE,STRIDE*sizeof(GL_FLOAT)
    ,(const GLvoid *)(6*sizeof(GL_FLOAT)));*/

  auto vd = VAOdat(vao,0);

  PState s(glm::vec3(0,0,0));
  int t = 0; srand(time(NULL));
  //int_set(default_program,3,"vert_amt");
  for(bool r = true;r;t++) {
    sf::Event e; while(window.pollEvent(e)) { if(e.type==sf::Event::Closed) { r=false; } }
    handle_input(s,&view);
    //report_ssbo(dat.size());
    mvp_set(default_program,model,view,projection); vec_set(default_program,s.pos,"pos");
    float_set(default_program,atan2(s.head.y,s.head.x),"tht");
    int_set(default_program,time(NULL)+t,"seed");
    float_set(compute_program,(float)(time(NULL)+t),"seed");
    paint(default_program,compute_program,PARTICLE_AMT,vd,buf); window.display(); }
  glDeleteVertexArrays(1,&vd.vao);
  return 0; }
