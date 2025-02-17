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
    {
    }
};

struct ship
{
    double angle;
    int x;
    int y;
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

int
main(void)
{
    ship s{ .angle = 0, .x = 100, .y = 100 };
    s.mounting_points.emplace_back(
      mounting_point{ .x_offset = 20, .y_offset = 10 });

    s.mounting_points.emplace_back(
      mounting_point{ .x_offset = -20, .y_offset = 10 });

    InitWindow(1920, 1080, "raylib [core] example - basic window");
    Camera2D c{};
    c.zoom = 1.0f;
    c.offset = { 1920.f / 2, 1080.f / 2 };

    while (!WindowShouldClose()) {

        c.zoom += ((float)GetMouseWheelMove() * 0.2f);
        c.target = s.get_center();
        BeginDrawing();
        ClearBackground(RAYWHITE);

        {
            BeginMode2D(c);
            draw_ship(s);
            EndMode2D();
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
