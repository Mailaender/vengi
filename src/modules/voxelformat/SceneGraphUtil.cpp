/**
 * @file
 */

#include "SceneGraphUtil.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"

namespace voxelformat {

static int addToGraph(SceneGraph &sceneGraph, SceneGraphNode &&node, int parent) {
	int newNodeId = sceneGraph.emplace(core::move(node), parent);
	if (newNodeId == -1) {
		Log::error("Failed to add node to the scene");
		return -1;
	}
	return newNodeId;
}

static void copy(const SceneGraphNode &node, SceneGraphNode &target) {
	target.setName(node.name());
	target.setKeyFrames(node.keyFrames());
	target.setVisible(node.visible());
	target.setLocked(node.locked());
	target.addProperties(node.properties());
	target.setPalette(node.palette());
	if (node.type() == SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
	} else {
		core_assert(node.volume() == nullptr);
	}
}

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume) {
	// TODO: also add all children
	if (copyVolume) {
		target.setVolume(new voxel::RawVolume(src.volume()), true);
	} else {
		target.setVolume(src.volume(), false);
	}
	copy(src, target);
}

int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(node.volume()), true);
	}
	return addToGraph(sceneGraph, core::move(newNode), parent);
}

int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		core_assert(node.owns());
		newNode.setVolume(node.volume(), true);
		node.releaseOwnership();
	}
	return addToGraph(sceneGraph, core::move(newNode), parent);
}

static int addSceneGraphNode_r(SceneGraph &target, const SceneGraph &source, SceneGraphNode &sourceNode, int parent) {
	const int newNodeId = addNodeToSceneGraph(target, sourceNode, parent);
	if (newNodeId == -1) {
		Log::error("Failed to add node to the scene graph");
		return 0;
	}

	int nodesAdded = sourceNode.type() == SceneGraphNodeType::Model ? 1 : 0;
	for (int sourceNodeIdx : sourceNode.children()) {
		core_assert(source.hasNode(sourceNodeIdx));
		SceneGraphNode &sourceChildNode = source.node(sourceNodeIdx);
		nodesAdded += addSceneGraphNode_r(target, source, sourceChildNode, newNodeId);
	}

	return nodesAdded;
}

int addSceneGraphNodes(SceneGraph &target, SceneGraph &source, int parent) {
	const SceneGraphNode &sourceRoot = source.root();
	int nodesAdded = 0;
	target.node(parent).addProperties(sourceRoot.properties());
	for (int sourceNodeId : sourceRoot.children()) {
		nodesAdded += addSceneGraphNode_r(target, source, source.node(sourceNodeId), parent);
	}
	return nodesAdded;
}

} // namespace voxel
