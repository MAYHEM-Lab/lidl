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

struct sha2_signature {
    ::lidl::array<u8, 32> sign;
};

template<typename CapT, typename ConsT>
class token {
public:
    const ::lidl::vector<frame<CapT, ConsT>>& frames() const {
        return m_raw.frames;
    }
    ::lidl::vector<frame<CapT, ConsT>>& frames() {
        return m_raw.frames;
    }
    const sha2_signature& signature() const {
        return m_raw.signature;
    }
    sha2_signature& signature() {
        return m_raw.signature;
    }

private:
    struct token_raw {
        ::lidl::vector<frame<CapT, ConsT>> frames;
        sha2_signature signature;
    };

    token_raw m_raw;
};
