// clang-format off

#include "common/macros.hpp"
#include "common/debug.hpp"

#include "common/stl.hpp"
#include "common/sa.hpp"
#include "common/time_scheduler.hpp"
#include "common/xorshift.hpp"

#include "constant.hpp"
#include "io.hpp"
// clang-format on


int N;
int D[N_UB * N_UB];
Cell cell[N_UB * N_UB];
vector<vector<int>> g;

int64_t d_sum = 0;

struct Score {
    // 未訪問のマスに対する係数
    int64_t xc = 0;
    // 1 回訪問したマスに対する係数
    int64_t yc = 0;

    inline int64_t f(int l) const { return l * (l + int64_t(1)); }

    int64_t z = 0;
    inline void add_z(int l, int64_t d) { z += d * f(l); }
    inline void sub_z(int l, int64_t d) { z -= d * f(l); }

    inline double val(int l, double c_x) const {
        return (c_x * xc * f(l) + yc * f(l - 1) + z) / (2.0 * l);
    }
};

struct State {
    vector<int16_t> seq;
    Score score;

    vector<int> visited_count;
    vector<pair<int, int>> visited_first_last;
    State() {
        seq.clear();
        score    = Score();
        score.xc = d_sum;
        visited_count.resize(N * N);
        visited_first_last.resize(N * N);
    }

    void sync_score() {
        score    = Score();
        score.xc = d_sum;
        fill(visited_count.begin(), visited_count.end(), 0);
        fill(visited_first_last.begin(), visited_first_last.end(),
             make_pair(-1, -1));

        set<int> multiple_visited;

        for (size_t i = 0; i < seq.size(); i++) {
            const int v = seq[i];

            // auto& first = visited_first_last[v].first;
            auto& last = visited_first_last[v].second;
            switch (visited_count[v]) {
                case 0:
                    score.xc -= D[v];
                    score.yc += D[v];
                    break;
                case 1:
                    score.yc -= D[v];
                    assert(last != -1);
                    score.add_z(i - last - 1, D[v]);
                    multiple_visited.insert(v);
                    break;
                default:
                    assert(last != -1);
                    score.add_z(i - last - 1, D[v]);
                    break;
            }
            visited_count[v]++;
            if (visited_first_last[v].first == -1) {
                visited_first_last[v].first = i;
            }
            visited_first_last[v].second = i;
        }

        // cerr << score.xc << " " << score.yc << " " << score.z << endl;

        for (int v : multiple_visited) {
            const auto [first, last] = visited_first_last[v];
            score.add_z((seq.size() - last - 1) + (first - 1), D[v]);
        }
    }

    inline void add_back(int v) { seq.push_back(v); }
};

void dfs(State& state, int v) {
    static vector<int> visited(0);
    if (visited.empty()) {
        visited.resize(N * N, 0);
    }
    visited[v] = 1;
    state.add_back(v);
    for (int u : g[v]) {
        if (visited[u]) continue;
        dfs(state, u);
        state.add_back(v);
    }
}

State sample() {
    State state;
    dfs(state, 0);
    return state;
}

int main() {
    cerr << fixed << setprecision(5);
    N = input(g, D, cell);
    for (int i = 0; i < N * N; i++) {
        d_sum += D[i];
    }
    cerr << "N: " << N << endl;
    auto ret = sample();
    ret.sync_score();
    cerr << "score: " << ret.score.val(ret.seq.size() - 1, 1e10) << endl;
    output(ret.seq, cell);
    return 0;
}
