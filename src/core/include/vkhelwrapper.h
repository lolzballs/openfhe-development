#ifndef SRC_CORE_INCLUDE_VKHELWRAPPER_H_
#define SRC_CORE_INCLUDE_VKHELWRAPPER_H_

#ifdef WITH_VKHEL

    #include <memory>

extern "C" {
    #include <vkhel.h>
}

namespace lbcrypto {
class VkhelCtxManager {
private:
    VkhelCtxManager() : _ctx(vkhel_ctx_create(), vkhel_ctx_destroy){};

    std::shared_ptr<vkhel_ctx> _ctx;

public:
    static std::shared_ptr<vkhel_ctx> getContext() {
        static VkhelCtxManager ctx;
        return ctx._ctx;
    }
};

using VkhelNTTTablesWrapper = std::shared_ptr<vkhel_ntt_tables>;
};  // namespace lbcrypto

#endif

#endif
