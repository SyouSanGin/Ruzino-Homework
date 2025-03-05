#include <gtest/gtest.h>

#include "MCore/Graph.h"
#include "MCore/MaterialXNodeTree.hpp"

using namespace USTC_CG;

TEST(MaterialXNodeTree, Constructor)
{
    MaterialXNodeTreeDescriptor descriptor;
    MaterialXNodeTree tree(
        "test.mtlx", std::make_shared<MaterialXNodeTreeDescriptor>());
}