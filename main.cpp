#include "include/util.h"
#include <Eigen/Dense>
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/Rotation2D.h>
#include <iostream>
#include <optional>
#include <raylib.h>
#include <raymath.h>
#include <vector>

template<typename T>
concept has_center = requires(const T& t) {
    { t.get_center()->Vector2 };
};

struct ship;

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

struct mounting_point;
struct turret;
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
        DrawCircleLines(cx, cy, 10, BLACK);
        auto angle = Vector2LineAngle({ cx, cy }, mouse_pos);
        auto [dx, dy] = Vector2Rotate({ 0, -15 }, angle + PI / 2);
        DrawLine(cx, cy, cx + dx, cy - dy, RED);
    } else {
        auto [cx, cy] = mouse_pos;
        DrawCircleLines(cx, cy, 10, BLACK);
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
    DrawRectangleLinesEx(r, 1, BLACK);
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

int
main(void)
{
    ship s{ ship_param{} };
    ship y{ ship_param{} };
    s.mounting_points.emplace_back(
      mounting_point{ .offset{ 20, 10 }, .parent = s });

    s.mounting_points.emplace_back(
      mounting_point{ .offset{ -20, 10 }, .parent = s });

    turret t;

    InitWindow(1920, 1080, "raylib [core] example - basic window");
    Camera2D c{};
    c.zoom = 1.0f;
    c.offset = { 1920.f / 2, 1080.f / 2 };
    SetTargetFPS(60);
    HideCursor();

    while (!WindowShouldClose()) {

        c.zoom += ((float)GetMouseWheelMove() * 0.2f);
        c.target = s.get_center();
        s.y += 1;

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

        BeginDrawing();
        auto [mouse_x, mouse_y] = GetMousePosition();
        DrawCircleLines(mouse_x, mouse_y, 3, BLACK);
        ClearBackground(RAYWHITE);

        {
            BeginMode2D(c);
            draw_ship(s);
            draw_ship(y);
            draw_turret(t, GetScreenToWorld2D(GetMousePosition(), c));
            EndMode2D();
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
