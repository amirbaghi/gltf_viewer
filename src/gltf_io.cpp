// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#include "gltf_io.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <bitset>

namespace json = rapidjson;  // Use shorter alias for namespace

namespace gltf {

// Base64 characters used for decoding
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

// Check if a character is a valid base64 character
static inline bool is_base64(BYTE c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

// Decode a base64 string to a vector of bytes
// Code from https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
std::vector<unsigned char> base64_decode(std::string const &encoded_string)
{
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    BYTE char_array_4[4], char_array_3[3];
    std::vector<BYTE> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) char_array_4[j] = 0;

        for (j = 0; j < 4; j++) char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

static bool load_file_to_bytebuffer(const std::string &filename, std::vector<char> &buffer)
{
    FILE *stream = std::fopen(filename.c_str(), "rb+");
    if (!stream) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return false;
    }

    std::fseek(stream, 0, SEEK_END);
    const uint32_t numBytes = std::ftell(stream) + 1;
    std::rewind(stream);

    buffer.resize(numBytes);
    buffer.resize(std::fread(&buffer[0], sizeof(char), numBytes, stream));
    std::fclose(stream);
    return true;
}

// Load a base64 encoded file to a byte buffer
<<<<<<< Updated upstream
static bool load_byte64_file_to_bytebuffer(const std::vector<unsigned char> decoded_data, std::vector<char>& buffer) {
    
=======
static bool load_byte64_file_to_bytebuffer(const std::vector<unsigned char> decoded_data,
                                           std::vector<char> &buffer)
{

>>>>>>> Stashed changes
    int numBytes = decoded_data.size();

    buffer.resize(numBytes);
    buffer.assign(decoded_data.begin(), decoded_data.end());
    return true;
}

static bool load_image_to_bytebuffer(const std::string &filename, std::vector<char> &buffer,
                                     int &width, int &height)
{
    // Load image file (ask for RGBA format with four components)
    int w, h, c;
    uint8_t *image = stbi_load(filename.c_str(), &w, &h, &c, 4);
    if (image == nullptr) {
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        return false;
    }

    width = w, height = h;
    buffer.resize(width * height * 4);
    std::memcpy(&buffer[0], image, width * height * 4);
    stbi_image_free(image);  // Clean up resources
    return true;
}

// Load a base64 encoded image to a byte buffer
<<<<<<< Updated upstream
static bool load_base64_image_to_bytebuffer(const std::vector<unsigned char> &decoded_data, std::vector<char> &buffer)
{
    // Load image file (ask for RGBA format with four components)
    int w, h, c;
    uint8_t *image = stbi_load_from_memory((const stbi_uc *)decoded_data.data(), decoded_data.size(), &w, &h, &c, 4);
=======
static bool load_base64_image_to_bytebuffer(const std::vector<unsigned char> decoded_data,
                                            std::vector<char> &buffer, int &width, int &height)
{
    // Load image file (ask for RGBA format with four components)
    int w, h, c;
    uint8_t *image = stbi_load_from_memory((const stbi_uc *)decoded_data.data(),
                                           decoded_data.size(), &w, &h, &c, 4);
>>>>>>> Stashed changes
    if (image == nullptr) {
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
<<<<<<< Updated upstream

    buffer.resize(w * h * 4);
    std::memcpy(&buffer[0], image, w * h * 4);
    stbi_image_free(image);  // Clean up resources
=======
    width = w, height = h;
    buffer.resize(w * h * 4);
    std::memcpy(&buffer[0], image, w * h * 4);
    stbi_image_free(image);  // Clean up resources

>>>>>>> Stashed changes
    return true;
}

static std::vector<Scene> create_scenes_from_json(const json::Value &value)
{
    std::vector<Scene> scenes(value.Size());
    for (uint32_t i = 0; i < value.Size(); ++i) {
        if (value[i].HasMember("name")) {
            // Note: this attribute seems to be optional
            scenes[i].name = value[i]["name"].GetString();
        }

        if (value[i].HasMember("nodes")) {
            const json::Value &tmp = value[i]["nodes"];
            scenes[i].nodes.resize(tmp.Size());
            for (uint32_t j = 0; j < tmp.Size(); ++j) {
                scenes[i].nodes[j] = int32_t(tmp[j].GetInt());
            }
        }
    }
    return scenes;
}

static std::vector<Node> create_nodes_from_json(const json::Value &value)
{
    std::vector<Node> nodes(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        nodes[i].mesh = value[i]["mesh"].GetInt();

        if (value[i].HasMember("name")) {
            // Note: this attribute seems to be optional
            nodes[i].name = value[i]["name"].GetString();
        }

        if (value[i].HasMember("children")) {
            const json::Value &tmp = value[i]["children"];
            nodes[i].children.resize(tmp.Size());
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].children[j] = float(tmp[j].GetInt());
            }
        }

        if (value[i].HasMember("translation")) {
            const json::Value &tmp = value[i]["translation"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].translation[j] = float(tmp[j].GetDouble());
            }
        } else {
            nodes[i].translation = glm::vec3(0.0f);
        }

        if (value[i].HasMember("rotation")) {
            const json::Value &tmp = value[i]["rotation"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].rotation[j] = float(tmp[j].GetDouble());
            }
        } else {
            nodes[i].rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        if (value[i].HasMember("scale")) {
            const json::Value &tmp = value[i]["scale"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].scale[j] = float(tmp[j].GetDouble());
            }
        } else {
            nodes[i].scale = glm::vec3(1.0f);
        }

        nodes[i].hasMatrix = false;
        if (value[i].HasMember("matrix")) {
            const json::Value &tmp = value[i]["matrix"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                // Note: matrix is stored in column major array order
                nodes[i].matrix[j / 4][j % 4] = float(tmp[j].GetDouble());
            }
            nodes[i].hasMatrix = true;
        } else {
            nodes[i].matrix = glm::mat4(1.0f);
        }
    }
    return nodes;
}

static MaterialTexture create_material_texture_from_json(const json::Value &value)
{
    MaterialTexture materialTexture;
    materialTexture.index = value["index"].GetInt();

    if (value.HasMember("texCoord")) {
        materialTexture.texCoord = value["texCoord"].GetInt();
    } else {
        materialTexture.texCoord = 0;
    }

    if (value.HasMember("scale")) {
        materialTexture.scale = float(value["scale"].GetDouble());
    } else {
        materialTexture.scale = 1.0f;
    }

    if (value.HasMember("strength")) {
        materialTexture.strength = float(value["strength"].GetDouble());
    } else {
        materialTexture.strength = 1.0f;
    }
    return materialTexture;
}

static PBRMetallicRoughness create_pbr_metallic_roughness_from_json(const json::Value &value)
{
    PBRMetallicRoughness pbrMetallicRoughness;

    if (value.HasMember("baseColorFactor")) {
        const json::Value &tmp = value["baseColorFactor"];
        for (unsigned i = 0; i < tmp.Size(); ++i) {
            pbrMetallicRoughness.baseColorFactor[i] = float(tmp[i].GetDouble());
        }
    } else {
        pbrMetallicRoughness.baseColorFactor = glm::vec4(1.0f);
    }

    if (value.HasMember("roughnessFactor")) {
        pbrMetallicRoughness.roughnessFactor = float(value["roughnessFactor"].GetDouble());
    } else {
        pbrMetallicRoughness.roughnessFactor = 1.0f;
    }

    if (value.HasMember("metallicFactor")) {
        pbrMetallicRoughness.metallicFactor = float(value["metallicFactor"].GetDouble());
    } else {
        pbrMetallicRoughness.metallicFactor = 0.0f;
    }

    if (value.HasMember("baseColorTexture")) {
        pbrMetallicRoughness.baseColorTexture =
            create_material_texture_from_json(value["baseColorTexture"]);
        pbrMetallicRoughness.hasBaseColorTexture = true;
    } else {
        pbrMetallicRoughness.hasBaseColorTexture = false;
    }

    if (value.HasMember("metallicRoughnessTexture")) {
        pbrMetallicRoughness.metallicRoughnessTexture =
            create_material_texture_from_json(value["metallicRoughnessTexture"]);
        pbrMetallicRoughness.hasMetallicRoughnessTexture = true;
    } else {
        pbrMetallicRoughness.hasMetallicRoughnessTexture = false;
    }
    return pbrMetallicRoughness;
}

static std::vector<Material> create_materials_from_json(const json::Value &value)
{
    std::vector<Material> materials(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        materials[i].name = value[i]["name"].GetString();
        materials[i].type = DEFAULT_MATERIAL;

        if (value[i].HasMember("pbrMetallicRoughness")) {
            auto pbrMetallicRoughness =
                create_pbr_metallic_roughness_from_json(value[i]["pbrMetallicRoughness"]);
            materials[i].pbrMetallicRoughness = pbrMetallicRoughness;
            materials[i].type = PBR_METALLIC_ROUGHNESS;
        }

        if (value[i].HasMember("normalTexture")) {
            auto materialTexture = create_material_texture_from_json(value[i]["normalTexture"]);
            materials[i].normalTexture = materialTexture;
            materials[i].hasNormalTexture = true;
        } else {
            materials[i].hasNormalTexture = false;
        }

        if (value[i].HasMember("occlusionTexture")) {
            auto materialTexture = create_material_texture_from_json(value[i]["occlusionTexture"]);
            materials[i].occlusionTexture = materialTexture;
            materials[i].hasOcclusionTexture = true;
        } else {
            materials[i].hasOcclusionTexture = false;
        }
    }
    return materials;
}

static std::vector<Texture> create_textures_from_json(const json::Value &value)
{
    std::vector<Texture> textures(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        textures[i].source = value[i]["source"].GetInt();

        if (value[i].HasMember("sampler")) {
            textures[i].sampler = value[i]["sampler"].GetInt();
            textures[i].hasSampler = true;
        } else {
            textures[i].hasSampler = false;
        }
    }
    return textures;
}

static std::vector<Image> create_images_from_json(const json::Value &value)
{
    std::vector<Image> images(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        if (value[i].HasMember("uri")) { images[i].uri = value[i]["uri"].GetString(); }
    }
    return images;
}

static std::vector<Sampler> create_samplers_from_json(const json::Value &value)
{
    std::vector<Sampler> samplers(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        if (value[i].HasMember("magFilter")) {
            samplers[i].magFilter = value[i]["magFilter"].GetInt();
        } else {
            samplers[i].magFilter = 0x2601;  // GL_LINEAR
        }

        if (value[i].HasMember("minFilter")) {
            samplers[i].minFilter = value[i]["minFilter"].GetInt();
        } else {
            samplers[i].minFilter = 0x2601;  // GL_LINEAR
        }

        if (value[i].HasMember("wrapS")) {
            samplers[i].wrapS = value[i]["wrapS"].GetInt();
        } else {
            samplers[i].wrapS = 0x812f;  // GL_CLAMP_TO_EDGE
        }

        if (value[i].HasMember("wrapT")) {
            samplers[i].wrapT = value[i]["wrapT"].GetInt();
        } else {
            samplers[i].wrapT = 0x812f;  // GL_CLAMP_TO_EDGE
        }
    }
    return samplers;
}

static std::vector<Primitive> create_primitives_from_json(const json::Value &value)
{
    std::vector<Primitive> primitives(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        for (const auto &it : value[i]["attributes"].GetObject()) {
            Attribute attribute = {it.name.GetString(), it.value.GetInt()};
            primitives[i].attributes.push_back(attribute);
        }
        primitives[i].indices = value[i]["indices"].GetInt();

        if (value[i].HasMember("material")) {
            primitives[i].material = value[i]["material"].GetInt();
            primitives[i].hasMaterial = true;
        } else {
            primitives[i].hasMaterial = false;
        }
    }
    return primitives;
}

static std::vector<Mesh> create_meshes_from_json(const json::Value &value)
{
    std::vector<Mesh> meshes(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        meshes[i].name = value[i]["name"].GetString();
        if (value[i].HasMember("primitives")) {
            auto primitives = create_primitives_from_json(value[i]["primitives"]);
            meshes[i].primitives = primitives;
        }
    }
    return meshes;
}

static std::vector<Accessor> create_accessors_from_json(const json::Value &value)
{
    std::vector<Accessor> accessors(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        accessors[i].bufferView = value[i]["bufferView"].GetInt();
        accessors[i].componentType = value[i]["componentType"].GetInt();
        accessors[i].count = value[i]["count"].GetInt();
        accessors[i].type = value[i]["type"].GetString();

        if (value[i].HasMember("byteOffset")) {
            accessors[i].byteOffset = value[i]["byteOffset"].GetInt();
        } else {
            accessors[i].byteOffset = 0;
        }
    }
    return accessors;
}

static std::vector<BufferView> create_buffer_views_from_json(const json::Value &value)
{
    std::vector<BufferView> bufferViews(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        bufferViews[i].buffer = value[i]["buffer"].GetInt();
        bufferViews[i].byteLength = value[i]["byteLength"].GetInt();
        bufferViews[i].byteOffset = value[i]["byteOffset"].GetInt();

        if (value[i].HasMember("byteStride")) {
            bufferViews[i].byteStride = value[i]["byteStride"].GetInt();
        } else {
            bufferViews[i].byteStride = 0;
        }
    }
    return bufferViews;
}

static std::vector<Buffer> create_buffers_from_json(const json::Value &value)
{
    std::vector<Buffer> buffers(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        buffers[i].byteLength = value[i]["byteLength"].GetInt();
        buffers[i].uri = value[i]["uri"].GetString();
    }
    return buffers;
}

bool load_gltf_asset(const std::string &filename, const std::string &filedir, GLTFAsset &asset)
{

    json::Document root;
    std::vector<char> buffer;

    if (!load_file_to_bytebuffer(filedir + filename, buffer)) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return false;
    }
    root.Parse(&buffer[0]);

    asset = GLTFAsset();

    if (root.HasMember("scenes")) {
        const auto &scenes = create_scenes_from_json(root["scenes"]);
        asset.scenes = scenes;
    }

    if (root.HasMember("nodes")) {
        auto nodes = create_nodes_from_json(root["nodes"]);
        asset.nodes = nodes;
    }

    if (root.HasMember("materials")) {
        auto materials = create_materials_from_json(root["materials"]);
        asset.materials = materials;
    }

    if (root.HasMember("textures")) {
        auto textures = create_textures_from_json(root["textures"]);
        asset.textures = textures;
    }

    if (root.HasMember("images")) {

        auto images = create_images_from_json(root["images"]);
        // Now also load the actual image data (from image files)
        for (unsigned i = 0; i < images.size(); ++i) {
<<<<<<< Updated upstream
            if (images[i].uri.find(".png") != std::string::npos &&
                images[i].uri.find(".jpeg") != std::string::npos) {
                load_base64_image_to_bytebuffer(base64_decode(images[i].uri), images[i].data);
            }
            else {
=======
            int j = images[i].uri.find("base64");
            if (j != std::string::npos) {
                images[i].uri.erase(0, j + 7);
                load_base64_image_to_bytebuffer(base64_decode(images[i].uri), images[i].data,
                                                images[i].width, images[i].height);
            } else {
>>>>>>> Stashed changes
                load_image_to_bytebuffer(filedir + images[i].uri, images[i].data, images[i].width,
                                         images[i].height);
            }
        }
        asset.images = images;
    }

    if (root.HasMember("samplers")) {
        auto samplers = create_samplers_from_json(root["samplers"]);
        asset.samplers = samplers;
    }

    if (root.HasMember("meshes")) {
        auto meshes = create_meshes_from_json(root["meshes"]);
        asset.meshes = meshes;
    }

    if (root.HasMember("accessors")) {
        auto accessors = create_accessors_from_json(root["accessors"]);
        asset.accessors = accessors;
    }

    if (root.HasMember("bufferViews")) {
        auto bufferViews = create_buffer_views_from_json(root["bufferViews"]);
        asset.bufferViews = bufferViews;
    }

    if (root.HasMember("buffers")) {
        auto buffers = create_buffers_from_json(root["buffers"]);

        // Now also load the actual buffer data (from .bin files/base64 encoded strings)
        for (unsigned i = 0; i < buffers.size(); ++i) {
            // IF the buffer is base64 encoded, decode it first
<<<<<<< Updated upstream
            if (buffers[i].uri.find(".bin")==std::string::npos) {
                load_byte64_file_to_bytebuffer(base64_decode(buffers[i].uri), buffers[i].data);
            }
            // Else, load the buffer from a .bin file 
=======
            if (buffers[i].uri.find(".bin") == std::string::npos) {
                load_byte64_file_to_bytebuffer(base64_decode(buffers[i].uri), buffers[i].data);
            }
            // Else, load the buffer from a .bin file
>>>>>>> Stashed changes
            else {
                load_file_to_bytebuffer(filedir + buffers[i].uri, buffers[i].data);
            }
        }
        asset.buffers = buffers;
    }

    return true;
}

}  // namespace gltf