/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief AceOfSpades VXL format
 *
 * https://silverspaceship.com/aosmap/
 *
 * @ingroup Formats
 */
class AoSVXLFormat : public RGBAFormat {
private:
	struct Header {
		uint8_t len = 0;
		uint8_t colorStartIdx = 0;
		uint8_t colorEndIdx = 0;
		uint8_t airStartIdx = 0;
	};
	static_assert(sizeof(Header) == 4, "Unexpected size of span header struct");
	static bool isSurface(const voxel::RawVolume *v, int x, int y, int z);
	glm::ivec3 dimensions(io::SeekableReadStream &stream) const;
	bool loadMap(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, int width, int height, int depths);
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
	bool loadGroups(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
