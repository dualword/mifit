#ifdef MI_USE_TREE

#include "TreeData.h"

std::set<TreeData*> treeDataRegistry;

bool validTreeData(TreeData* data) {
  if (data == NULL) {
    return false;
  }
  return treeDataRegistry.find(data) != treeDataRegistry.end();
}

void invalidateTreeData(TreeData* data) {
  treeDataRegistry.erase(data);
}

#endif // MI_USE_TREE
