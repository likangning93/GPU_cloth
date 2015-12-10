#include "mesh.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp> 

class Rbody : public Mesh
{
public:
  Rbody(string filename);
  ~Rbody();

  glm::vec3 translation;
  glm::vec3 scale;
  glm::vec3 eulerRotation;

  //vector<float> weights; // binding weights
  //vector<float> keyframe_times;
  //vector<glm::mat4> keyframe_transforms;

  GLuint ssbo_initPos; // buffer of initial positions. vec4s.
  GLuint ssbo_triangles; // buffer of triangles as vec4s

  glm::mat4 getTransformationAtTime(float dt);

private:
	// two basic "dances"
	glm::mat4 twirl(float t);
	glm::mat4 sineHop(float t);
};