/**
 * @file
 */

#pragma once

#include "SceneGraph.h"
#include "core/collection/Array.h"
#include "io/File.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "video/Texture.h"

namespace voxel {
class Palette;
}

namespace voxelformat {

#define VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED (1 << 0)
#define VOX_FORMAT_FLAG_PALETTE_EMBEDDED (1 << 1)
#define VOX_FORMAT_FLAG_MESH (1 << 2)

extern const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[];
extern const io::FormatDescription* voxelLoad();
extern const io::FormatDescription* voxelSave();

/**
 * @brief Tries to load a palette from the given file. This can either be an image which is reduced to 256 colors or a
 * volume format with an embedded palette
 */
extern bool importPalette(const core::String &filename, voxel::Palette &palette);
/**
 * @brief Tries to load the embedded palette from the given file. If the format doesn't have a palette embedded, this returns @c 0
 */
extern size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette);
extern image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream);
extern bool loadFormat(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph);

/**
 * @brief Save both to volume or to mesh - depends on the given file extension
 */
extern bool saveFormat(const io::FilePtr &filePtr, SceneGraph &sceneGraph);

extern bool isMeshFormat(const core::String &filename);
extern bool isModelFormat(const core::String &filename);

} // namespace voxelformat
