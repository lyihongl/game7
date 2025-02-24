#include "quadtree.h"
#include "util.h"
#include <Eigen/Dense>
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/Rotation2D.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <ratio>
#include <raylib.h>
#include <raymath.h>
#include <vector>

template<typename T>
concept has_center = requires(const T& t) {
    { t.get_center()->Vector2 };
};

struct ship;
struct mounting_point;
struct turret;
class drone_manager;
struct drone;
struct bullet;

Rectangle
place_center_at(Rectangle& r, Vector2& c)
{
    r.x = c.x - r.width / 2;
    r.y = c.y - r.height / 2;
    return r;
}

Rectangle
place_center_at(Rectangle& r, Vector2&& c)
{
    r.x = c.x - r.width / 2;
    r.y = c.y - r.height / 2;
    return r;
}

struct drone
{
    Vector2 pos;
    Vector2 vel{ 0, 0 };
    float mass{ 1.f };
    int health;
};

class drone_manager
{
  public:
    drone_manager(int n, std::mt19937& gen)
      : green(n)
      , red(3)
      , yellow(n)
      , player(1)
      , qtree_green(-1000, -1000, 4920, 4080)
      , qtree_red(-1000, -1000, 4920, 4080)
      , qtree_yellow(-1000, -1000, 4920, 4080)
    {
        auto xd = std::uniform_int_distribution<>{ 0, 1920 };
        auto yd = std::uniform_int_distribution<>{ 0, 1920 };
        for (auto& g : green) {
            g.pos = { float(xd(gen)), float(yd(gen)) };
            g.health = 1;
        }
        for (auto& g : red) {
            g.pos = { float(xd(gen)), float(yd(gen)) };
            g.mass = 150.f;
            g.health = 100;
        }
        for (auto& g : yellow) {
            g.pos = { float(xd(gen)), float(yd(gen)) };
            g.health = 1;
        }
        player[0].pos = { 1920.f / 2, 1080.f / 2 };
    }
    void tick(Vector2 const&, const Camera2D&);
    void render() const;
    void rule(std::vector<drone>& a,
              std::vector<drone>& b,
              float f,
              float effective_dist);
    void rule(std::vector<drone>& a,
              yhl_util::quadtree<drone>& b,
              float f,
              float effective_dist);
    void player_rule(std::vector<drone>& a,
                     const Vector2& player_pos,
                     float f,
                     float effective_dist);

    yhl_util::quadtree<drone> qtree_green;
    yhl_util::quadtree<drone> qtree_yellow;
    yhl_util::quadtree<drone> qtree_red;

  private:
    std::vector<drone> green;
    std::vector<drone> red;
    std::vector<drone> yellow;
    std::vector<drone> player;
};

void
drone_manager::rule(std::vector<drone>& a,
                    std::vector<drone>& b,
                    float f,
                    float effective_dist)
{
    for (auto& pa : a) {
        Vector2 tf{ 0, 0 };

        for (auto& pb : b) {
            float dist = Vector2Distance(pa.pos, pb.pos);
            if (dist > 0 && dist < effective_dist) {
                float F = pb.mass * 0.5 * f / dist;
                tf.x += F * (pa.pos.x - pb.pos.x);
                tf.y += F * (pa.pos.y - pb.pos.y);
            }
        }
        if (tf.x != 0 && tf.y != 0) {
            pa.vel = Vector2Scale(pa.vel + tf, 0.5);
            pa.vel = Vector2ClampValue(pa.vel, 1.f, 10.f);
            pa.pos = pa.pos + pa.vel;
        }
    }
}
void
drone_manager::rule(std::vector<drone>& a,
                    yhl_util::quadtree<drone>& b,
                    float f,
                    float effective_dist)
{
    for (auto& pa : a) {
        Vector2 tf{ 0, 0 };
        std::vector<typename std::vector<drone>::iterator> res;
        auto pos = GetWorldToScreen2D(pa.pos, *b.c);
        b.query(pos.x - effective_dist,
                pos.y - effective_dist,
                effective_dist * 2,
                effective_dist * 2,
                res);
        for (auto pb : res) {
            float dist = Vector2Distance(pa.pos, pb->pos);
            if (dist > 0 && dist < effective_dist) {
                float F = pb->mass * 0.5 * f / dist;
                tf.x += F * (pa.pos.x - pb->pos.x);
                tf.y += F * (pa.pos.y - pb->pos.y);
            }
        }

        if (tf.x != 0 && tf.y != 0) {
            pa.vel = Vector2Scale(pa.vel + tf, 0.5);
            pa.vel = Vector2ClampValue(pa.vel, 1.f, 10.f);
            pa.pos = pa.pos + pa.vel;
        }
    }
}
void
drone_manager::player_rule(std::vector<drone>& a,
                           const Vector2& player_pos,
                           float f,
                           float effective_dist)
{
    for (auto& pa : a) {
        Vector2 tf{ 0, 0 };
        float dist = Vector2Distance(pa.pos, player_pos);
        float F = 0.5 * f / dist;
        if (dist > 1000) {
            F *= dist / 1000;
        }
        if (dist > 100) {
            tf.x += F * (pa.pos.x - player_pos.x);
            tf.y += F * (pa.pos.y - player_pos.y);
        } else if (dist <= 100) {
            tf.x -= 2 * F * (pa.pos.x - player_pos.x);
            tf.y -= 2 * F * (pa.pos.y - player_pos.y);
        }
        pa.vel = Vector2Scale(pa.vel + tf, 0.5);
        // pa.vel = Vector2ClampValue(pa.vel, 1.f, 50.f);
        pa.pos = pa.pos + pa.vel;
    }
}

void
drone_manager::tick(Vector2 const& player_pos, const Camera2D& c)
{
    qtree_green.clear();
    qtree_yellow.clear();
    qtree_red.clear();
    auto remove_green = std::remove_if(
      green.begin(), green.end(), [](auto const& g) { return g.health <= 0; });
    green.erase(remove_green, green.end());

    for (auto it = green.begin(); it != green.end(); it++) {
        auto [px, py] = GetWorldToScreen2D(it->pos, c);
        qtree_green.insert(it, px, py);
    }
    for (auto it = yellow.begin(); it != yellow.end(); it++) {
        auto [px, py] = GetWorldToScreen2D(it->pos, c);
        qtree_yellow.insert(it, px, py);
    }
    // for (auto it = red.begin(); it != red.end(); it++) {
    //     auto [px, py] = GetWorldToScreen2D(it->pos, c);
    //     qtree_red.insert(it, px, py);
    // }

    rule(green, qtree_green, -0.32, 200);
    rule(green, qtree_green, 0.3, 70);
    rule(green, red, 0.8, 50);
    rule(green, red, -0.17, 200);
    // rule(green, red, 0.5, 10);
    rule(green, qtree_yellow, 0.34, 200);
    rule(red, qtree_green, -0.34, 200);
    rule(red, red, 0.1, 400);
    rule(red, qtree_yellow, 0.3, 100);
    // rule(red, red, 0.8, 50);
    rule(yellow, qtree_yellow, 0.15, 60);
    rule(yellow, qtree_green, -0.2, 200);

    // rule(green, green, -0.32, 200);
    // rule(green, green, 0.3, 70);
    // rule(green, red, 0.8, 50);
    // rule(green, red, -0.17, 200);
    // // rule(green, red, 0.5, 10);
    // rule(green, yellow, 0.34, 200);
    // rule(red, green, -0.34, 200);
    // rule(red, red, 0.1, 400);
    // rule(red, yellow, 0.3, 100);
    // // rule(red, red, 0.8, 50);
    // rule(yellow, yellow, 0.15, 60);
    // rule(yellow, green, -0.2, 200);
    player_rule(yellow, player_pos, -0.2, 500);
    player_rule(green, player_pos, -1.4, 2000);
    player_rule(red, player_pos, -1.4, 2000);
}

void
drone_manager::render() const
{
    for (auto const& g : green) {
        DrawCircle(g.pos.x, g.pos.y, 2, GREEN);
    }
    for (auto const& g : red) {
        DrawCircle(g.pos.x, g.pos.y, 10, RED);
    }
    for (auto const& g : yellow) {
        DrawCircle(g.pos.x, g.pos.y, 2, YELLOW);
    }
}

/*
    the turrent will attach to a mounting point
*/
struct turret
{
    std::optional<std::vector<mounting_point>::iterator> attachment_point;
    turret()
      : attachment_point(std::nullopt)
      , gun_angle(0)
    {
    }
    float gun_angle;
    void tick();
};

struct mounting_point
{
    Eigen::Vector2d offset;
    const ship& parent;
    Vector2 get_center() const;
    Rectangle get_bounding_box() const;
    std::optional<turret> attachment;
};

struct ship_param
{
    Eigen::Rotation2Dd angle{ M_PI / 4 };
    float x{ 0.f };
    float y{ 0.f };
};

struct ship
{
    Eigen::Rotation2Dd angle;
    float x;
    float y;
    ship(const ship_param& p)
      : angle(p.angle)
      , x(p.x)
      , y(p.y)
    {
    }
    std::vector<mounting_point> mounting_points;
    Vector2 get_center() const;
    friend ship_param;
};

void
draw_turret(const turret& t, const Vector2& mouse_pos)
{
    if (t.attachment_point.has_value()) {
        auto [cx, cy] = t.attachment_point.value()->get_center();
        DrawCircleLines(cx, cy, 10, WHITE);
        auto angle = Vector2LineAngle({ cx, cy }, mouse_pos);
        auto [dx, dy] = Vector2Rotate({ 0, -15 }, angle + PI / 2);
        DrawLine(cx, cy, cx + dx, cy - dy, RED);
    } else {
        auto [cx, cy] = mouse_pos;
        DrawCircleLines(cx, cy, 10, WHITE);
        DrawLine(cx, cy, cx, cy - 15, RED);
    }
}

Rectangle
mounting_point::get_bounding_box() const
{
    auto [cx, cy] = get_center();
    auto r = Rectangle{ 0, 0, 20, 20 };
    return place_center_at(r, get_center());
}

Vector2
ship::get_center() const
{
    return { x + 20.f, y + 50.f };
}

Vector2
mounting_point::get_center() const
{
    return (Vector2{ float(offset.x()), float(offset.y()) } +
            parent.get_center());
}

void
draw_ship(const ship& s)
{
    Rectangle r{ 0, 0, 40, 100 };
    place_center_at(r, s.get_center());
    DrawRectangleLinesEx(r, 1, WHITE);
    auto [cx, cy] = s.get_center();
    DrawCircle(cx, cy, 2, RED);
    for (auto const& m : s.mounting_points) {
        auto [mcx, mcy] = m.get_center();
        DrawCircle(mcx, mcy, 2, PURPLE);
        if (!m.attachment.has_value()) {
            DrawRectangleRec(m.get_bounding_box(), Color{ 200, 0, 0, 150 });
            DrawRectangleLinesEx(m.get_bounding_box(), 1, RED);
        }
    }
}

std::ostream&
operator<<(std::ostream& os, const Vector2& c)
{
    os << "Vector2(" << c.x << ", " << c.y << ")";
    return os;
}

Vector2
operator+(Vector2& s, const Vector2& other)
{
    return Vector2Add(s, other);
}

Vector2
operator-(Vector2& s, const Vector2& other)
{
    return Vector2Subtract(s, other);
}

Vector2&
operator*(Vector2& s, const float other)
{
    s.x *= other;
    s.y *= other;
    return s;
}

struct bullet
{
    Vector2 v;
    Vector2 pos;
    decltype(std::chrono::system_clock::now()) lifetime;
    int hits = 1;
};

struct explosion_particle
{
    Vector2 pos;
    std::chrono::milliseconds lifetime;
    decltype(std::chrono::system_clock::now()) creation_time;
};

template<yhl_util::has_pos T>
Camera2D* yhl_util::quadtree<T>::c = nullptr;

int
main(void)
{
    ship s{ ship_param{ .x = 1920.f / 2, .y = 1080.f / 2 } };
    ship y{ ship_param{} };
    s.mounting_points.emplace_back(
      mounting_point{ .offset{ 20, 10 }, .parent = s });

    s.mounting_points.emplace_back(
      mounting_point{ .offset{ -20, 10 }, .parent = s });

    turret t;

    std::random_device rd{};
    auto mtgen = std::mt19937{ rd() };

    drone_manager dm{ 1000, mtgen };

    InitWindow(1920, 1080, "raylib [core] example - basic window");
    Camera2D c{};
    c.zoom = 1.0f;
    c.offset = { 1920.f / 2, 1080.f / 2 };
    yhl_util::quadtree<drone>::c = &c;
    SetTargetFPS(60);
    HideCursor();

    std::vector<bullet> bullets;

    bool qtree_debug = false;

    auto bullet_time = std::chrono::duration(std::chrono::milliseconds(10));
    auto next_time = std::chrono::system_clock::now();

    std::vector<explosion_particle> explosions;

    while (!WindowShouldClose()) {

        c.zoom += ((float)GetMouseWheelMove() * 0.2f);
        c.target = s.get_center();
        // s.y += 1;

        for (auto m = s.mounting_points.begin(); m != s.mounting_points.end();
             m++) {
            if (CheckCollisionCircleRec(
                  GetScreenToWorld2D(GetMousePosition(), c),
                  1,
                  m->get_bounding_box()) &&
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                !t.attachment_point.has_value()) {
                // attach turret to ship
                t.attachment_point = m;
                m->attachment.emplace(t);
            }
            if (CheckCollisionCircleRec(
                  GetScreenToWorld2D(GetMousePosition(), c),
                  1,
                  m->get_bounding_box()) &&
                IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
                m->attachment.has_value()) {
                t.attachment_point.value()->attachment.reset();
                t.attachment_point = std::nullopt;
            }
        }
        if (IsKeyPressed(KEY_B)) {
            qtree_debug = !qtree_debug;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            for (auto const& m : s.mounting_points) {
                if (m.attachment.has_value() &&
                    std::chrono::system_clock::now() >= next_time) {

                    bullets.emplace_back(bullet{
                      .v = Vector2Normalize(
                             GetScreenToWorld2D(GetMousePosition(), c) -
                             m.get_center()) *
                           15.f,
                      .pos = m.get_center(),
                      .lifetime = std::chrono::system_clock::now() +
                                  std::chrono::seconds(5),
                    });
                    next_time =
                      std::max(next_time + bullet_time,
                               std::chrono::system_clock::now() + bullet_time);
                }
            }
        }

        BeginDrawing();
        auto [mouse_x, mouse_y] = GetMousePosition();
        DrawCircleLines(mouse_x, mouse_y, 3, WHITE);
        ClearBackground(BLACK);
        dm.tick(s.get_center(), c);
        std::vector<typename std::vector<drone>::iterator> res;
        if (qtree_debug) {
            dm.qtree_green.draw();
            dm.qtree_yellow.draw();
            dm.qtree_green.query(
              mouse_x - 50, mouse_y - 50, 100, 100, res, true);
            dm.qtree_yellow.query(
              mouse_x - 50, mouse_y - 50, 100, 100, res, true);
        }
        DrawRectangleLines(mouse_x - 50, mouse_y - 50, 100, 100, WHITE);

        {
            BeginMode2D(c);
            for (auto& b : bullets) {
                Color c = SKYBLUE;
                c.a = 255 *
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                        b.lifetime - std::chrono::system_clock::now())
                        .count() /
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::seconds(5))
                        .count();
                DrawCircleV(b.pos, 4, c);
            }
            draw_ship(s);
            draw_ship(y);
            draw_turret(t, GetScreenToWorld2D(GetMousePosition(), c));
            auto remove_explosion = std::remove_if(
              explosions.begin(), explosions.end(), [](auto const& p) {
                  return std::chrono::system_clock::now() >
                         p.creation_time + p.lifetime;
              });
            explosions.erase(remove_explosion, explosions.end());
            for (auto const& p : explosions) {
                float r =
                  float(std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now() - p.creation_time)
                          .count()) /
                  p.lifetime.count();
                int ir = 50 * (r * r);
                DrawRing(p.pos, r * 50, ir, 0, 360, 0, ORANGE);
            }
            dm.render();
            for (auto i : res) {
                DrawCircle(i->pos.x, i->pos.y, 3, BLUE);
            }
            EndMode2D();
        }
        for (auto& b : bullets) {
            std::vector<typename std::vector<drone>::iterator> bullet_res;
            b.pos += b.v;
            auto [bx, by] = GetWorldToScreen2D(b.pos, c);
            dm.qtree_green.query(bx - 10, by - 10, 20, 20, bullet_res);
            if (bullet_res.size() > 0) {
                auto closest = bullet_res.front();
                auto current_dist = Vector2Distance(closest->pos, b.pos);
                for (auto const& br : bullet_res) {
                    auto new_dist = Vector2Distance(br->pos, b.pos);
                    if (new_dist < current_dist) {
                        current_dist = new_dist;
                        closest = br;
                    }
                }
                if (CheckCollisionCircles(b.pos, 4, closest->pos, 2)) {
                    b.hits -= 1;
                    std::cout << "hit something" << std::endl;
                    closest->health -= 1;
                    explosions.emplace_back(explosion_particle{
                      .pos = b.pos,
                      .lifetime = std::chrono::milliseconds(400),
                      .creation_time = std::chrono::system_clock::now(),
                    });
                }
            }
        }
        if (bullets.size() > 0) {
            auto it =
              std::remove_if(bullets.begin(), bullets.end(), [](auto& b) {
                  return b.lifetime < std::chrono::system_clock::now() ||
                         b.hits <= 0;
              });
            bullets.erase(it, bullets.end());
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
