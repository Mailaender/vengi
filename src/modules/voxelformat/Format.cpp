/**
 * @file
 */

#include "Format.h"
#include "app/App.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "voxel/MaterialColor.h"
#include "VolumeFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include "math/Math.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VoxelUtil.h"
#include <limits>

namespace voxelformat {

static inline glm::vec4 transform(const glm::mat4x4 &mat, const glm::vec3 &pos, const glm::vec4 &pivot) {
	return glm::floor(mat * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - pivot));
}

voxel::RawVolume* Format::transformVolume(const SceneGraphTransform &t, const voxel::RawVolume *in) const {
	const glm::mat4 &mat = t.worldMatrix();
	const glm::vec3 &pivotNormalized = t.pivot();
	const voxel::Region &inRegion = in->region();
	const glm::ivec3 &inMins = inRegion.getLowerCorner();
	const glm::ivec3 &inMaxs = inRegion.getUpperCorner();
	const glm::vec4 pivot(glm::floor(pivotNormalized * glm::vec3(inRegion.getDimensionsInVoxels())), 0.0f);
	const glm::vec4 tmins(transform(mat, inMins, pivot));
	const glm::vec4 tmaxs(transform(mat, inMaxs, pivot));
	const glm::ivec3 rmins = glm::min(tmins, tmaxs);
	const glm::ivec3 rmaxs = glm::max(tmins, tmaxs);
	const voxel::Region outRegion(rmins, rmaxs);
	Log::debug("transform volume");
	Log::debug("* normalized pivot: %f:%f:%f", pivotNormalized.x, pivotNormalized.y, pivotNormalized.z);
	Log::debug("* pivot: %f:%f:%f", pivot.x, pivot.y, pivot.z);
	Log::debug("* mins: %i:%i:%i, maxs: %i:%i:%i", inMins.x, inMins.y, inMins.z, inMaxs.x, inMaxs.y, inMaxs.z);
	Log::debug("* transformed region: mins: %i:%i:%i, maxs: %i:%i:%i", rmins.x, rmins.y, rmins.z, rmaxs.x, rmaxs.y, rmaxs.z);
	voxel::RawVolume *v = new voxel::RawVolume(outRegion);
	voxel::RawVolume::Sampler inSampler(in);
	for (int z = inMins.z; z <= inMaxs.z; ++z) {
		for (int y = inMins.y; y <= inMaxs.y; ++y) {
			inSampler.setPosition(0, y, z);
			for (int x = inMins.x; x <= inMaxs.x; ++x) {
				const voxel::Voxel voxel = inSampler.voxel();
				inSampler.movePositiveX();
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				const glm::ivec3 pos(transform(mat, inSampler.position(), pivot));
				v->setVoxel(pos, voxel);
			}
		}
	}
	return v;
}

core::String Format::stringProperty(const SceneGraphNode* node, const core::String &name, const core::String &defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return node->property(name);
}

bool Format::boolProperty(const SceneGraphNode* node, const core::String &name, bool defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return core::string::toBool(node->property(name));
}

float Format::floatProperty(const SceneGraphNode* node, const core::String &name, float defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return core::string::toFloat(node->property(name));
}

void Format::splitVolumes(const SceneGraph& srcSceneGraph, SceneGraph& destSceneGraph, const glm::ivec3 &maxSize, bool crop) {
	destSceneGraph.reserve(srcSceneGraph.size());
	for (SceneGraphNode &node : srcSceneGraph) {
		const voxel::Region& region = node.region();
		if (!region.isValid()) {
			Log::debug("invalid region for node %i", node.id());
			continue;
		}
		if (glm::all(glm::lessThan(region.getDimensionsInVoxels(), maxSize))) {
			SceneGraphNode newNode;
			copyNode(node, newNode, true);
			destSceneGraph.emplace(core::move(newNode));
			continue;
		}
		core::DynamicArray<voxel::RawVolume *> rawVolumes;
		voxelutil::splitVolume(node.volume(), maxSize, rawVolumes);
		for (voxel::RawVolume *v : rawVolumes) {
			SceneGraphNode newNode;
			if (crop) {
				voxel::RawVolume *cv = voxelutil::cropVolume(v);
				delete v;
				v = cv;
			}
			copyNode(node, newNode, false);
			newNode.setVolume(v, true);
			destSceneGraph.emplace(core::move(newNode));
		}
	}
}

bool Format::isEmptyBlock(const voxel::RawVolume *v, const glm::ivec3 &maxSize, int x, int y, int z) const {
	const voxel::Region region(x, y, z, x + maxSize.x - 1, y + maxSize.y - 1, z + maxSize.z - 1);
	return voxelutil::isEmpty(*v, region);
}

void Format::calcMinsMaxs(const voxel::Region& region, const glm::ivec3 &maxSize, glm::ivec3 &mins, glm::ivec3 &maxs) const {
	const glm::ivec3 &lower = region.getLowerCorner();
	mins[0] = lower[0] & ~(maxSize.x - 1);
	mins[1] = lower[1] & ~(maxSize.y - 1);
	mins[2] = lower[2] & ~(maxSize.z - 1);

	const glm::ivec3 &upper = region.getUpperCorner();
	maxs[0] = (upper[0] & ~(maxSize.x - 1)) + maxSize.x - 1;
	maxs[1] = (upper[1] & ~(maxSize.y - 1)) + maxSize.y - 1;
	maxs[2] = (upper[2] & ~(maxSize.z - 1)) + maxSize.z - 1;

	Log::debug("%s", region.toString().c_str());
	Log::debug("mins(%i:%i:%i)", mins.x, mins.y, mins.z);
	Log::debug("maxs(%i:%i:%i)", maxs.x, maxs.y, maxs.z);
}

size_t Format::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	return 0;
}

size_t PaletteFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	SceneGraph sceneGraph;
	loadGroupsPalette(filename, stream, sceneGraph, palette);
	return palette.colorCount;
}

image::ImagePtr Format::loadScreenshot(const core::String &filename, io::SeekableReadStream &) {
	Log::debug("%s doesn't have a supported embedded screenshot", filename.c_str());
	return image::ImagePtr();
}

bool Format::save(const voxel::RawVolume* volume, const core::String &filename, io::SeekableWriteStream& stream) {
	if (volume == nullptr) {
		return false;
	}
	SceneGraph sceneGraph(2);
	SceneGraphNode node;
	node.setVolume(volume, false);
	sceneGraph.emplace(core::move(node));
	return saveGroups(sceneGraph, filename, stream);
}

bool Format::stopExecution() {
	return app::App::getInstance()->shouldQuit();
}

}
