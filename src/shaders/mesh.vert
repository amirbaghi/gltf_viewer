#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;

// Model, View, Projection
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// Lighting
uniform vec3 u_diffuseColor;
uniform vec3 u_lightPosition;
uniform vec3 u_ambientColor;
uniform vec3 u_specularColor;
uniform float u_specularPower;

// Flags
uniform bool u_useLighting;
uniform bool u_useDiffuseLighting;
uniform bool u_useAmbientLighting;
uniform bool u_useSpecularLighting;
uniform bool u_useNormalsAsColor;

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
    // Calculate MVP matrix
    mat4 mv = u_view * u_model;
    mat4 mvp = u_projection * mv;
    
    // Calculate the coordinates of the vertex
    gl_Position = mvp * vec4(a_position.xyz, 1.0f);

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

    // If should use the normal vectors as the color
    if (u_useNormalsAsColor)
    {
        v_color = 0.5 * a_normal + 0.5;
    }
    // Otherwise
    else
    {
        v_color = a_color;

        // If should use lighting
        if (u_useLighting)
        {
            // Calculate the final color of the vertex by adding the ambient, diffuse, and specular terms
            // multiplied by their respective colors (i.e. Blinn-Phong Lighting) to the color of the object itself.
            if (u_useAmbientLighting)
            {
                v_color += u_ambientColor;
            }
            if (u_useDiffuseLighting)
            {
                v_color += diffuse * u_diffuseColor;
            }
            if (u_useSpecularLighting)
            {
                v_color += specular * u_specularColor;
            }
        }
    }
}
