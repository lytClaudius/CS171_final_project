#include <object.h>
#include <shader.h>
#include <utils.h>

#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <math.h>

Object::Object() {}
Object::~Object() {}

/**
 * TODO: initialize VAO, VBO, VEO and set the related varialbes
 */
void Object::init(const std::vector<Vertex> &verts,
                  const std::vector<GLuint> &inds, GLenum mode)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    vertices = verts;
    indices = inds;

    basic_vertices = verts;
    basic_indices = inds;

    draw_mode.primitive_mode = mode;
    position = vec3(0.0f);
    for (auto &vertex : basic_vertices)
    {
        position += vertex.position;
    }
    position /= static_cast<float>(basic_vertices.size());
}

void Object::UpdateBufferData()
{
    indices = quad_to_triangles(indices);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);

    // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, position));

    // Vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, normal));

    glBindVertexArray(0);
}

std::vector<GLuint> Object::quad_to_triangles(const std::vector<GLuint> &quad_indices)
{
    std::vector<GLuint> triangle_indices;
    for (size_t i = 0; i < quad_indices.size(); i += 4)
    {
        GLuint v0 = quad_indices[i];
        GLuint v1 = quad_indices[i + 1];
        GLuint v2 = quad_indices[i + 2];
        GLuint v3 = quad_indices[i + 3];

        // First triangle
        triangle_indices.push_back(v0);
        triangle_indices.push_back(v1);
        triangle_indices.push_back(v2);

        // Second triangle
        triangle_indices.push_back(v2);
        triangle_indices.push_back(v3);
        triangle_indices.push_back(v0);
    }
    // for (GLuint idx : triangle_indices)
    // {
    //     std::cout << idx << " ";
    // }
    return triangle_indices;
}

/**
 * TODO: draw object with VAO and VBO
 * You can choose to implement either one or both of the following functions.
 */

/* Implement this one if you do not use a shader */
void Object::drawArrays() const
{
    glBindVertexArray(VAO);
    glDrawArrays(draw_mode.primitive_mode, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

/* Implement this one if you do use a shader */
void Object::drawArrays(const Shader &shader) const
{
    shader.use();
    glBindVertexArray(VAO);
    glDrawArrays(draw_mode.primitive_mode, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

/**
 * TODO: draw object with VAO, VBO, and VEO
 * You can choose to implement either one or both of the following functions.
 */

/* Implement this one if you do not use a shader */
void Object::drawElements() const
{
    glBindVertexArray(VAO);
    glDrawElements(draw_mode.primitive_mode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

/* Implement this one if you do use a shader */
void Object::drawElements(const Shader &shader) const
{
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(draw_mode.primitive_mode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

std::pair<GLuint, GLuint> getEdgeKey(GLuint v1, GLuint v2)
{
    if (v1 < v2)
        return {v1, v2};
    else
        return {v2, v1};
}

void Object::CatmullClarkSubdivision(int level)
{
    vertices = basic_vertices;
    indices = basic_indices;
    for (int i = 0; i < level; ++i)
    {
        std::set<std::pair<GLuint, GLuint>> edges_set;
        std::vector<std::pair<GLuint, GLuint>> edges;
        std::vector<std::vector<GLuint>> vertex_faces(vertices.size());      // faces adjacent to each vertex
        std::map<std::pair<GLuint, GLuint>, std::vector<GLuint>> edge_faces; // faces adjacent to each edge
        std::map<GLuint, std::vector<std::pair<GLuint, GLuint>>> face_edges; // edges adjacent to each face

        for (int i = 0; i < indices.size() / 4; ++i)
        {
            GLuint v0 = indices[4 * i];
            GLuint v1 = indices[4 * i + 1];
            GLuint v2 = indices[4 * i + 2];
            GLuint v3 = indices[4 * i + 3];

            // Record faces adjacent to vertices
            vertex_faces[v0].push_back(i);
            vertex_faces[v1].push_back(i);
            vertex_faces[v2].push_back(i);
            vertex_faces[v3].push_back(i);

            // Record faces adjacent to edges
            edge_faces[getEdgeKey(v0, v1)].push_back(i);
            edge_faces[getEdgeKey(v1, v2)].push_back(i);
            edge_faces[getEdgeKey(v2, v3)].push_back(i);
            edge_faces[getEdgeKey(v3, v0)].push_back(i);

            // Record edges adjacent to faces
            face_edges[i].push_back(getEdgeKey(v0, v1));
            face_edges[i].push_back(getEdgeKey(v1, v2));
            face_edges[i].push_back(getEdgeKey(v2, v3));
            face_edges[i].push_back(getEdgeKey(v3, v0));

            // Collect unique edges
            edges_set.insert(getEdgeKey(v0, v1));
            edges_set.insert(getEdgeKey(v1, v2));
            edges_set.insert(getEdgeKey(v2, v3));
            edges_set.insert(getEdgeKey(v3, v0));
        }
        edges.assign(edges_set.begin(), edges_set.end());

        // identify boundary vertices and their neighboring boundary vertices
        std::set<GLuint> boundary_verts;
        std::map<GLuint, std::vector<GLuint>> vert_to_boundary_neighbors;
        for (const auto &edge_pair : edge_faces)
        {
            if (edge_pair.second.size() == 1)
            {
                GLuint v0 = edge_pair.first.first;
                GLuint v1 = edge_pair.first.second;
                boundary_verts.insert(v0);
                boundary_verts.insert(v1);
                vert_to_boundary_neighbors[v0].push_back(v1);
                vert_to_boundary_neighbors[v1].push_back(v0);
            }
        }

        // face points
        std::vector<Vertex> face_points;
        for (int i = 0; i < indices.size() / 4; ++i)
        {
            GLuint v0 = indices[4 * i];
            GLuint v1 = indices[4 * i + 1];
            GLuint v2 = indices[4 * i + 2];
            GLuint v3 = indices[4 * i + 3];

            vec3 face_point = (vertices[v0].position + vertices[v1].position +
                               vertices[v2].position + vertices[v3].position) *
                              0.25f;
            face_points.push_back({face_point});
        }
        // edge points
        std::map<std::pair<GLuint, GLuint>, int> edge_points_idx;
        std::vector<Vertex> edge_points;
        for (const auto &edge : edges)
        {
            GLuint v0 = edge.first;
            GLuint v1 = edge.second;

            // specail:boundary edge
            if (edge_faces[edge].size() == 1)
            {
                vec3 edge_point = (vertices[v0].position + vertices[v1].position) * 0.5f;
                edge_points_idx[edge] = static_cast<int>(vertices.size() + face_points.size() + edge_points.size());
                edge_points.push_back({edge_point});
                continue;
            }

            vec3 edge_point = (vertices[v0].position + vertices[v1].position) * 0.25f;
            for (const auto &face_idx : edge_faces[edge])
            {
                edge_point = edge_point + face_points[face_idx].position * 0.25f;
            }
            edge_points_idx[edge] = static_cast<int>(vertices.size() + face_points.size() + edge_points.size());
            edge_points.push_back({edge_point});
        }
        // new vertex positions
        std::vector<Vertex> new_vertex_positions(vertices.size());
        for (int i = 0; i < vertices.size(); ++i)
        {
            if (boundary_verts.count(i))
            {
                // Apply boundary vertex rules
                const auto &neighbors = vert_to_boundary_neighbors.at(i);
                if (neighbors.size() == 2)
                {
                    // Smooth boundary point: (1/8)P_prev + (6/8)P_curr + (1/8)P_next
                    GLuint v_prev = neighbors[0];
                    GLuint v_curr = i;
                    GLuint v_next = neighbors[1];
                    new_vertex_positions[i].position = vertices[v_prev].position * 0.125f +
                                                       vertices[v_curr].position * 0.75f +
                                                       vertices[v_next].position * 0.125f;
                }
                else
                {
                    new_vertex_positions[i].position = vertices[i].position;
                }
                continue;
            }

            vec3 F(0.0f); // average of face points
            for (const auto &face_idx : vertex_faces[i])
            {
                F = F + face_points[face_idx].position;
            }
            F = F * (1.0f / static_cast<float>(vertex_faces[i].size()));

            vec3 R(0.0f); // average of edge midpoints
            int edge_count = 0;
            for (int j = 0; j < edges.size(); ++j)
            {
                if (edges[j].first == i || edges[j].second == i)
                {
                    vec3 edge_midpoint = (vertices[edges[j].first].position + vertices[edges[j].second].position) * 0.5f;
                    R = R + edge_midpoint;
                    edge_count++;
                }
            }
            R = R * (1.0f / static_cast<float>(edge_count));

            vec3 P = vertices[i].position; // original position

            new_vertex_positions[i].position = (F + R * 2.0f + P * (static_cast<float>(edge_count) - 3.0f)) /
                                               static_cast<float>(edge_count);
        }

        // Rebuild the mesh with new points
        int n_vertices = vertices.size();
        int n_faces = face_points.size();
        int n_edges = edge_points.size();
        std::vector<Vertex> new_vertices = new_vertex_positions;
        new_vertices.insert(new_vertices.end(), face_points.begin(), face_points.end());
        new_vertices.insert(new_vertices.end(), edge_points.begin(), edge_points.end());
        std::vector<GLuint> new_indices;

        for (int i = 0; i < n_faces; ++i)
        {
            GLuint v0 = indices[4 * i];
            GLuint v1 = indices[4 * i + 1];
            GLuint v2 = indices[4 * i + 2];
            GLuint v3 = indices[4 * i + 3];

            GLuint face_point_idx = n_vertices + i;
            GLuint edge_point_idx_0 = edge_points_idx[getEdgeKey(v0, v1)];
            GLuint edge_point_idx_1 = edge_points_idx[getEdgeKey(v1, v2)];
            GLuint edge_point_idx_2 = edge_points_idx[getEdgeKey(v2, v3)];
            GLuint edge_point_idx_3 = edge_points_idx[getEdgeKey(v3, v0)];

            // Create four new quads
            new_indices.push_back(v0);
            new_indices.push_back(edge_point_idx_0);
            new_indices.push_back(face_point_idx);
            new_indices.push_back(edge_point_idx_3);

            new_indices.push_back(v1);
            new_indices.push_back(edge_point_idx_1);
            new_indices.push_back(face_point_idx);
            new_indices.push_back(edge_point_idx_0);

            new_indices.push_back(v2);
            new_indices.push_back(edge_point_idx_2);
            new_indices.push_back(face_point_idx);
            new_indices.push_back(edge_point_idx_1);

            new_indices.push_back(v3);
            new_indices.push_back(edge_point_idx_3);
            new_indices.push_back(face_point_idx);
            new_indices.push_back(edge_point_idx_2);
        }
        vertices = new_vertices;
        indices = new_indices;
        // calculateNormals();
    }
    calculateNormals();
    UpdateBufferData();
}

void Object::calculateNormals()
{
    for (auto &vertex : vertices)
    {
        vertex.normal = vec3(0.0f);
    }

    std::vector<GLuint> tri_indices = quad_to_triangles(indices);

    for (size_t i = 0; i < tri_indices.size(); i += 3)
    {
        GLuint i0 = tri_indices[i];
        GLuint i1 = tri_indices[i + 1];
        GLuint i2 = tri_indices[i + 2];

        Vertex &v0 = vertices[i0];
        Vertex &v1 = vertices[i1];
        Vertex &v2 = vertices[i2];

        vec3 edge1 = v1.position - v0.position;
        vec3 edge2 = v2.position - v0.position;
        vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        v0.normal += faceNormal;
        v1.normal += faceNormal;
        v2.normal += faceNormal;
    }

    for (auto &vertex : vertices)
    {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

int binomialCoefficient(int n, int k)
{
    if (k > n)
        return 0;
    if (k == 0 || k == n)
        return 1;
    k = std::min(k, n - k); // Take advantage of symmetry
    int c = 1;
    for (int i = 0; i < k; ++i)
    {
        c = c * (n - i) / (i + 1);
    }
    return c;
}

float bernsteinPolynomial(int i, int n, float t)
{
    return static_cast<float>(binomialCoefficient(n, i)) * pow(t, i) * pow(1 - t, n - i);
}

void Object::generateBezierSurface(const std::vector<std::vector<vec3>> &control_points)
{
    basic_vertices.clear();
    basic_indices.clear();
    int n = control_points.size() - 1;    // degree in u direction
    int m = control_points[0].size() - 1; // degree in v direction
    int res = 4;

    for (int i = 0; i <= res; ++i)
    {
        float u = static_cast<float>(i) / res;
        for (int j = 0; j <= res; ++j)
        {
            float v = static_cast<float>(j) / res;
            vec3 point(0.0f);
            for (int k = 0; k <= n; ++k)
            {
                for (int l = 0; l <= m; ++l)
                {
                    float bu = bernsteinPolynomial(k, n, u);
                    float bv = bernsteinPolynomial(l, m, v);
                    point += bu * bv * control_points[k][l];
                }
            }
            basic_vertices.push_back({point});
        }
    }
    for (int i = 0; i < res; ++i)
    {
        for (int j = 0; j < res; ++j)
        {
            GLuint v0 = i * (res + 1) + j;
            GLuint v1 = v0 + 1;
            GLuint v2 = v0 + (res + 1) + 1;
            GLuint v3 = v0 + (res + 1);

            basic_indices.push_back(v0);
            basic_indices.push_back(v1);
            basic_indices.push_back(v2);
            basic_indices.push_back(v3);
        }
    }
    // vertices = basic_vertices;
    // indices = basic_indices;
    calculateNormals();
}