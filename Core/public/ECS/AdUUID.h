#ifndef ADUUID_H
#define ADUUID_H

#include "AdEngine.h"

namespace WuDu {
        class AdUUID {
        public:
                AdUUID();
                AdUUID(uint32_t uuid);
                AdUUID(const AdUUID&) = default;
                operator uint32_t() const { return mUUID; }

                uint64_t mUUID;
        };
}

namespace std {
        template<>
        struct hash<WuDu::AdUUID> {
                std::size_t operator()(const WuDu::AdUUID& uuid) const {
                        if (!uuid) {
                                return 0;
                        }
                        return (uint32_t)uuid;
                }
        };
}

#endif