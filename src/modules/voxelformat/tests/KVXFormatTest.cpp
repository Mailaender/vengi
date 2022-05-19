/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KVXFormat.h"

namespace voxelformat {

class KVXFormatTest: public AbstractVoxFormatTest {
};

TEST_F(KVXFormatTest, testLoad) {
	canLoad("test.kvx");
}

}
