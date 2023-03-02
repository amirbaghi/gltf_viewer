// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#pragma once

#include "gltf_scene.h"

#include <string>
#define _BASE64_H_
namespace gltf {

// Code for decoding a base64 string to a vector of bytes.    
typedef unsigned char BYTE;
std::vector<unsigned char> base64_decode(std::string const &encoded_string);

bool load_gltf_asset(const std::string &filename, const std::string &filedir, GLTFAsset &asset);

}  // namespace gltf
