#include "Snail.h"
#include <glm/gtc/matrix_transform.hpp>
#include <common/model.h>
#include <iostream>

using namespace glm;

Snail::Snail(
    vec3 pos, vec3 vel, float scalar, float mass,
    vec3 anchor, float stiffness, float damping)
    : a(anchor), k(stiffness), b(damping) {
    mesh = new Drawable("models/Mesh_Snail.obj");

    s = scalar;
    m = mass;
    x = pos;
    v = vel;
    P = m * v;
   
}

Snail::~Snail() {
    delete mesh;
}

void Snail::draw() {
    mesh->bind();
    mesh->draw();
    
}

void Snail::update(float t, float dt) {
    //integration
    advanceState(t, dt);

    float springLength = glm::distance(a, x);

    // compute model matrix
    mat4 scale = glm::scale(mat4(), vec3(s, s, s));
    mat4 tranlation = translate(mat4(), vec3(x.x, x.y, x.z));
#ifdef USE_QUATERNIONS
    mat4 rotation = mat4_cast(q);
#else
    mat4 rotation = mat4(R);
#endif
    snailModelMatrix = tranlation * rotation  * scale;
}
