/**
 * @file
 */

#include "VXTFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/StringSet.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/ZipReadStream.h"
#include "io/Filesystem.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vxt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vxt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXTFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &in, SceneGraph &sceneGraph, voxel::Palette &) {
	io::ZipReadStream stream(in, (int)in.size());
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'T') {
		Log::error("Could not load vxt file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version = magic[3] - '0';
	if (version > 2) {
		Log::error("Could not load vxt file: Unsupported version found (%i)", version);
		return false;
	}

	int32_t width;
	wrap(stream.readInt32(width))
	int32_t height;
	wrap(stream.readInt32(height))
	int32_t depth;
	wrap(stream.readInt32(depth))
	int32_t models;
	wrap(stream.readInt32(models))
	SceneGraph tileGraph;

	for (int32_t i = 0; i < models; ++i) {
		char path[1024];
		wrapBool(stream.readString(sizeof(path), path, true))
		const io::FilePtr& file = io::filesystem()->open(path);
		if (!file->validHandle()) {
			Log::warn("Could not load file %s", path);
			continue;
		}
		io::FileStream childStream(file);
		VXMFormat f;
		SceneGraph subGraph;
		if (!f.loadGroups(path, childStream, subGraph)) {
			Log::warn("Failed to load vxm tile %s", path);
			continue;
		}
		for (const SceneGraphNode &node : subGraph) {
			SceneGraphNode newNode(SceneGraphNodeType::Model);
			copyNode(node, newNode, true);
			newNode.setProperty("tileidx", core::string::format("%i", i));
			newNode.setName(path);
			tileGraph.emplace(core::move(newNode));
		}
	}

	int idx = 0;
	for (;;) {
		int32_t rle;
		wrap(stream.readInt32(rle))
		if (rle == 0) {
			break;
		}

		int32_t modelIdx = 0;
		wrap(stream.readInt32(modelIdx))
		uint8_t orientation = 0;
		if (version >= 2) {
			wrap(stream.readUInt8(orientation))
		}
		if (modelIdx == -1) {
			idx += rle;
			continue;
		}

		const SceneGraphNode* node = tileGraph[modelIdx];
		if (node == nullptr) {
			Log::warn("Failed to get model from scene graph with index %i", modelIdx);
			continue;
		}
		const int tileSize = 64; // TODO: positioning unknown
		if (version >= 2) {
			//const uint32_t forward = (orientation & 0xF0) >> 4;
			//const uint32_t up = orientation & 0xF;
		}
		for (int32_t i = idx; i < idx + rle; ++i) {
			const int32_t x = i / (height * depth);
			const int32_t y = (i / depth) % height;
			const int32_t z = i % depth;
			SceneGraphNode tileNode(SceneGraphNodeType::Model);
			copyNode(*node, tileNode, true);
			const glm::ivec3 pos(x * tileSize, y * tileSize, z * tileSize);
			tileNode.volume()->translate(pos); // TODO
			//tileNode.transform().position = pos;
			tileGraph.emplace(core::move(tileNode));
		}
		idx += rle;
	}
	return true;
}

bool VXTFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream) {
	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxel
