#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
// Lighting
uniform vec3 u_diffuseColor;
uniform vec3 u_ambientColor;
uniform vec3 u_specularColor;
uniform float u_specularPower;

// Flags
uniform bool u_useLighting;
uniform bool u_useDiffuseLighting;
uniform bool u_useAmbientLighting;
uniform bool u_useSpecularLighting;
uniform bool u_useNormalsAsColor;
uniform bool u_useGammaCorrection;
// ...

// Fragment shader inputs
in vec3 N;
in vec3 L;
in vec3 V;
in vec3 v_normal;
in vec3 v_color;
// ...

// Fragment shader outputs
out vec3 frag_color;

void main()
{
    // Calculate the diffuse (Lambertian) reflection term
    float diffuse = max(0.0, dot(N, L));

    // Calculate the specular term
    vec3 H = normalize(L + V);
    float specular = pow(dot(N, H), u_specularPower);

    // If should use the normal vectors as the color
    if (u_useNormalsAsColor)
    {
        frag_color = 0.5 * v_normal + 0.5;
    }
    // Otherwise
    else
    {
        frag_color = v_color;

        // If should use lighting
        if (u_useLighting)
        {
            // Calculate the final color of the vertex by adding the ambient, diffuse, and specular terms
            // multiplied by their respective colors (i.e. Blinn-Phong Lighting) to the color of the object itself.
            if (u_useAmbientLighting)
            {
                frag_color += u_ambientColor;
            }
            if (u_useDiffuseLighting)
            {
                frag_color += diffuse * u_diffuseColor;
            }
            if (u_useSpecularLighting)
            {
                // Normalize the specular term to make it more visible also
                frag_color += ((u_specularPower + 8) / 8) * u_specularColor * specular;
            }
        }
    }

    if (u_useGammaCorrection)
    {
        frag_color = pow(frag_color, vec3(1 / 2.2));
    }
}
