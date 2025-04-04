#pragma once
#include "Entity.hpp"
#include <vector>
#include <cmath>
#include <memory>

namespace Physics
{
    class Collision {
    public:
        static Vec2<float> GetOverlap(const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b) {
            auto aBB = a->get<CBoundingBox>();  
            auto aPos = a->get<CTransform>().pos;
            auto bBB = b->get<CBoundingBox>();
            auto bPos = b->get<CTransform>().pos;

            sf::FloatRect aRect = aBB.getRect(aPos);
            sf::FloatRect bRect = bBB.getRect(bPos);

            float overlapX = std::max(0.0f, 
                std::min(aRect.left + aRect.width, bRect.left + bRect.width) - 
                std::max(aRect.left, bRect.left));

            float overlapY = std::max(0.0f, 
                std::min(aRect.top + aRect.height, bRect.top + bRect.height) - 
                std::max(aRect.top, bRect.top));

            return { overlapX, overlapY };
        }

        static bool LineIntersects(Vec2<float> a1, Vec2<float> a2, Vec2<float> b1, Vec2<float> b2) {
            auto cross = [](Vec2<float> v1, Vec2<float> v2) -> float {
                return v1.x * v2.y - v1.y * v2.x;
            };

            Vec2<float> r = {a2.x - a1.x, a2.y - a1.y};
            Vec2<float> s = {b2.x - b1.x, b2.y - b1.y};

            float rxs = cross(r, s);
            float qpxr = cross({b1.x - a1.x, b1.y - a1.y}, r);

            if (std::fabs(rxs) < 1e-6) { 
                if (std::fabs(qpxr) < 1e-6) { 
                    return (std::min(a1.x, a2.x) <= std::max(b1.x, b2.x) &&
                            std::max(a1.x, a2.x) >= std::min(b1.x, b2.x) &&
                            std::min(a1.y, a2.y) <= std::max(b1.y, b2.y) &&
                            std::max(a1.y, a2.y) >= std::min(b1.y, b2.y));
                }
                return false;
            }

            float t = cross({b1.x - a1.x, b1.y - a1.y}, s) / rxs;
            float u = cross({b1.x - a1.x, b1.y - a1.y}, r) / rxs;

            return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
        }

        static bool IsPathBlocked(Vec2<float> start, Vec2<float> end, const std::vector<std::shared_ptr<Entity>>& tiles) {
            for (auto& tile : tiles) {
                if (!tile->has<CBoundingBox>() || !tile->has<CTransform>()) continue;

                auto& tileBB = tile->get<CBoundingBox>();
                auto& tileTrans = tile->get<CTransform>();
                sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

                Vec2<float> tl(tileRect.left, tileRect.top);
                Vec2<float> tr(tileRect.left + tileRect.width, tileRect.top);
                Vec2<float> bl(tileRect.left, tileRect.top + tileRect.height);
                Vec2<float> br(tileRect.left + tileRect.width, tileRect.top + tileRect.height);

                if (LineIntersects(start, end, tl, tr) ||
                    LineIntersects(start, end, tr, br) ||
                    LineIntersects(start, end, br, bl) ||
                    LineIntersects(start, end, bl, tl)) {
                    return true;
                }
            }
            return false;
        }
    };

    class Forces {
    public:
        static void ApplyKnockback(std::shared_ptr<Entity> entity, Vec2<float> direction, float strength) {
            if (!entity->has<CTransform>() || !entity->has<CState>()) return;
            auto& transform = entity->get<CTransform>();
            // Imposta una velocità orizzontale molto elevata basata sulla direzione e sulla forza.
            transform.velocity.x = direction.x * strength;
        }
    };
}
