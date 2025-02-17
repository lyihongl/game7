#include "include/util.h"
#include <concepts>
#include <iostream>
#include <optional>
#include <raylib.h>
#include <raymath.h>
#include <type_traits>
#include <vector>

template<typename T>
concept has_center = requires(const T& t) {
    { t.get_center()->Vector2 };
};

struct mounting_point
{
    int x_offset;
    int y_offset;
    Vector2 get_center() const;
};

Vector2
mounting_point::get_center() const
{
    return { 10.f, 10.f };
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
};

void
draw_turret(const turret& t, const Vector2& mouse_pos)
{
    if (t.attachment_point.has_value()) {

    } else {
        auto [cx, cy] = mouse_pos;
        DrawCircleLines(cx, cy, 10, BLACK);
        DrawLine(cx, cy, cx, cy - 15, RED);
    }
}

struct ship
{
    double angle;
    float x;
    float y;
    std::vector<mounting_point> mounting_points;
    Vector2 get_center() const;
};

Vector2
ship::get_center() const
{
    return { x + 20.f, y + 50.f };
}

void
draw_ship(const ship& s)
{
    DrawRectangleLines(s.x, s.y, 40, 100, BLACK);
    auto [cx, cy] = s.get_center();
    DrawCircle(cx, cy, 2, RED);
    for (auto const m : s.mounting_points) {
        DrawCircle(cx + m.x_offset, cy + m.y_offset, 2, PURPLE);
        auto [mcx, mcy] = m.get_center();
        DrawRectangle(cx + m.x_offset - mcx,
                      cy + m.y_offset - mcy,
                      20,
                      20,
                      Color{ 200, 0, 0, 150 });
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
    ship s{ .angle = 0, .x = 100, .y = 100 };
    ship y{ .angle = 0, .x = 200, .y = 100 };
    s.mounting_points.emplace_back(
      mounting_point{ .x_offset = 20, .y_offset = 10 });

    s.mounting_points.emplace_back(
      mounting_point{ .x_offset = -20, .y_offset = 10 });

    turret t;

    InitWindow(1920, 1080, "raylib [core] example - basic window");
    Camera2D c{};
    c.zoom = 1.0f;
    c.offset = { 1920.f / 2, 1080.f / 2 };
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        c.zoom += ((float)GetMouseWheelMove() * 0.2f);
        c.target = s.get_center();
        std::cout << c.target << std::endl;
        s.y += 1;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        {
            BeginMode2D(c);
            draw_ship(s);
            draw_ship(y);
            draw_turret(t, GetMousePosition() + c.target - c.offset);
            EndMode2D();
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
