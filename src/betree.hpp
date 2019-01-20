// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BETREE_HPP_
#define SRC_BETREE_HPP_

#include "imlab/betree.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------------------------------
IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::insert(const Key &key, const T &value) {
    // add_message<Message::Insert>(key, value);
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::insert(const Key &key, T &&value) {
    // add_message<Message::Insert>(key, std::forward(value));
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::insert_or_assign(const Key &key, const T &value) {
    // add_message<Message::InsertOrAssign>(key, value);
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::insert_or_assign(const Key &key, T &&value) {
    // add_message<Message::InsertOrAssign>(key);
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::erase(const Key &key) {
    // add_message<Message::Erase>(key);
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix IMLAB_BETREE_CLASS::root_fix_exclusive() {
    if (root)
        return this->manager.fix_exclusive(*root);

    root = next_page_id;
    return new_leaf();
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix IMLAB_BETREE_CLASS::new_leaf() {
    auto fix = this->manager.fix_exclusive(next_page_id++);
    new (fix.data()) LeafNode();
    ++count;

    return fix;
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix IMLAB_BETREE_CLASS::new_inner(const Node &child) {
    auto fix = this->manager.fix_exclusive(next_page_id++);
    new (fix.data()) InnerNode(child);

    return fix;
}
// ---------------------------------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BETREE_HPP_
