#include "Batch3D.hpp"

#include "Mesh.hpp"
#include "Texture.hpp"

#include <GL/glew.h>
#include <typedefs.hpp>
#include <maths/UVRegion.hpp>

/// xyz, uv, rgba
inline constexpr uint B3D_VERTEX_SIZE = 9;

Batch3D::Batch3D(size_t capacity) 
    : capacity(capacity) {
    const vattr attrs[] = {
        {3}, {2}, {4}, {0}
    };

    buffer = std::make_unique<float[]>(capacity * B3D_VERTEX_SIZE);
    mesh = std::make_unique<Mesh>(buffer.get(), 0, attrs);
    index = 0;

    const ubyte pixels[] = {
        255, 255, 255, 255,
    };
    ImageData image(ImageFormat::rgba8888, 1, 1, pixels);
    blank = Texture::from(&image);
    currentTexture = nullptr;
}

Batch3D::~Batch3D(){
}

void Batch3D::begin(){
    currentTexture = nullptr;
    blank->bind();
}

void Batch3D::vertex(
    float x, float y, float z, float u, float v,
    float r, float g, float b, float a
) {
    buffer[index++] = x;
    buffer[index++] = y;
    buffer[index++] = z;
    buffer[index++] = u;
    buffer[index++] = v;
    buffer[index++] = r;
    buffer[index++] = g;
    buffer[index++] = b;
    buffer[index++] = a;
}
void Batch3D::vertex(
    glm::vec3 coord, float u, float v,
    float r, float g, float b, float a
) {
    buffer[index++] = coord.x;
    buffer[index++] = coord.y;
    buffer[index++] = coord.z;
    buffer[index++] = u;
    buffer[index++] = v;
    buffer[index++] = r;
    buffer[index++] = g;
    buffer[index++] = b;
    buffer[index++] = a;
}
void Batch3D::vertex(
    glm::vec3 point,
    glm::vec2 uvpoint,
    float r, float g, float b, float a
) {
    buffer[index++] = point.x;
    buffer[index++] = point.y;
    buffer[index++] = point.z;
    buffer[index++] = uvpoint.x;
    buffer[index++] = uvpoint.y;
    buffer[index++] = r;
    buffer[index++] = g;
    buffer[index++] = b;
    buffer[index++] = a;
}

void Batch3D::face(
    const glm::vec3& coord, 
    float w, float h,
    const glm::vec3& axisX,
    const glm::vec3& axisY,
    const UVRegion& region,
    const glm::vec4& tint
) {
    if (index + B3D_VERTEX_SIZE * 6 > capacity) {
        flush();
    }
    vertex(coord, region.u1, region.v1, 
           tint.r, tint.g, tint.b, tint.a);
    vertex(coord + axisX * w, region.u2, region.v1, 
            tint.r, tint.g, tint.b, tint.a);
    vertex(coord + axisX * w + axisY * h, region.u2, region.v2, 
           tint.r, tint.g, tint.b, tint.a);

    vertex(coord, region.u1, region.v1, 
           tint.r, tint.g, tint.b, tint.a);
    vertex(coord + axisX * w + axisY * h, region.u2, region.v2,
           tint.r, tint.g, tint.b, tint.a);
    vertex(coord + axisY * h, region.u1, region.v2, 
           tint.r, tint.g, tint.b, tint.a);
}

void Batch3D::texture(Texture* new_texture){
    if (currentTexture == new_texture)
        return;
    flush();
    currentTexture = new_texture;
    if (new_texture == nullptr)
        blank->bind();
    else
        new_texture->bind();
}

void Batch3D::sprite(
    glm::vec3 pos, 
    glm::vec3 up, 
    glm::vec3 right, 
    float w, float h, 
    const UVRegion& uv, 
    glm::vec4 color
){
    const float r = color.r;
    const float g = color.g;
    const float b = color.b;
    const float a = color.a;
    if (index + 6*B3D_VERTEX_SIZE >= capacity) {
        flush();
    }

    vertex(pos.x - right.x * w - up.x * h,
            pos.y - right.y * w - up.y * h,
            pos.z - right.z * w - up.z * h,
            uv.u1, uv.v1,
            r,g,b,a);

    vertex(pos.x + right.x * w + up.x * h,
            pos.y + right.y * w + up.y * h,
            pos.z + right.z * w + up.z * h,
            uv.u2, uv.v2,
            r,g,b,a);

    vertex(pos.x - right.x * w + up.x * h,
            pos.y - right.y * w + up.y * h,
            pos.z - right.z * w + up.z * h,
            uv.u1, uv.v2,
            r,g,b,a);

    vertex(pos.x - right.x * w - up.x * h,
            pos.y - right.y * w - up.y * h,
            pos.z - right.z * w - up.z * h,
            uv.u1, uv.v1,
            r,g,b,a);

    vertex(pos.x + right.x * w - up.x * h,
            pos.y + right.y * w - up.y * h,
            pos.z + right.z * w - up.z * h,
            uv.u2, uv.v1,
            r,g,b,a);

    vertex(pos.x + right.x * w + up.x * h,
            pos.y + right.y * w + up.y * h,
            pos.z + right.z * w + up.z * h,
            uv.u2, uv.v2,
            r,g,b,a);
}

inline glm::vec4 do_tint(float value) {
    return glm::vec4(value, value, value, 1.0f);
}

void Batch3D::xSprite(
    float w, float h, const UVRegion& uv, const glm::vec4 tint, bool shading
) {
    face(
        glm::vec3(-w * 0.25f, 0.0f, -w * 0.25f), 
        w, h, 
        glm::vec3(1, 0, 0), 
        glm::vec3(0, 1, 0), 
        uv, (shading ? do_tint(1.0f)*tint : tint)
    );
    face(
        glm::vec3(w * 0.25f, 0.0f, w * 0.5f - w * 0.25f), 
        w, h,
        glm::vec3(0, 0, -1), 
        glm::vec3(0, 1,  0), 
        uv, (shading ? do_tint(0.9f)*tint : tint)
    );
}

void Batch3D::cube(
    const glm::vec3 coord,
    const glm::vec3 size,
    const UVRegion(&texfaces)[6],
    const glm::vec4 tint,
    bool shading
) {
    const glm::vec3 X(1.0f, 0.0f, 0.0f);
    const glm::vec3 Y(0.0f, 1.0f, 0.0f);
    const glm::vec3 Z(0.0f, 0.0f, 1.0f);

    face(
        coord+glm::vec3(0.0f, 0.0f, 0.0f), 
        size.x, size.y, X, Y, texfaces[5], 
        (shading ? do_tint(0.8)*tint : tint)
    );
    face(
        coord+glm::vec3(size.x, 0.0f, -size.z), 
        size.x, size.y, -X, Y, texfaces[4], 
        (shading ? do_tint(0.8f)*tint : tint)
    );
    face(
        coord+glm::vec3(0.0f, size.y, 0.0f), 
        size.x, size.z, X, -Z, texfaces[3], 
        (shading ? do_tint(1.0f)*tint : tint)
    );
    face(
        coord+glm::vec3(0.0f, 0.0f, -size.z), 
        size.x, size.z, X, Z, texfaces[2], 
        (shading ? do_tint(0.7f)*tint : tint)
    );
    face(
        coord+glm::vec3(0.0f, 0.0f, -size.z), 
        size.z, size.y, Z, Y, texfaces[0], 
        (shading ? do_tint(0.9f)*tint : tint)
    );
    face(
        coord+glm::vec3(size.x, 0.0f, 0.0f), 
        size.z, size.y, -Z, Y, texfaces[1], 
        (shading ? do_tint(0.9f)*tint : tint)
    );
}

void Batch3D::blockCube(
    const glm::vec3 size, 
    const UVRegion(&texfaces)[6], 
    const glm::vec4 tint, 
    bool shading
) {
    cube((1.0f - size) * -0.5f, size, texfaces, tint, shading);
}

void Batch3D::point(glm::vec3 coord, glm::vec2 uv, glm::vec4 tint) {
    vertex(coord, uv, tint.r, tint.g, tint.b, tint.a);
}

void Batch3D::point(glm::vec3 coord, glm::vec4 tint) {
    point(coord, glm::vec2(), tint);
}

void Batch3D::flush() {
    mesh->reload(buffer.get(), index / B3D_VERTEX_SIZE);
    mesh->draw();
    index = 0;
}

void Batch3D::flushPoints() {
    mesh->reload(buffer.get(), index / B3D_VERTEX_SIZE);
    mesh->draw(GL_POINTS);
    index = 0;
}
