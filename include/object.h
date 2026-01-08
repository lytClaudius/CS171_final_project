#ifndef _object_H_
#define _object_H_
#include <shader.h>

#include <vector>

#include "defines.h"

// you can modify this structure to support texture binding
struct Vertex
{
    vec3 position;
    vec3 normal;
};

struct DrawMode
{
    GLenum primitive_mode;
};

class Object
{
private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Vertex> basic_vertices;
    std::vector<GLuint> basic_indices;

    vec3 position;

    DrawMode draw_mode;

    Object();
    ~Object();

    void init(const std::vector<Vertex> &verts,
              const std::vector<GLuint> &inds, GLenum mode);
    void UpdateBufferData();

    std::vector<GLuint> quad_to_triangles(const std::vector<GLuint> &quad_indices);
    void drawArrays() const;
    void drawArrays(const Shader &shader) const;
    void drawElements() const;
    void drawElements(const Shader &shader) const;
    void CatmullClarkSubdivision(int level = 1);
    void calculateNormals();
    void generateBezierSurface(const std::vector<std::vector<vec3>> &control_points);
};
#endif