/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief Qubicle Binary Tree (qbt) is the successor of the widespread voxel exchange format Qubicle Binary.
 *
 * @see QBCLFormat
 * @see QBFormat
 * @see QEFFormat
 *
 * https://getqubicle.com/qubicle/documentation/docs/file/qbt/
 *
 * @ingroup Formats
 */
class QBTFormat : public PaletteFormat {
private:
	bool skipNode(io::SeekableReadStream& stream);
	bool loadMatrix(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette);
	bool loadCompound(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette);
	bool loadModel(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette);
	bool loadNode(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;

	bool loadColorMap(io::SeekableReadStream& stream, voxel::Palette &palette);
	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node, bool colorMap) const;
	bool saveColorMap(io::SeekableWriteStream& stream, const voxel::Palette& palette) const;
	bool saveModel(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph, bool colorMap) const;
public:
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
