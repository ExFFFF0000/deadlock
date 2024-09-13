#include <iostream>
#include "deadlock.h"
#include "render.h"


int main()
{
    deadlock::init();
    render::loop([]() {
        deadlock::update();
        auto local_player = deadlock::get_local_player();
        auto local_team = deadlock::get_team_num(local_player);
        auto view_matrix = deadlock::get_view_matrix();
        for (int idx = 0; idx < 32; ++idx) {
            auto entity = deadlock::get_entity_by_idx(idx);
            if(!entity)
                continue;
            auto pawn = deadlock::get_player_pawn((uintptr_t)entity);
            if (!pawn || (uintptr_t)pawn == local_player)
                continue;

            auto team = deadlock::get_team_num((uintptr_t)pawn);
            if (team == local_team)
                continue;

            //auto root_position = deadlock::get_player_position((uintptr_t)pawn);
            //auto head_position = deadlock::get_player_head_position((uintptr_t)pawn);

            //vec3 head_screen_position,root_screen_position;
            //if(!math::world_to_screen({ 1920, 1080 }, root_position, root_screen_position, view_matrix))
            //    continue;
            //if(!math::world_to_screen({ 1920, 1080 }, head_screen_position, head_screen_position, view_matrix))
            //    continue;

            //auto rect = deadlock::get_box_rect(head_position, root_position);

            //deadlock::draw_health_bar((uintptr_t)pawn, { (float)rect.x,(float)rect.y }, 100);
            deadlock::draw_bone((uintptr_t)pawn, view_matrix);
        }

    });
    return 0;
}
