#include "mesh.hpp"

class Rbody : public Mesh
{
public:
  Rbody(string filename);
  ~Rbody();

  //vector<float> weights; // binding weights
  //vector<float> keyframe_times;
  //vector<glm::mat4> keyframe_transforms;

  //GLuint ssbo_animPos; // buffer of animated positions. vec4s.
  GLuint ssbo_triangles; // buffer of triangles as vec4s
};