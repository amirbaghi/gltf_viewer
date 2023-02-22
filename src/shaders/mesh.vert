#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;

// Model, View, Projection
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// Light position
uniform vec3 u_lightPosition;

// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;
// ...

// Vertex shader outputs
out vec3 N;
out vec3 L;
out vec3 V;
out vec3 v_normal;
out vec3 v_color;
// ...

void main()
{
    // Calculate MVP matrix
    mat4 mv = u_view * u_model;
    mat4 mvp = u_projection * mv;
    
    // Calculate the coordinates of the vertex
    gl_Position = mvp * vec4(a_position.xyz, 1.0f);

    // Calculate the view-space position
    vec3 positionEye = vec3(mv * a_position);
    V = -positionEye;

    // Calculate the view-space normal
    N = normalize(mat3(mv) * a_normal);

    // Calculate the view-space light direction
    L = normalize(u_lightPosition - positionEye);

    // Pass the normal and color to the fragment shader
    v_normal = a_normal;
    v_color = a_color;
}
