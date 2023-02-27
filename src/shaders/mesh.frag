#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
// Lighting
uniform vec3 u_diffuseColor;
uniform vec3 u_ambientColor;
uniform vec3 u_specularColor;
uniform float u_specularPower;

// // Flags
uniform bool u_useLighting;
uniform bool u_useDiffuseLighting;
uniform bool u_useAmbientLighting;
uniform bool u_useSpecularLighting;
uniform bool u_useNormalsAsColor;
uniform bool u_useGammaCorrection;
uniform bool u_useCubemap;

uniform bool u_visualiseTexCoords;
uniform bool u_hasDiffuseTexture;
uniform bool u_hasNormalTexture;
uniform bool u_useDiffuseTexture;
uniform bool u_useNormalTexture;

// Cubemap
uniform samplerCube u_cubemap;

// Textures
uniform sampler2D u_baseColorTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_shadowmap;

uniform mat4 u_shadowFromView;

// ...

// Fragment shader inputs
in vec3 N;
in vec3 L;
in vec3 V;
in vec3 v_normal;
in vec3 v_color;
in vec2 v_texcoord_0;
// ...

// Fragment shader outputs
out vec3 frag_color;

float shadowmap_visibility(sampler2D shadowmap, vec4 shadowPos, float bias)
{
    vec2 delta = vec2(0.5) / textureSize(shadowmap, 0).xy;

    vec2 texcoord = (shadowPos.xy / shadowPos.w) * 0.5 + 0.5;
    float depth = (shadowPos.z / shadowPos.w) * 0.5 + 0.5;
    
    // Sample the shadowmap and compare texels with (depth - bias) to
    // return a visibility value in range [0, 1]. If you take more
    // samples (using delta to offset the texture coordinate), the
    // returned value should be the average of all comparisons.
    float texel = texture(shadowmap, texcoord).r;
    float visibility = float(texel > depth - bias);
    return visibility;
}

// Calculate the tangent space matrix
mat3 tangent_space(vec3 eye, vec2 texcoord, vec3 normal)
{
    // Compute pixel derivatives
    vec3 delta_pos1 = dFdx(eye);
    vec3 delta_pos2 = dFdy(eye);
    vec2 delta_uv1 = dFdx(texcoord);
    vec2 delta_uv2 = dFdy(texcoord);

    // Compute tangent space vectors
    vec3 N = normal;
    vec3 T = normalize(delta_pos1 * delta_uv2.y -
                       delta_pos2 * delta_uv1.y);
    vec3 B = normalize(delta_pos2 * delta_uv1.x -
                       delta_pos1 * delta_uv2.x);
    return mat3(T, B, N);
}

void main()
{
    vec3 normal = N;

    // If should visualise the texture coordinates
    if (u_visualiseTexCoords && (u_hasDiffuseTexture || u_hasNormalTexture))
    {
        frag_color = vec3(v_texcoord_0.x, v_texcoord_0.y, 0.0);
    }
    // If should use the cubemap
    else if (u_useCubemap)
    {
        // Calculate the reflection vector
        vec3 R = reflect(-V, N);

        frag_color = texture(u_cubemap, R).rgb;
    }
    else
    {
        // If should use the normal vectors as the color
        if (u_useNormalsAsColor)
        {
            frag_color = 0.5 * v_normal + 0.5;
        }
        // Otherwise
        else
        {
            // If we have a diffuse texture
            if (u_useDiffuseTexture && u_hasDiffuseTexture)
            {
                frag_color = texture(u_baseColorTexture, v_texcoord_0).rgb;
            }
            else
            {
                frag_color = v_color;
            }

            // If we have a normal texture
            if (u_hasNormalTexture && u_useNormalTexture)
            {
                // Calculate the tangent space matrix
                mat3 TBN = tangent_space(V, v_texcoord_0, N);

                // Calculate the normal vector from the height map
                float delta = 0.0010000000474974513;
                vec3 t = vec3(1, 0, texture(u_normalTexture, v_texcoord_0 + vec2(delta, 0.0)).r - texture(u_normalTexture, v_texcoord_0 + vec2(-delta, 0.0)).r);
                vec3 s = vec3(0, 1, texture(u_normalTexture, v_texcoord_0 + vec2(0.0, delta)).r - texture(u_normalTexture, v_texcoord_0 + vec2(0.0, -delta)).r);
                normal = cross(t, s);

                // Transform the normal vector from tangent space to object space
                normal = normalize(TBN * normal);
            }

            // If should use lighting
            if (u_useLighting)
            {
                // Calculate the diffuse (Lambertian) reflection term
                float diffuse = max(0.0, dot(normal, L)) ;

                // float diffuse = clamp(dot(normal, L), 0, 1);

                // Calculate the specular term
                vec3 H = normalize(L + V);
                float specular = max(0.0, pow(dot(normal, H), u_specularPower)) ;

                // Calculate the final color of the vertex by adding the ambient, diffuse, and specular terms
                // multiplied by their respective colors (i.e. Blinn-Phong Lighting) to the color of the object itself.
                if (u_useAmbientLighting)
                {
                    frag_color += u_ambientColor;
                }
                if (u_useDiffuseLighting)
                {
                    frag_color += diffuse * u_diffuseColor * shadowmap_visibility(u_shadowmap, u_shadowFromView * vec4(-V, 1.0), 0.005);
                }
                if (u_useSpecularLighting)
                {
                    // Normalize the specular term to make it more visible also
                    frag_color += ((u_specularPower + 8) / 8) * u_specularColor * specular *  shadowmap_visibility(u_shadowmap, u_shadowFromView * vec4(-V, 1.0), 0.005);
                }
            }
        }

        if (u_useGammaCorrection)
        {
            frag_color = pow(frag_color, vec3(1 / 2.2));
        }
    }
}
