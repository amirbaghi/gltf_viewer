#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_diffuseColor;
uniform vec3 u_lightPosition;
uniform vec3 u_ambientColor;
uniform vec3 u_specularColor;
uniform float u_specularPower;
// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;
// ...

// Vertex shader outputs
out vec3 v_color;
// ...

void main()
{
    mat4 mv = u_view * u_model;
    mat4 mvp = u_projection * mv;

    gl_Position = mvp * a_position;

    // v_color = 0.5 * a_normal + 0.5;
    
    vec3 positionEye = vec3(mv * a_position);

    // Calculate the view-space normal
    vec3 N = normalize(mat3(mv) * a_normal);

    // Calculate the view-space light direction
    vec3 L = normalize(u_lightPosition - positionEye);

    // Calculate the diffuse (Lambertian) reflection term
    float diffuse = max(0.0, dot(N, L));

    // Calculate the specular term
    vec3 H = normalize(L + (-positionEye));
    float specular = pow(dot(N, H), u_specularPower);
    v_color = u_ambientColor + diffuse*u_diffuseColor + specular * u_specularColor;

    // Multiply the diffuse reflection term with the base surface color
}
