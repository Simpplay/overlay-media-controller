#pragma once

#include <vector>

struct HitRegion {
    int x, y, w, h;
    bool interactive;

    bool contains(int px, int py) const {
        return px >= x && px <= x + w &&
            py >= y && py <= y + h;
    }
};

namespace omc::infra {
    class HitTestManager {
    public:
        void setRegions(std::vector<HitRegion> r) {
            regions = std::move(r);
        }

        bool isInteractive(int x, int y) const {
            for (const auto& r : regions) {
                if (r.contains(x, y))
                    return r.interactive;
            }
            return false;
        }

    private:
        std::vector<HitRegion> regions;
    };
}