#pragma once

#include <lidl/lidl.hpp>

template<typename CapT, typename ConsT>
class frame {
public:
    const ::lidl::vector<CapT>& capabilities() const {
        return m_raw.capabilities;
    }
    ::lidl::vector<CapT>& capabilities() {
        return m_raw.capabilities;
    }
    const ::lidl::vector<ConsT>& constraints() const {
        return m_raw.constraints;
    }
    ::lidl::vector<ConsT>& constraints() {
        return m_raw.constraints;
    }

private:
    struct frame_raw {
        ::lidl::vector<CapT> capabilities;
        ::lidl::vector<ConsT> constraints;
    };

    frame_raw m_raw;
};
