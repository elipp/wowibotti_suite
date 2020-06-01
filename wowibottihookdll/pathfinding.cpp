#include <queue>
#include <limits>
#include <cmath>

#include "pathfinding.h"

// represents a single pixel
class Node {
public:
    int idx;     // index in the flattened grid
    float cost;  // cost of traversing this pixel

    Node(int i, float c) : idx(i), cost(c) {}
};

// the top of the priority queue is the greatest element by default,
// but we want the smallest, so flip the sign
bool operator<(const Node& n1, const Node& n2) {
    return n1.cost > n2.cost;
}

bool operator==(const Node& n1, const Node& n2) {
    return n1.idx == n2.idx;
}

// See for various grid heuristics:
// http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html#S7
// L_\inf norm (diagonal distance)
float linf_norm(int i0, int j0, int i1, int j1) {
    return std::max(std::abs(i0 - i1), std::abs(j0 - j1));
}

float euclidean(int i0, int j0, int i1, int j1) {
    float i = (i0 - i1);
    float j = (j0 - j1);
    return sqrt(i * i + j * j);
}

// L_1 norm (manhattan distance)
float l1_norm(int i0, int j0, int i1, int j1) {
    return std::abs(i0 - i1) + std::abs(j0 - j1);
}

// weights:        flattened h x w grid of costs
// h, w:           height and width of grid
// start, goal:    index of start/goal in flattened grid
// diag_ok:        if true, allows diagonal moves (8-conn.)
// paths (output): for each node, stores previous node in path

#pragma optimize("t", on)

extern "C" bool astar(
    const float* weights, const int h, const int w,
    const int start, const int goal, bool diag_ok,
    int* paths) {

    const float INF = std::numeric_limits<float>::infinity();

    Node start_node(start, 0.);
    Node goal_node(goal, 0.);

    float* costs = new float[h * w];
    for (int i = 0; i < h * w; ++i)
        costs[i] = INF;
    costs[start] = 0.;

    std::priority_queue<Node> nodes_to_visit;
    nodes_to_visit.push(start_node);

    int* nbrs = new int[8];

    bool solution_found = false;
    while (!nodes_to_visit.empty()) {
        // .top() doesn't actually remove the node
        Node cur = nodes_to_visit.top();

        if (cur == goal_node) {
            solution_found = true;
            break;
        }

        nodes_to_visit.pop();

        int row = cur.idx / w;
        int col = cur.idx % w;
        // check bounds and find up to eight neighbors: top to bottom, left to right
        nbrs[0] = (diag_ok && row > 0 && col > 0) ? cur.idx - w - 1 : -1;
        nbrs[1] = (row > 0) ? cur.idx - w : -1;
        nbrs[2] = (diag_ok && row > 0 && col + 1 < w) ? cur.idx - w + 1 : -1;
        nbrs[3] = (col > 0) ? cur.idx - 1 : -1;
        nbrs[4] = (col + 1 < w) ? cur.idx + 1 : -1;
        nbrs[5] = (diag_ok && row + 1 < h && col > 0) ? cur.idx + w - 1 : -1;
        nbrs[6] = (row + 1 < h) ? cur.idx + w : -1;
        nbrs[7] = (diag_ok && row + 1 < h && col + 1 < w) ? cur.idx + w + 1 : -1;

        float heuristic_cost;
        for (int i = 0; i < 8; ++i) {
            if (nbrs[i] >= 0) {
                // the sum of the cost so far and the cost of this move
                float new_cost = costs[cur.idx] + weights[nbrs[i]];
                if (new_cost < costs[nbrs[i]]) {
                    // estimate the cost to the goal based on legal moves
                    if (diag_ok) {
                        heuristic_cost = euclidean(nbrs[i] / w, nbrs[i] % w,
                            goal / w, goal % w);
                    }
                    else {
                        heuristic_cost = l1_norm(nbrs[i] / w, nbrs[i] % w,
                            goal / w, goal % w);
                    }

                    // paths with lower expected cost are explored first
                    float priority = new_cost + heuristic_cost;
                    nodes_to_visit.push(Node(nbrs[i], priority));

                    costs[nbrs[i]] = new_cost;
                    paths[nbrs[i]] = cur.idx;
                }
            }
        }
    }

    delete[] costs;
    delete[] nbrs;

    return solution_found;
}

#pragma optimize( "", on ) // turns the cmdline-provided optimization back on

bool inside(const vec2_t& p, const circle& crc) {
    return length(p - crc.center) < crc.radius;
}

bool intersection(const line_segment& ln, const circle& crc) {
    vec2_t d = ln.end - ln.start;
    vec2_t f = ln.start - crc.center;

    const float& r = crc.radius;

    float a = dot(d, d);
    float b = 2 * dot(f, d);
    float c = dot(f, f) - r * r;

    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
    {
        return false;
    }
    else
    {
        // ray didn't totally miss sphere,
        // so there is a solution to
        // the equation.

        discriminant = sqrt(discriminant);

        // either solution may be on or off the ray so need to test both
        // t1 is always the smaller value, because BOTH discriminant and
        // a are nonnegative.
        float t1 = (-b - discriminant) / (2 * a);
        float t2 = (-b + discriminant) / (2 * a);

        // 3x HIT cases:
        //          -o->             --|-->  |            |  --|->
        // Impale(t1 hit,t2 hit), Poke(t1 hit,t2>1), ExitWound(t1<0, t2 hit), 

        // 3x MISS cases:
        //       ->  o                     o ->              | -> |
        // FallShort (t1>1,t2>1), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>1)

        if (t1 >= 0 && t1 <= 1)
        {
            // t1 is the intersection, and it's closer than t2
            // (since t1 uses -b - discriminant)
            // Impale, Poke
            return true;
        }

        // here t1 didn't intersect so we are either started
        // inside the sphere or completely past it
        if (t2 >= 0 && t2 <= 1)
        {
            // ExitWound
            return true;
        }

        // no intn: FallShort, Past, CompletelyInside
        return false;
    }
}