#pragma once


#include <lde/cellfy/forest/forest.h>


namespace lde::cellfy::boox {


using forest_t = lde::forest::forest<>;

using node_key_type = typename forest_t::key_type;

template<typename T>
using forest_iterator = typename forest_t::iterator<T>;


} // namespace lde::cellfy::boox
