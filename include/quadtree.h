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
    double min_w;
    double min_h;

  public:
    double x;
    double y;
    double w;
    double h;
    quadtree(double x, double y, double w, double h);
    void insert(std::vector<T>::iterator element, double x_, double y_);
    void query(double,
               double,
               double,
               double,
               std::vector<typename std::vector<T>::iterator>&) const;
    void draw() const;
    void clear();
};
template<has_pos T>
quadtree<T>::quadtree(double x, double y, double w, double h)
  : x(x)
  , y(y)
  , w(w)
  , h(h)
  , capacity(25)
  , min_h(h / (2 << 5))
  , min_w(w / (2 << 5))
{
}

template<has_pos T>
void
quadtree<T>::query(double x_,
                   double y_,
                   double w_,
                   double h_,
                   std::vector<typename std::vector<T>::iterator>& res) const
{
    if (!CheckCollisionRecs(
          Rectangle{
            .x = float(x_),
            .y = float(y_),
            .width = float(w_),
            .height = float(h_),
          },
          Rectangle{
            .x = float(x),
            .y = float(y),
            .width = float(w),
            .height = float(h),
          })) {
        return;
    }
    // DrawRectangleLinesEx(Rectangle{ x, y, w, h }, 10, RED);

    if (elements.size() != 0) {
        for (auto& e : elements) {
            res.emplace_back(e);
        }
    }
    if (quadrants[0]) {
        for (auto& q : quadrants) {
            q->query(x_, y_, w_, h_, res);
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
        for (auto& i : elements) {
            if (i->pos.x <= x + w / 2) {
                if (i->pos.y <= y + h / 2) {
                    quadrants[0]->insert(i, i->pos.x, i->pos.y);
                } else {
                    quadrants[3]->insert(i, i->pos.x, i->pos.y);
                }
            } else {
                if (i->pos.y <= y + h / 2) {
                    quadrants[1]->insert(i, i->pos.x, i->pos.y);
                } else {
                    quadrants[2]->insert(i, i->pos.x, i->pos.y);
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