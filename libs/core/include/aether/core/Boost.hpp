#pragma once
#include <boost/container/static_vector.hpp>

namespace aether::core {
    // TODO: In Release, disable exceptions:
    // using StaticVecOptions = boost::container::static_vector_options_t<boost::container::throw_on_overflow<false>>;

    template <class T, size_t Max>
    using StaticVector = boost::container::static_vector<T, Max>;
}