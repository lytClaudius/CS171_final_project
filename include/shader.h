#ifndef _SHADER_H_
#define _SHADER_H_
#include <string>
#include <vector>

#include "defines.h"

class Shader
{
public:
    Shader() = default;
    /**
     * init shader with shader files
     */
    Shader(const std::string &vsPath, const std::vector<std::string> &fsPaths);
    void init(const std::string &vsPath, const std::string &fsPath);
    /**
     * init shader with shader codes
     */
    void initWithCode(const std::string &vs, const std::string &fs);
    /**
     * get shader code from a file
     */
    static std::string getCodeFromFile(const std::string &path);
    /**
     * use shader
     */
    void use() const;
    /**
     * get a uniform variable's location according to its name
     */
    GLint getUniformLocation(const std::string &name) const;
    /**
     * set value of uniform variables
     */
    std::string getCodeFromFiles(const std::vector<std::string> &paths);
    void setInt(const std::string &name, GLint value) const;
    void setFloat(const std::string &name, GLfloat value) const;
    void setMat3(const std::string &name, const mat3 &value) const;
    void setMat4(const std::string &name, const mat4 &value) const;
    void setVec2(const std::string &name, const vec2 &value) const;
    void setVec3(const std::string &name, const vec3 &value) const;
    void setVec4(const std::string &name, const vec4 &value) const;

private:
    GLuint id = 0;
};
#endif