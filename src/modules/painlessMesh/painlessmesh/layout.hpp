#ifndef _PAINLESS_MESH_LAYOUT_HPP_
#define _PAINLESS_MESH_LAYOUT_HPP_

#include <list>
#include <memory>

// #include "modules/painlessmesh/painlessmesh/protocol.hpp"
#include "protocol.hpp"

namespace painlessmesh {
namespace layout {

#include <memory>
/*
struct virtual_enable_shared_from_this_base
    : std::enable_shared_from_this<virtual_enable_shared_from_this_base> {
  virtual ~virtual_enable_shared_from_this_base() {}
};

template <typename T>
struct virtual_enable_shared_from_this
    : virtual virtual_enable_shared_from_this_base {
  std::shared_ptr<T> shared_from_this() {
    return std::dynamic_pointer_cast<T>(
        virtual_enable_shared_from_this_base::shared_from_this());
  }
};*/

/*
struct A: virtual_enable_shared_from_this<A> {};
struct B: virtual_enable_shared_from_this<B> {};
struct Z: A, B { };
int main() {
 std::shared_ptr<Z> z = std::make_shared<Z>();
 std::shared_ptr<B> b = z->B::shared_from_this();
}
*/

/**
 * Whether the tree contains the given nodeId
 */
inline bool contains(protocol::NodeTree nodeTree, uint32_t nodeId) {
  if (nodeTree.nodeId == nodeId) {
    return true;
  }
  for (auto&& s : nodeTree.subs) {
    if (contains(s, nodeId)) return true;
  }
  return false;
}

inline protocol::NodeTree excludeRoute(protocol::NodeTree&& tree,
                                       uint32_t exclude) {
  // Make sure to exclude any subs with nodeId == 0,
  // even if exlude is not set to zero
  tree.subs.remove_if([exclude](protocol::NodeTree s) {
    return s.nodeId == 0 || s.nodeId == exclude;
  });
  return tree;
}

template <class T>
class Layout {
 public:
  std::list<std::shared_ptr<T> > subs;
  uint32_t nodeId;
  bool root;

  protocol::NodeTree asNodeTree() {
    auto nt = protocol::NodeTree(nodeId, root);
    for (auto&& s : subs) {
      nt.subs.push_back(protocol::NodeTree(*s));
    }
    return nt;
  }
};

class Neighbour : public protocol::NodeTree {
 public:
  // Inherit constructors
  using protocol::NodeTree::NodeTree;

  /**
   * Is the passed nodesync valid
   *
   * If not then the caller of this function should probably disconnect
   * this neighbour.
   */
  bool validSubs(protocol::NodeTree tree) {
    if (nodeId == 0)  // Cant really know, so valid as far as we know
      return true;
    if (nodeId != tree.nodeId) return false;
    for (auto&& s : tree.subs) {
      if (layout::contains(s, nodeId)) return false;
    }
    return true;
  }

  /**
   * Update subs with the new subs
   *
   * \param tree The possible new tree with this node as base
   *
   * Generally one probably wants to call validSubs before calling this
   * function.
   *
   * \return Whether we adopted the new tree
   */
  bool updateSubs(protocol::NodeTree tree) {
    if (nodeId == 0 || tree != (*this)) {
      nodeId = tree.nodeId;
      subs = tree.subs;
      root = tree.root;
      return true;
    }
    return false;
  }

  /**
   * Create a request
   */
  protocol::NodeSyncRequest request(NodeTree&& layout) {
    auto subTree = excludeRoute(std::move(layout), nodeId);
    return protocol::NodeSyncRequest(layout.nodeId, nodeId, subTree.subs, root);
  }

  /**
   * Create a reply
   */
  protocol::NodeSyncReply reply(NodeTree&& layout) {
    auto subTree = excludeRoute(std::move(layout), nodeId);
    return protocol::NodeSyncReply(layout.nodeId, nodeId, subTree.subs, root);
  }
};

template <class T>
std::shared_ptr<T> findRoute(Layout<T> tree,
                             std::function<bool(std::shared_ptr<T>)> func) {
  auto route = std::find_if(tree.subs.begin(), tree.subs.end(), func);
  if (route == tree.subs.end()) return NULL;
  return (*route);
}

template <class T>
std::shared_ptr<T> findRoute(Layout<T> tree, uint32_t nodeId) {
  return findRoute<T>(tree, [nodeId](std::shared_ptr<T> s) {
    return layout::contains((*s), nodeId);
  });
}

/**
 * The size of the mesh (the number of nodes)
 */
inline uint32_t size(protocol::NodeTree nodeTree) {
  auto no = 1;
  for (auto&& s : nodeTree.subs) {
    no += size(s);
  }
  return no;
}

/**
 * Whether the top node in the tree is also the root of the mesh
 */
inline bool isRoot(protocol::NodeTree nodeTree) {
  if (nodeTree.root) return true;
  return false;
}

/**
 * Whether any node in the tree is also root of the mesh
 */
inline bool isRooted(protocol::NodeTree nodeTree) {
  if (isRoot(nodeTree)) return true;
  for (auto&& s : nodeTree.subs) {
    if (isRooted(s)) return true;
  }
  return false;
}

/**
 * Return all nodes in a list container
 */
inline std::list<uint32_t> asList(protocol::NodeTree nodeTree,
                                  bool includeSelf = true) {
  std::list<uint32_t> lst;
  if (includeSelf) lst.push_back(nodeTree.nodeId);
  for (auto&& s : nodeTree.subs) {
    lst.splice(lst.end(), asList(s));
  }
  return lst;
}

}  // namespace layout
}  // namespace painlessmesh

#endif

