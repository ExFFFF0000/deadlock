#pragma once
#include <frong.h>
#include <memory>
#include "log.h"
#include "math.h"
#include <string>
#include <list>
#include <unordered_map>
#include <Imgui/imgui.h>

namespace {

std::unique_ptr<frg::process> game;

uintptr_t client = 0;
uintptr_t entity_list = 0;


std::vector<std::string> bone_names = {
	"pelvis",
	"neck_0",
	"head",
	"clavicle_L",
	"arm_upper_L",
	"arm_lower_L",
	"hand_L",
	"clavicle_R",
	"arm_upper_R",
	"arm_lower_R",
	"hand_R",
	"leg_upper_L",
	"leg_lower_L",
	"ankle_L",
	"leg_upper_R",
	"leg_lower_R",
	"ankle_R",
};

std::unordered_map<std::string, std::array<int, 17>> bone_indexes;

namespace offsets {

uintptr_t entity_list = 0x23B5F78;
uintptr_t local_player = 0x2562300;
uintptr_t view_matrix = 0x2573E30;
ptrdiff_t m_hpawn = 0x60c;
ptrdiff_t m_pgamescenenode = 0x328;
ptrdiff_t m_modelstate = 0x170;
ptrdiff_t m_hmodel = 0x00D0;
ptrdiff_t m_modelname = 0x00D8;
ptrdiff_t m_voldorigin = 0x0EDC;
ptrdiff_t m_iteamnum = 0x03EF;


ptrdiff_t m_playerdataglobal = 0x0750;

ptrdiff_t m_imaxhealth =  0x0010; // int32
ptrdiff_t m_ihealth = 0x0048;
// int32

}// namespace offsets

}

namespace deadlock {

bool init() {
	game = std::make_unique<frg::process>(L"project8.exe", true);
	if (!game->valid()) {
		ERROR("Failed to attach to game\n");
		return false;
	}
	client = (uintptr_t)game->module(L"client.dll").value().base();
	if (!client) {
		ERROR("Failed to find client.dll base\n");
		return false;
	}
	LOG("client.dll base: 0x{:x}\n", client);

	LOG("Game initialized\n");
	return true;
}

void update() {
	entity_list = game->read<uintptr_t>((void*)(client + offsets::entity_list));
}

void* get_entity_by_idx(const int32_t idx)
{
	if (idx > 0x7FFE) {
		return nullptr;
	}

	if ((idx >> 9) > 0x3F) {
		return nullptr;
	}

	const auto entry_list = 
		game->read<uintptr_t>((void*)(entity_list + 8i64 * (idx >> 9) + 16));
	if (!entry_list)
		return nullptr;

	const auto player_controller = (uint32_t*)(120i64 * (idx & 0x1FF) + entry_list);
	if (!player_controller)
		return nullptr;

	return game->read<void*>(player_controller);
}

void* get_player_pawn(uintptr_t entity) {
	auto pawn_handle = game->read<uint32_t>((void*)(entity + offsets::m_hpawn ));
	return get_entity_by_idx(pawn_handle & 0x7fff);
}

uintptr_t get_local_player() {
	auto loacl_entity = game->read<uintptr_t>((void*)(client + offsets::local_player));
	return (uintptr_t )get_player_pawn(loacl_entity);
}

uintptr_t get_node(uintptr_t entity) {
	return  game->read<uintptr_t>((void*)(entity + offsets::m_pgamescenenode));
}

vec3 get_bone_position(uintptr_t node, int bone_index) {
	uintptr_t boneArray = game->read<uintptr_t>((void*)(node + offsets::m_modelstate + 0x80));
	if (!boneArray) return vec3();
	uintptr_t boneAddress = boneArray + bone_index * 0x20;
	return  game->read<vec3>((void*)boneAddress);
}

uintptr_t get_model(uintptr_t node) {
	uintptr_t hmodel = game->read<uintptr_t>((void*)(node + offsets::m_modelstate + offsets::m_hmodel));
	return game->read<uintptr_t>((void*)hmodel);
}

std::string get_bone_name(uintptr_t model, int bone_index) {
	const auto names_count = game->read<uint32_t>((void*)(model + 0x178));
	if (names_count < 0 || bone_index >= names_count)
		return "root";
	const auto names_ptr = game->read<uintptr_t>((void*)(model + 0x168));
	const auto bone_name_address = game->read<uintptr_t>((void*)(names_ptr + bone_index * 8));
	char buffer[64]{};
	game->read((void*)bone_name_address, &buffer, 64);
	return std::string{ buffer };
}

void loop_bone_name(uintptr_t model) {
	const auto names_count = game->read<uint32_t>((void*)(model + 0x178));
	for (int idx = 0; idx < names_count; idx++) {
		LOG("Bone idx {} name {}", idx, get_bone_name(model, idx));
	}
}

view_matrix_t get_view_matrix() {
	view_matrix_t view_matrix;
	game->read((void*)(client  + offsets::view_matrix), &view_matrix, sizeof(view_matrix));
	return view_matrix;
}

vec3 get_player_position(uintptr_t entity) {
	return game->read<vec3>((void*)(entity + offsets::m_voldorigin));
}




std::string get_model_name(uintptr_t node) {
	uintptr_t hmodel = game->read<uintptr_t>((void*)(node + offsets::m_modelstate + offsets::m_modelname));
	if (!hmodel)return {"none"};
	char buffer[64]{};
	game->read((void*)hmodel, &buffer, 64);
	std::string model_path(buffer);
	return model_path.substr(model_path.rfind("/") + 1, model_path.rfind(".") - model_path.rfind("/") - 1);
}

uint8_t get_team_num(uintptr_t entity) {
	return game->read<uint8_t>((void*)(entity + offsets::m_iteamnum));
}

std::array<int, 17> get_bone_indexes(uintptr_t node) {
	std::string bone_name = get_model_name(node);
	if (bone_indexes.find(bone_name) != bone_indexes.end()) {
		return bone_indexes[bone_name];
	}
	auto model = get_model(node);
	const auto names_count = game->read<uint32_t>((void*)(model + 0x178));
	std::array<int, 17> res{};
	for (int idx = 0; idx < 17;++idx) {
		for (int cur = 0; cur < 64; cur++) {
			if (bone_names[idx] == get_bone_name(model, cur)) {
				res[idx] = cur;
				break;
			}
		}
	}
	bone_indexes[bone_name] = res;
	return res;
}
vec3 get_player_head_position(uintptr_t entity) {
	auto node = get_node(entity);
	auto bone_indexes = get_bone_indexes(node);
	return get_bone_position(node, bone_indexes[2]);
}

void draw_bone(uintptr_t entity, view_matrix_t view_matrix) {
	static std::list<std::list<int>> bones = {
		{1,0},
		{1,4,5,6},
		{1,8,9,10},
		{0,11,12,13},
		{0,14,15,16},
	};
	auto node = deadlock::get_node(entity);
	auto bone_indexes = deadlock::get_bone_indexes(node);
	for (auto& part : bones) {
		vec3 previous = vec3();
		for (auto index : part) {
			auto bone_position = deadlock::get_bone_position(node, bone_indexes[index]);
			vec3 screen_position;
			if(!math::world_to_screen({ 1920, 1080 }, bone_position, screen_position, view_matrix))
				return ;
			if (screen_position.x == 0.0 && screen_position.y == 0.0) {
				return ;
			}
			if (previous.x == 0.0)
			{
				previous = screen_position;
				continue;
			}
			ImGui::GetForegroundDrawList()->
				AddLine({ previous.x, previous.y }, { screen_position.x, screen_position.y }, IM_COL32(255, 0, 0, 255));
			previous = screen_position;
		}
	}
}

int get_max_entity_count() {
	return game->read<int>((void*)(entity_list + 0x1520));
}

int get_health(uintptr_t entity) {
	return game->read<int>((void*)(entity + 0x034C));
}

int get_max_health(uintptr_t entity) {
	return game->read<int>((void*)(entity + 0x0348));
}

vec4 get_box_rect(vec3 top, vec3 bottom) {
	vec4 ret{};
	ret.h = (int)(bottom.y - top.y);
	ret.w = ret.h / 2;
	ret.x = (int)(top.x - ret.w / 2);
	ret.y = (int)(top.y);
	return ret;
}

void draw_health_bar(uintptr_t entity,vec2 pos,int width) {

}


}// namespace deadlock