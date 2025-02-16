/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/AoSVXLFormat.h"

namespace voxelformat {

class AoSVXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(AoSVXLFormatTest, testLoad) {
	canLoad("aceofspades.vxl");
}

TEST_F(AoSVXLFormatTest, testLoadPalette) {
	AoSVXLFormat f;
	voxel::Palette pal;
	EXPECT_GT(loadPalette("aceofspades.vxl", f, pal), 200);
}

TEST_F(AoSVXLFormatTest, DISABLED_testSave) {
	AoSVXLFormat f;
	voxel::Region region(glm::ivec3(0), glm::ivec3(255, 64, 255));
	voxel::RawVolume layer1(region);
	const char *filename = "tests-aos.vxl";
	for (int x = 0; x < region.getWidthInVoxels(); ++x) {
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			EXPECT_TRUE(layer1.setVoxel(x, 0, z, createVoxel(voxel::VoxelType::Generic, 1)));
			EXPECT_TRUE(layer1.setVoxel(x, 1, z, createVoxel(voxel::VoxelType::Generic, 1)));
		}
	}
	SceneGraph sceneGraph;
	SceneGraphNode node1;
	node1.setVolume(&layer1, false);
	sceneGraph.emplace(core::move(node1));
	const io::FilePtr &sfile = open(filename, io::FileMode::SysWrite);
	io::FileStream sstream(sfile);
	ASSERT_TRUE(f.saveGroups(sceneGraph, sfile->name(), sstream));
	SceneGraph sceneGraphLoad;
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	EXPECT_TRUE(f.loadGroups(file->name(), stream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

}
