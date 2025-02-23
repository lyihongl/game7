#pragma once
#include <array>
#include <concepts>
#include <memory>
#include <raylib.h>
#include <type_traits>

#include "util.h"
#include <vector>
namespace yhl_util {

template<typename T>
concept has_pos = requires(T t) {
    { t.pos } -> std::same_as<Vector2&>;
};

template<has_pos T>
class quadtree
{
    /*
    a quad will have a smallest size
    a quad will have a capacity
    - if the capacity is reached, we will try to sub-divide the quad
    - to start off we have a quad that is the size of the screen
    insert nodes
    query area
    */

    /*
    0 - top left
    1 - top right
    2 - bottom right
    3 - bottom left
    */
    // struct quadrant_map{
    //     double x;
    //     double y;
    //     double w;
    //     double h;
    // };
    std::array<std::unique_ptr<quadtree<T>>, 4> quadrants;
    std::vector<typename std::vector<T>::iterator> elements;
    std::size_t capacity;
    double min_w{ 1920.f / 32 };
    double min_h{ 1080.f / 32 };

  public:
    double x;
    double y;
    double w;
    double h;
    static Camera2D* c;
    quadtree(double x, double y, double w, double h);
    void insert(std::vector<T>::iterator element, double x_, double y_);
    void query(double,
               double,
               double,
               double,
               std::vector<typename std::vector<T>::iterator>&,
               bool debug = false) const;
    void draw() const;
    void clear();
};
template<has_pos T>
quadtree<T>::quadtree(double x, double y, double w, double h)
  : x(x)
  , y(y)
  , w(w)
  , h(h)
  , capacity(50)
{
}

template<has_pos T>
void
quadtree<T>::query(double x_,
                   double y_,
                   double w_,
                   double h_,
                   std::vector<typename std::vector<T>::iterator>& res,
                   bool debug) const
{
    if (!yhl_util::check_collision(
          yhl_util::Rectangle{
            .x = x_,
            .y = y_,
            .width = w_,
            .height = h_,
          },
          yhl_util::Rectangle{
            .x = x,
            .y = y,
            .width = w,
            .height = h,
          })) {
        return;
    }
    if (debug) {
        DrawRectangleLinesEx(::Rectangle{ x, y, w, h }, 5, RED);
    }

    if (elements.size() > 0) {
        for (auto e : elements) {
            if (debug) {
                if (c) {
                    auto ep = GetWorldToScreen2D(e->pos, *c);
                    DrawLineV(ep, Vector2{ x, y }, RED);
                    std::string pos = "(" + std::to_string(ep.x) + ", " +
                                      std::to_string(ep.y) + ")";
                    DrawText(pos.c_str(), ep.x, ep.y, 10, RED);
                }
            }
            res.emplace_back(e);
        }
    }
    if (quadrants[0]) {
        for (auto& q : quadrants) {
            q->query(x_, y_, w_, h_, res, debug);
        }
    }
}

template<has_pos T>
void
quadtree<T>::insert(std::vector<T>::iterator element, double x_, double y_)
{
    if (elements.size() >= capacity && w > min_w && h > min_h &&
        !quadrants[0]) {
        // initialize the sub-trees and insert everything
        quadrants[0] = std::make_unique<quadtree<T>>(x, y, w / 2, h / 2);
        quadrants[1] =
          std::make_unique<quadtree<T>>(x + w / 2, y, w / 2, h / 2);
        quadrants[2] =
          std::make_unique<quadtree<T>>(x + w / 2, y + h / 2, w / 2, h / 2);
        quadrants[3] =
          std::make_unique<quadtree<T>>(x, y + h / 2, w / 2, h / 2);
        for (auto i : elements) {
            auto pos = i->pos;
            if (c) {
                pos = GetWorldToScreen2D(i->pos, *c);
            }
            if (pos.x <= x + w / 2) {
                if (pos.y <= y + h / 2) {
                    quadrants[0]->insert(i, pos.x, pos.y);
                } else {
                    quadrants[3]->insert(i, pos.x, pos.y);
                }
            } else {
                if (pos.y <= y + h / 2) {
                    quadrants[1]->insert(i, pos.x, pos.y);
                } else {
                    quadrants[2]->insert(i, pos.x, pos.y);
                }
            }
        }
        elements.clear();
        return;
    }
    if (!quadrants[0]) {
        elements.emplace_back(element);
    } else {
        if (x_ <= x + w / 2) {
            if (y_ <= y + h / 2) {
                quadrants[0]->insert(element, x_, y_);
            } else {
                quadrants[3]->insert(element, x_, y_);
            }
        } else {
            if (y_ <= y + h / 2) {
                quadrants[1]->insert(element, x_, y_);
            } else {
                quadrants[2]->insert(element, x_, y_);
            }
        }
    }
}

template<has_pos T>
void
quadtree<T>::draw() const
{
    if (!quadrants[0]) {
        DrawRectangleLines(x, y, w, h, WHITE);
    } else {
        for (auto const& q : quadrants) {
            q->draw();
        }
    }
}
template<has_pos T>
void
quadtree<T>::clear()
{
    for (auto& q : quadrants) {
        q.release();
    }
    elements.clear();
}

};