/**
 * @file
 */

#pragma once

#include "voxelformat/SceneGraph.h"

namespace voxelformat {

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume);

// this makes a copy of the volumes affected
int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent);

// this doesn't copy but transfer the volume ownership
int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent);

int addSceneGraphNodes(SceneGraph& target, SceneGraph& source, int parent);

} // namespace voxel
