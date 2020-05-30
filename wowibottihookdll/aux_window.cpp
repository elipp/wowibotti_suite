#include <windows.h>
#include <gl/GL.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include <array>
#include <numeric>

#include "defs.h"
#include "dllmain.h"
#include "aux_window.h"
#include "shader.h"
#include "wowmem.h"
#include "linalg.h"
#include "timer.h"

#pragma comment (lib, "opengl32.lib")

static int aux_window_created = 0;
static int running = 0;

static std::string conf_to_set = "";

static HWND hWnd;
static HDC hDC;
static HGLRC hRC;

static ShaderProgram *avoid_shader;
static ShaderProgram *imp_shader;
static ShaderProgram *rev_shader;
static ShaderProgram *debug_shader;

static hcache_t hcache;

const hcache_t& get_hcache() {
	return hcache;
}

static std::array<BYTE, HMAP_SIZE * HMAP_SIZE> pixbuf;

static const hconfig_t* current_hconfig;
static int num_avoid_points;

static int hot_enabled = 0;

static hotness_status_t hstatus;
static HANDLE render_thread;

static const GLfloat scr[2] = { HMAP_SIZE, HMAP_SIZE };

static std::unordered_map<std::string, Render> renders;


int hotness_enabled() {
	return hot_enabled;
}

void hotness_enable(bool state) {
	if (!current_hconfig) {
		ECHO_WOW("hotness enable called but no config set! use hconfig set <confname> first");
		hot_enabled = 0;
		return;
	}
	else {
		hot_enabled = state;
		ECHO_WOW("hotness set to %d", hot_enabled);
	}
}


static const std::unordered_map<std::string, hconfig_t> hconfigs = {
	// this will leak memory when ejected :D NVM
	{"Marrowgar", hconfig_t("Lord Marrowgar",
		{ new avoid_npc_t(15, FALLOFF_CONSTANT, "Lord Marrowgar"), new avoid_npc_t(9, FALLOFF_CUBIC, "Coldflame"), new avoid_spellobject_t(9, FALLOFF_CUBIC, 69146), new avoid_units_t(8, FALLOFF_LINEAR) },
		//REV_SELF | REV_BOSS,
		0,
		{ 
		arena_impassable_t(vec2(-401.8, 2170), vec2(-0.762509, -0.646977)),
		arena_impassable_t(vec2(-422.9, 2200.4), vec2(-1.000000, 0.000000)),
		arena_impassable_t(vec2(-412.5, 2241.4), vec2(-0.834276, 0.551347)),
		arena_impassable_t(vec2(-372.9, 2263.8), vec2(0.854369, 0.519668)),
		arena_impassable_t(vec2(-357.7, 2182.9), vec2(0.850798, -0.525493)),
		})
	},
	{"Rotface", hconfig_t("Rotface",
	{new avoid_npc_t(12, FALLOFF_LINEAR, "Sticky Ooze") }, REV_SELF, {}) },

	{"Putricide", hconfig_t("Professor Putricide",
	{ //new avoid_npc_t(15, "Professor Putricide"), 
	new avoid_npc_t(12, FALLOFF_CONSTANT, "Choking Gas Bomb"), new avoid_npc_t(16, FALLOFF_QUADRATIC, "Growing Ooze Puddle") },
	REV_SELF | REV_FOCUS, {} )},
};


int hconfig_set(const std::string& confname) {
	conf_to_set = confname;
	return 1;
}

hotness_status_t hotness_status() {
	return hstatus;
}

void update_hotness_cache() {
	hcache_t::mutex.lock();
	hcache = OMgr->get_snapshot();
	hcache_t::mutex.unlock();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

fp_glGenVertexArrays glGenVertexArrays;
fp_glBindVertexArray glBindVertexArray;
fp_glGenBuffers glGenBuffers;
fp_glBindBuffer glBindBuffer;
fp_glEnableVertexAttribArray glEnableVertexAttribArray;
fp_glDisableVertexAttribArray glDisableVertexAttribArray;
fp_glVertexAttribPointer glVertexAttribPointer;
fp_glVertexAttribIPointer glVertexAttribIPointer;
fp_glCreateShader glCreateShader;
fp_glShaderSource glShaderSource;
fp_glGetShaderiv glGetShaderiv;
fp_glCompileShader glCompileShader;
fp_glGetShaderInfoLog glGetShaderInfoLog;
fp_glAttachShader glAttachShader;
fp_glLinkProgram glLinkProgram;
fp_glGetProgramiv glGetProgramiv;
fp_glGetProgramInfoLog glGetProgramInfoLog;
fp_glDetachShader glDetachShader;
fp_glDeleteShader glDeleteShader;
fp_glBufferData glBufferData;
fp_glUseProgram glUseProgram;
fp_glCreateProgram glCreateProgram;
fp_glBufferSubData glBufferSubData;
fp_glGetActiveUniform glGetActiveUniform;

fp_glUniform1f glUniform1f;
fp_glUniform2f glUniform2f;
fp_glUniform3f glUniform3f;
fp_glUniform4f glUniform4f;
fp_glUniform1fv glUniform1fv;
fp_glUniform2fv glUniform2fv;
fp_glUniform3fv glUniform3fv;
fp_glUniform4fv glUniform4fv;
fp_glGetUniformLocation glGetUniformLocation;

static int initialize_gl_extensions() {

	// quite a bit cleaner with this macro, i'm saying :D
#define LOADEXT(name) { name = (fp_ ## name)wglGetProcAddress(#name); assert(name); }

	LOADEXT(glGenVertexArrays);
	LOADEXT(glBindVertexArray);
	LOADEXT(glGenBuffers);
	LOADEXT(glBindBuffer);
	LOADEXT(glEnableVertexAttribArray);
	LOADEXT(glDisableVertexAttribArray);
	LOADEXT(glVertexAttribPointer);
	LOADEXT(glVertexAttribIPointer);
	LOADEXT(glCreateShader);
	LOADEXT(glShaderSource);
	LOADEXT(glCompileShader);
	LOADEXT(glGetShaderiv);
	LOADEXT(glGetShaderInfoLog);
	LOADEXT(glAttachShader);
	LOADEXT(glLinkProgram);
	LOADEXT(glGetProgramiv);
	LOADEXT(glGetProgramInfoLog);
	LOADEXT(glDetachShader);
	LOADEXT(glDeleteShader);
	LOADEXT(glUseProgram);
	LOADEXT(glBufferData);
	LOADEXT(glCreateProgram);
	LOADEXT(glGetActiveUniform);
	LOADEXT(glUniform1f);
	LOADEXT(glUniform2f);
	LOADEXT(glUniform3f);
	LOADEXT(glUniform4f);
	LOADEXT(glUniform1fv);
	LOADEXT(glUniform2fv);
	LOADEXT(glUniform3fv);
	LOADEXT(glUniform4fv);
	LOADEXT(glGetUniformLocation);
	LOADEXT(glBufferSubData);

	return 1;

}

static int hconfig_set_actual(const std::string& confname) {
	try {
		const auto& c = hconfigs.at(confname);
		if (current_hconfig == &c) {
			return 1;
		}
		current_hconfig = &c;
		ECHO_WOW("hconfig_set: config set to %s", confname.c_str());
		if (current_hconfig->impassable.size() > 0) {
			auto& r = renders.at("imp");
			r.update_buffer(&current_hconfig->impassable[0], current_hconfig->impassable.size() * sizeof(arena_impassable_t));

		}

		return 1;
	}
	catch (const std::exception& e) {
		ECHO_WOW("hconfig_set: error: %s", e.what());
		current_hconfig = NULL;
		return 0;
	}
}

static inline int clampi(int i, int min, int max) {
	if (i < min) return min;
	if (i > max) return max;
	return i;
}

static vec2_t tex2screen(int x, int y) {
	// assume square texture
	float fx = (float(x)) / (float)HMAP_SIZE;
	float fy = (float(y)) / (float)HMAP_SIZE;

	return vec2(2 * fx - 1, 2 * fy - 1);
}
static vec2i_t screen2tex(float x, float y) {
	float xt = clampi((x + 1) * HMAP_SIZE / 2.0f, 0, HMAP_SIZE - 1);
	float yt = clampi((y + 1) * HMAP_SIZE / 2.0f, 0, HMAP_SIZE - 1);
	return { (int)round(xt), (int)round(yt) };
}

static vec2_t world2screen(float x, float y) {
	const vec3& p = hcache.player.pos;
	return vec2((-y + p.y) / (0.5f * ARENA_SIZE), (x - p.x) / (0.5f * ARENA_SIZE));
}

static vec2_t screen2world(float x, float y) {
	const vec3& p = hcache.player.pos;
	return vec2(p.x + y*0.5*ARENA_SIZE, p.y - x*0.5*ARENA_SIZE);
}

static vec2_t tex2world(int x, int y) {
	vec2_t t = tex2screen(x, y);
	return screen2world(t.x, t.y);
}

static vec2i_t world2tex(float x, float y) {
	vec2_t scr = world2screen(x, y);
	return screen2tex(scr.x, scr.y);
}

int HMAP_get_flatindex_from_worldpos(const vec3& pos) {
	vec2i_t tex = world2tex(pos.x, pos.y);
	return HMAP_FLAT_INDEX(tex.x, tex.y);
}

std::vector<avoid_point_t> avoid_npc_t::get_points() const {
	std::vector<avoid_point_t> p;

	std::lock_guard<std::mutex> lg(hcache_t::mutex);
	for (const auto &o : hcache.objects) {
		if (o.type == OBJECT_TYPE_NPC) {
			if (o.name == this->name) {
				p.emplace_back(avoid_point_t(o.pos.x, o.pos.y, this->radius, this->falloff));
			}
		}
	}

	return p;
}

std::vector<avoid_point_t> avoid_spellobject_t::get_points() const {
	std::vector<avoid_point_t> p;

	std::lock_guard<std::mutex> lg(hcache_t::mutex);
	for (const auto &o : hcache.objects) {
		if (o.type == OBJECT_TYPE_DYNAMICOBJECT) {
			if (o.DO_spellid == this->spellID) {
				p.emplace_back(avoid_point_t(o.pos.x, o.pos.y, this->radius, this->falloff));
			}
		}
	}

	return p;
}

std::vector<avoid_point_t> avoid_units_t::get_points() const {
	std::vector<avoid_point_t> p;
	GUID_t player_guid = OMgr->get_local_GUID();

	std::lock_guard<std::mutex> lg(hcache_t::mutex);
	for (const auto &o : hcache.objects) {
		if (o.type == OBJECT_TYPE_UNIT) {
			if (o.GUID != player_guid) {
				p.emplace_back(avoid_point_t(o.pos.x, o.pos.y, this->radius, this->falloff));
			}
		}
	}

	return p;
}

arena_impassable_t::arena_impassable_t(vec2_t p, vec2_t n) {
	vec2_t un = unit(n);
	vec2_t perp1 = perp(un);
	vec2_t v1 = p + 1000 * perp1;
	vec2_t v2 = p - 1000 * perp1;
	vec2_t v3 = p + 1500 * n;
	this->tri = { v1, v2, v3 };
}



std::vector<rev_target_t> hconfig_t::get_rev_targets() const {
	ObjectManager OM;
	GUID_t pGUID = OM.get_local_GUID();
	std::vector<rev_target_t> r;

	if (rev_flags & REV_SELF) {
		const WO_cached *p = &hcache.player;
		r.push_back({ p->pos.x, p->pos.y, 24 });
	}

	if (rev_flags & REV_BOSS) {
		const WO_cached* b = hcache.find(bossname);
		if (b) {
			r.push_back({ b->pos.x, b->pos.y, 30 });
		}
	}

	if (rev_flags & REV_TARGET) {
		const WO_cached* t = hcache.find(hcache.target_GUID);
		if (t) {
			r.push_back({ t->pos.x, t->pos.y, 30 });
		}
	}

	if (rev_flags & REV_FOCUS) {
		const WO_cached* f = hcache.find(hcache.focus_GUID);
		if (f) {
			r.push_back({ f->pos.x, f->pos.y, 32 });
		}
	}

	return r;

}

static void init_avoid_render(Render* r) {
	glGenVertexArrays(1, &r->VAOid);
	glBindVertexArray(r->VAOid);

	glGenBuffers(1, &r->VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, r->VBOid);
	glBufferData(GL_ARRAY_BUFFER, 512 * sizeof(avoid_point_t), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(avoid_point_t), // stride
		(void*)0            // array buffer offset
	);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(
		1,
		1,
		GL_INT,
		sizeof(avoid_point_t),
		BUFFER_OFFSET(3 * sizeof(GLfloat))
	);

	glBindVertexArray(0);
}

static void draw_avoid(Render* r) {

	if (num_avoid_points > 0) {
		glUseProgram(r->shader->programHandle());
		glUniform2fv(r->shader->get_uniform_location("render_target_size"), 1, scr);
		glUniform2fv(r->shader->get_uniform_location("player_pos"), 1, &hcache.player.pos.x);
		glUniform1f(r->shader->get_uniform_location("arena_size"), ARENA_SIZE);

		glBindVertexArray(r->VAOid);
		glDrawArrays(GL_POINTS, 0, num_avoid_points);
	}
}

static void init_imp_render(Render* r) {

	glGenVertexArrays(1, &r->VAOid);
	glBindVertexArray(r->VAOid);

	glGenBuffers(1, &r->VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, r->VBOid);
	glBufferData(GL_ARRAY_BUFFER, 64 * sizeof(arena_impassable_t), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	glBindVertexArray(0);
}

static void draw_imp(Render* r) {

	if (current_hconfig->impassable.size() > 0) {
		glUseProgram(r->shader->programHandle());
		glUniform2fv(r->shader->get_uniform_location("player_pos"), 1, &hcache.player.pos.x); // should work, even though pos actually a vec3
		glUniform1f(r->shader->get_uniform_location("arena_size"), ARENA_SIZE);

		glBindVertexArray(r->VAOid);
		glDrawArrays(GL_TRIANGLES, 0, current_hconfig->impassable.size() * 3);
	}

}

static void init_rev_render(Render* r) {

	glGenVertexArrays(1, &r->VAOid);
	glBindVertexArray(r->VAOid);
	glGenBuffers(1, &r->VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, r->VBOid);
	static const tri_t fullscreen_tri = { vec2(-1, -1), vec2(3, -1), vec2(-1, 3) }; // might want to add a margin
	glBufferData(GL_ARRAY_BUFFER, 1 * sizeof(tri_t), &fullscreen_tri, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	glBindVertexArray(0);

}

static void draw_rev(Render* r) {

	glUseProgram(r->shader->programHandle());
	glBindVertexArray(r->VAOid);
	glUniform2fv(r->shader->get_uniform_location("render_target_size"), 1, scr);
	glUniform2fv(r->shader->get_uniform_location("player_pos"), 1, &hcache.player.pos.x);
	glUniform1f(r->shader->get_uniform_location("arena_size"), ARENA_SIZE);

	auto rev = current_hconfig->get_rev_targets();
	for (const auto& R : rev) {
		glUniform2fv(r->shader->get_uniform_location("world_pos"), 1, &R.pos.x);
		glUniform1f(r->shader->get_uniform_location("radius"), R.radius);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

}

static void init_debug_render(Render* r) {
	glGenVertexArrays(1, &r->VAOid);
	glBindVertexArray(r->VAOid);
	glGenBuffers(1, &r->VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, r->VBOid);
	glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(tri_t), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	glBindVertexArray(0);
}

static const double halfpi = 0.5 * M_PI;
static const double dpi = 2.0 * M_PI / 3.0;

static inline tri_t get_tri(vec2_t pos, float size) {

	return { 
		pos + size * vec2(cos(halfpi),				sin(halfpi)),
		pos + size * vec2(cos(halfpi + dpi),		sin(halfpi + dpi)),
		pos + size * vec2(cos(halfpi + 2.0 * dpi),	sin(halfpi + 2.0 * dpi)) };

}

static inline tri_t get_arrowhead(vec2_t pos, float size, float theta) {

	return {
		pos + 1.8*size * vec2(cos(halfpi + theta),				sin(halfpi + theta)),
		pos + size * vec2(cos(halfpi + dpi + theta),		sin(halfpi + dpi + theta)),
		pos + size * vec2(cos(halfpi + 2.0 * dpi + theta),	sin(halfpi + 2.0 * dpi + theta)) };

}


template <typename T, int S>
struct heaparray {
	std::array<T, S>* p;
	heaparray() : p(new std::array<T, S>()) {}
	~heaparray() { delete p; }
	T& operator[](int flatindex) {
		return (*p)[flatindex];
	}
	std::array<T, S>& operator*() {
		return *p;
	}

	std::size_t size() const {
		return p->size();
	}

	auto begin() {
		return p->begin();
	}

	auto end() {
		return p->end();
	}

	constexpr T* get() {
		return &(*p)[0];
	}
};

static std::vector<avoid_point_t> avoid_points;

typedef std::vector<vec2_t> waypoint_vec_t;

struct XDpathnode_t;
void get_pathfork_XD(const line_segment &ls, std::vector<circle> *valid_circles, XDpathnode_t *current);

struct XDpathnode_t {
	line_segment segment;
	XDpathnode_t *prev; // to make extracting paths a bit easier
	std::array<XDpathnode_t*, 2> next;
	std::vector<circle> *valid_circles;
	int num_next;
	XDpathnode_t(const line_segment &ls, std::vector<circle> *vc) : segment(ls), valid_circles(vc), prev(nullptr), num_next(0), next{nullptr, nullptr} {}
	XDpathnode_t(const vec2_t& from, const vec2_t &to, std::vector<circle> *vc) : XDpathnode_t({from, to}, vc) {}
	XDpathnode_t() : XDpathnode_t({0}, {0}, nullptr) {}
	void compute_next(const vec2_t &wp) {
		assert(num_next < 2);
		auto *vn = add_waypoint(wp);
		get_pathfork_XD(vn->segment, valid_circles, vn);
		++num_next;
	}

	XDpathnode_t *add_waypoint(const vec2_t &wp) {
		next[num_next] = new XDpathnode_t(wp, segment.end, valid_circles);
		next[num_next]->prev = this;
		++num_next;
		return next[num_next - 1];
	}

	~XDpathnode_t() {
		for (auto &n : next) {
			if (n) delete n;
		}
	}

	waypoint_vec_t get_path() const {
		waypoint_vec_t r;
		r.push_back(segment.start); // for the last node, segment.start == segment.end
		const auto *p = this->prev;
		while (p) {
			r.push_back(p->segment.start);
			p = p->prev;
		}
		std::reverse(r.begin(), r.end());
		return r;
	}

};

struct lsc {
	line_segment ls;
	std::vector<circle> unchecked_circles;
	lsc(const line_segment& l) : ls(l) {}
};

struct XDfork {
	waypoint_vec_t path1;
	waypoint_vec_t path2;
};

template <typename T>
void SWAP(T &a, T &b) {
	T temp = a;
	a = b;
	b = temp;
}

template <typename T>
inline void back_insert(std::vector<T> &v, std::initializer_list<T> &&stuff) {
	v.insert(v.end(), stuff);
}

static waypoint_vec_t interpolate_steps_between(const vec2_t &r1, const vec2_t &r2, int direction, int steps = 5) {
	waypoint_vec_t r;

	float angle = acosf(dot(r1, r2));

	float a_incr = angle / (float)steps;

	for (int i = 0; i < steps; ++i) {
		r.push_back(rotate(r1, direction * i * a_incr));
	}
	
}

XDfork get_XDfork(const line_segment &ls, const circle& c) {

	vec2_t b = ls.start - c.center;
	float alpha = acosf(c.radius/length(b));

	vec2_t b_cwr = c.center + c.radius * unit(rotate(b, alpha));
	vec2_t b_ccwr = c.center + c.radius * unit(rotate(b, -alpha));

	vec2_t e = ls.end - c.center;
	float beta = acosf(c.radius/length(e));

	vec2_t e_cwr = c.center + c.radius * unit(rotate(e, beta));
	vec2_t e_ccwr = c.center + c.radius * unit(rotate(e, -beta));

//	vec2_t s1 = c.center + c.radius * (unit(b_cwr + e_ccwr)); // average is ok, but we probably should use more steps 
//  NOTE: this doesn't work when we add c.center already before this
//	vec2_t s2 = c.center + c.radius * (unit(b_ccwr + e_cwr));

	XDfork r;
	
	back_insert(r.path1, {b_cwr, e_ccwr});
	std::sort(r.path1.begin(), r.path1.end(), [&](const vec2_t &a, const vec2_t &b) -> bool {
		return length(a - ls.start) < length(b - ls.start);
	});

	back_insert(r.path2, {e_cwr, b_ccwr});

	std::sort(r.path2.begin(), r.path2.end(), [&](const vec2_t &a, const vec2_t &b) -> bool {
		return length(a - ls.start) < length(b - ls.start);
	});

	return r;
}

std::vector<circle>::iterator find_offending_circle(const vec2_t& pos, std::vector<circle> *circles) {
	for (auto it = circles->begin(); it != circles->end(); ++it) {
		if (inside(pos, *it)) {
			return it;
		}
	}
	return circles->end();
}

std::vector<circle>::iterator find_intersecting_circle(const line_segment &ls, std::vector<circle> *circles) {
	for (auto it = circles->begin(); it != circles->end(); ++it) {
		if (intersection(ls, *it)) {
			return it;
		}
	}
	return circles->end();
}

inline void DELETE_CIRCLE(std::vector<circle> *cv, std::vector<circle>::iterator &it) {
//	PRINT("[%p] erased circle (%f, %f) - ", &cv, it->center.x, it->center.y);
	cv->erase(it);
//	PRINT("cv.size (after erase): %zu\n", cv->size());
}

void get_pathfork_XD(const line_segment &ls, std::vector<circle> *valid_circles, XDpathnode_t *current) {
	
	if (ls.start == ls.end) { return; }

	auto it = find_intersecting_circle(ls, valid_circles);

	if (it == valid_circles->end()) {
		current->compute_next(ls.end); // no offending circles, so we simply assign start == end
		return;
	}

	else {
		//PRINT("found intersecting circle at (%f, %f)\n", it->center.x, it->center.y);
	}

	circle ic = *it; // take copy, it's needed in the next step
	DELETE_CIRCLE(valid_circles, it);

	auto f = get_XDfork(ls, ic);
	
	// maybe we should pre-prune any circles that are completely (or enough) inside each other

	// TODO: reimplement this
	
//	for (auto& ff : f.start) {
//		// see if any of the waypoints are inside another circle
//		auto oc = find_offending_circle(ff, valid_circles);
//		while (oc != valid_circles->end()) {
//			PRINT("found offending circle at %f, %f (r = %f)\n", oc->center.x, oc->center.y, oc->radius);
//			XDfork xf = get_XDfork(ls, *oc);
//			vec2_t *fine = xf.select_non_offending_or_closer(ic, ff);
//			if (!fine) {
//				PRINT("fine was nullptr (ie. both forkpoints were inside the original circle of intersection)\n");
//				return;
//			}
//
//			ff = *fine;
//			DELETE_CIRCLE(valid_circles, oc);
//			oc = find_offending_circle(ff, valid_circles);
//		}
//	}

	auto *nc = current->add_waypoint(f.path1[0]);
	nc->compute_next(f.path1[1]);

	nc = current->add_waypoint(f.path2[0]);
	nc->compute_next(f.path2[1]);

}

inline void print_XDpath(const XDpathnode_t& node, int level = 0) {
	if (node.next.size() == 0) {
		PRINT("---- path terminating ----\n");
		return;
	}

//	PRINT("level %d - root: (%f, %f)\n", level, node.segment.start.x, node.segment.start.y);
	for (const auto& n : node.next) {
		if (!n) continue;
		PRINT("level %d - next: (%f, %f)\n", level, n->segment.start.x, n->segment.start.y);
		print_XDpath(*n, level + 1);
	}
}

void extract_all_paths(const XDpathnode_t *node, std::vector<waypoint_vec_t> *paths) {

	if (node->num_next == 0) {
		paths->emplace_back(node->get_path());
		return;
	}

	for (const auto &n : node->next) {
		if (n) extract_all_paths(n, paths);
	}
}

bool get_path_XD(const vec2_t& from, const vec2_t& to, std::vector<waypoint_vec_t> *paths_out, std::vector<circle> *circles) {

	Timer t;

	const line_segment ls(from, to);
	XDpathnode_t root(ls, circles);

	get_pathfork_XD(ls, circles, &root);

	extract_all_paths(&root, paths_out);

	PRINT("get_path_XD took %f ms\n", t.get_ms());

	return true;

}

static heaparray<int, HMAP_SIZE * HMAP_SIZE> path;
static heaparray<float, HMAP_SIZE * HMAP_SIZE> weights;

static std::vector<ivec2> get_path(const vec3& from, const vec3& to) {
	hotness_convert_grid_astar(weights.get());

	int start = HMAP_get_flatindex_from_worldpos(from);
	int end = HMAP_get_flatindex_from_worldpos(to);
	
	std::vector<ivec2> coords;

	if (astar(weights.get(), HMAP_SIZE, HMAP_SIZE, start, end, true, path.get())) {
		int e = end;
		float total_length = 0;
		ivec2 prevc = HMAP_UNRAVEL_FLATINDEX(e);
		while (e != start) {
			ivec2 c = HMAP_UNRAVEL_FLATINDEX(e);
			coords.push_back(c);
			e = path[e];
			total_length += (sqrt(pow(prevc.x - c.x, 2.0f) + pow(prevc.y - c.y, 2.0f)));
			prevc = c;
		}
		//ECHO_WOW("total_length: %f\n", total_length);
	}


	return coords;

}

static void draw_debug(Render* r) {
	glUseProgram(r->shader->programHandle());
	glBindVertexArray(r->VAOid);
	
	// the red channel is reserved for the "hotness"
	static const GLfloat blue[] = { 0, 0, 1 };
	static const GLfloat green[] = { 0, 1, 0 };
	glUniform3fv(r->shader->get_uniform_location("color"), 1, green);
	vec2_t pos = tex2screen(hstatus.best_pixel.x, hstatus.best_pixel.y);
	tri_t tri = get_tri(pos, 0.015);

	r->update_buffer(&tri, sizeof(tri));
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glUniform3fv(r->shader->get_uniform_location("color"), 1, blue);
	tri = get_arrowhead({ 0, 0 }, 0.040, hcache.player.rot);

	r->update_buffer(&tri, sizeof(tri));
	glDrawArrays(GL_TRIANGLES, 0, 3);

	vec2_t path_from = { hcache.player.pos.x, hcache.player.pos.y };
	vec2_t path_to{ -386, 2254 };
	//vec3 path_to(path_from + 50*unitvec_from_rot(hcache.player.rot));

	std::vector<waypoint_vec_t> paths;
	std::vector<circle> circles;
	std::transform(avoid_points.begin(), avoid_points.end(), std::back_inserter(circles), [](const avoid_point_t& p) { return circle{ p.pos, p.radius }; });
	std::sort(circles.begin(), circles.end(), [&](const circle& a, const circle& b) { return length(a.center - path_from) < length(b.center - path_from); });

	get_path_XD(path_from, path_to, &paths, &circles);

	for (const auto &P : paths) {
		std::vector<vec2_t> vertices;
		std::transform(P.begin(), P.end(), std::back_inserter(vertices), [](const vec2_t &wp) -> vec2_t { return world2screen(wp.x, wp.y); });
		r->update_buffer(&vertices[0], vertices.size() * sizeof(vec2_t));
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size());
	}
}

static int initialize_renders() {
	renders.insert({ "avoid", Render(avoid_shader, init_avoid_render, draw_avoid) });
	renders.insert({ "imp", Render(imp_shader, init_imp_render, draw_imp) });
	renders.insert({ "rev", Render(rev_shader, init_rev_render, draw_rev) });
	renders.insert({ "debug", Render(debug_shader, init_debug_render, draw_debug) });
	return 1;
}


static void update_buffers() {

	if (!hotness_enabled()) return;

	WowObject p;
	
	avoid_points = std::vector<avoid_point_t>();
	avoid_points.reserve(256 * sizeof(avoid_point_t));

	if (!OMgr->get_local_object(&p)) return;
	
	for (const auto& a : current_hconfig->avoid) {
		auto v = a->get_points();
		avoid_points.insert(std::end(avoid_points), std::begin(v), std::end(v));
	}

	num_avoid_points = avoid_points.size();
	if (num_avoid_points == 0) return;

	auto& r = renders.at("avoid");
	r.update_buffer(&avoid_points[0], avoid_points.size() * sizeof(avoid_point_t));

}

static inline BYTE get_pixel(int x, int y) {
	return pixbuf[HMAP_FLAT_INDEX(x, y)];
}

static inline float pixbyte_to_float01(BYTE val) {
	return (float)val / (float)0xFF;
}

BYTE hmap_get_pixel_float01(int x, int y) {
	return pixbyte_to_float01(get_pixel(x, y));
}

static pixinfo_t get_lowest_pixel() {
	vec2i_t lowest = { 0,0 };
	BYTE lowest_val = 255;
	for (int y = 0; y < HMAP_SIZE; ++y) {
		for (int x = 0; x < HMAP_SIZE; ++x) {
			BYTE p = get_pixel(x, y);
			if (p < lowest_val) {
				lowest_val = p;
				lowest = { x, y };
			}
		}
	}
	
	return { lowest, lowest_val };
}

static void update_hstatus() {

	if (!current_hconfig) return;

	pixinfo_t lowest = get_lowest_pixel();
	hstatus.best_pixel = lowest.pos;
	hstatus.best_hotness = lowest.value;
	vec2_t w = tex2world(lowest.pos.x, lowest.pos.y);
	vec2_t bu = 1.5 * unit(w - vec2(hcache.player.pos.x, hcache.player.pos.y));
	hstatus.best_world_pos = vec3(w.x + bu.x, w.y + bu.y, hcache.player.pos.z); // the boss arenas are generally flat
	WowObject p;
	OMgr->get_local_object(&p);
	vec3 ppos = p.get_pos();
	vec2i_t ppos_tex = world2tex(ppos.x, ppos.y);
	hstatus.current_hotness = get_pixel(ppos_tex.x, ppos_tex.y);

	//PRINT("lowest: (%d, %d), value %u (world: %f, %f), (player pixel: %u, %u), current_hotness = %u\n", lowest.pos.x, lowest.pos.y, lowest.value, w.x, w.y, ppos_tex.x, ppos_tex.y, hstatus.current_hotness);
}

void aux_draw() {

	if (!aux_window_created || !hotness_enabled()) { return; }

	glClearColor(0, 0, 0, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	for (auto& it : renders) {
		it.second.draw();
	}
	
	glReadPixels(0, 0, HMAP_SIZE, HMAP_SIZE, GL_RED, GL_UNSIGNED_BYTE, &pixbuf[0]);
	update_hstatus();


}


void hotness_convert_grid_astar(float *r) {
	
	for (int x = 0; x < HMAP_SIZE; ++x) {
		for (int y = 0; y < HMAP_SIZE; ++y) {
			int findex = HMAP_FLAT_INDEX(x, y);
			r[findex] = 5.0f * pixbyte_to_float01(get_pixel(x, y));
		}
	}
}



void aux_hide() {
	ShowWindow(hWnd, SW_HIDE);
}
void aux_show() {
	ShowWindow(hWnd, SW_SHOW);
	// we don't want to lose focus to the aux window
	SetForegroundWindow(wow_hWnd);
	SetFocus(wow_hWnd);
}

void hotness_stop() {
	running = 0;
	WaitForSingleObject(render_thread, INFINITE);
}

typedef struct window_parms_t {
	std::string title;
	int width;
	int height;
} window_parms_t;

#define CLASSNAME "ogl hotness"

static DWORD WINAPI createwindow(LPVOID lpParam) {

	window_parms_t *p = static_cast<window_parms_t*>(lpParam);
	int width = p->width;
	int height = p->height;
	std::string title = p->title;

	delete p;

	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;

	RECT WindowRect;
	WindowRect.left = (long)0;
	WindowRect.right = (long)width;
	WindowRect.top = (long)0;
	WindowRect.bottom = (long)height;


	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = NULL;
	wc.hIcon = NULL;// LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = NULL; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASSNAME;

	if (!RegisterClass(&wc))
	{
		PRINT("registerclass failed: %d\n", GetLastError());
		MessageBox(NULL, "FAILED TO REGISTER THE WINDOW CLASS.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = width;
	dmScreenSettings.dmPelsHeight = height;
	dmScreenSettings.dmBitsPerPel = 32;
	dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;


	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU);


	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if (!(hWnd = CreateWindowEx(dwExStyle, CLASSNAME, title.c_str(),
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
		0, 0,
		WindowRect.right - WindowRect.left,
		WindowRect.bottom - WindowRect.top,
		NULL,
		NULL,
		NULL,
		NULL)))
	{
		MessageBox(NULL, "window creation error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}


	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!(hDC = GetDC(hWnd)))
	{
		MessageBox(NULL, "CANT CREATE A GL DEVICE CONTEXT.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		MessageBox(NULL, "cant find a suitable pixelformat.", "ERROUE", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}


	if (!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		MessageBox(NULL, "Can't SET ZE PIXEL FORMAT.", "ERROU", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(hRC = wglCreateContext(hDC)))
	{
		MessageBox(NULL, "WGLCREATECONTEXT FAILED.", "ERREUHX", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!wglMakeCurrent(hDC, hRC))
	{
		MessageBox(NULL, "Can't activate the gl rendering context.", "ERAIX", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	// disable close button =D 
	EnableMenuItem(GetSystemMenu(hWnd, FALSE), SC_CLOSE,
		MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	initialize_gl_extensions();

//	typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
//   ((PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT"))(1);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	try {
		avoid_shader = new ShaderProgram("avoid");
		imp_shader = new ShaderProgram("imp");
		rev_shader = new ShaderProgram("rev");
		debug_shader = new ShaderProgram("debug");
	}
	catch (const std::exception& ex) {
		PRINT("A shader error occurred: %s\n", ex.what());
		MessageBox(NULL, "Shader error", "error", 0);
		return FALSE;
	}

	initialize_renders();

	avoid_shader->cache_uniform_locations({ "render_target_size", "player_pos", "arena_size" });
	imp_shader->cache_uniform_locations({ "arena_size", "player_pos" });
	rev_shader->cache_uniform_locations({ "world_pos", "radius", "render_target_size", "player_pos", "arena_size" });
	debug_shader->cache_uniform_locations({ "color" });

	memset(&pixbuf[0], 0, pixbuf.size());
	//PRINT("imp size: %d\n", Marrowgar.impassable.size() * sizeof(tri_t));
	ECHO_WOW("Auxiliary OGL window successfully created!");
	ECHO_WOW("(hidden by default, use /lole hconfig show)!");

	running = 1;

	MSG msg = { 0 };

	timer_interval_t interval_25ms(25);

	while (running && WM_QUIT != msg.message)
	{
		interval_25ms.reset();
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (conf_to_set != "") {
			hconfig_set_actual(conf_to_set);
			conf_to_set = "";
		}
		
		update_buffers();
		aux_draw();
		Sleep((DWORD)std::max(interval_25ms.remaining_ms(), 0.0));
		SwapBuffers(hDC);
	}

	return TRUE;
}



int create_aux_window(const char* title, int width, int height) {

	if (aux_window_created) return 1;
	aux_window_created = 1;

	window_parms_t *p = new window_parms_t;
	p->title = title;
	p->width = width;
	p->height = height;

	render_thread = CreateThread(NULL, 0, createwindow, (LPVOID)p, 0, NULL);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
	}
}

void opengl_cleanup() {
	delete rev_shader;
	delete avoid_shader;
	delete imp_shader;
	delete debug_shader;
	wglDeleteContext(hRC);
	DestroyWindow(hWnd);
	UnregisterClass(CLASSNAME, GetModuleHandle(NULL));
}