// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BETREE_HPP_
#define SRC_BETREE_HPP_

#include "imlab/betree.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------------------------------
IMLAB_BETREE_TEMPL typename BufferManager<page_size>::Fix IMLAB_BETREE_CLASS::root_fix() {
    if (root)
        return this->manager.fix(root);
    return {};
}

IMLAB_BETREE_TEMPL typename BufferManager<page_size>::ExclusiveFix IMLAB_BETREE_CLASS::root_fix_exclusive() {
    if (root)
        return this->manager.fix_exclusive(*root);

    root = next_page_id;
    return new_leaf();
}

IMLAB_BETREE_TEMPL typename BufferManager<page_size>::ExclusiveFix IMLAB_BETREE_CLASS::new_leaf() {
    auto fix = this->manager.fix_exclusive(next_page_id++);
    new (fix.data()) LeafNode();
    ++count;

    return fix;
}

IMLAB_BETREE_TEMPL typename BufferManager<page_size>::ExclusiveFix IMLAB_BETREE_CLASS::new_inner(const Node &child) {
    auto fix = this->manager.fix_exclusive(next_page_id++);
    new (fix.data()) InnerNode(child);

    return fix;
}
// ---------------------------------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BETREE_HPP_
