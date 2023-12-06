// clang-format off

#include "common/macros.hpp"
#include "common/debug.hpp"

#include "common/stl.hpp"
#include "common/sa.hpp"
#include "common/time_scheduler.hpp"
#include "common/xorshift.hpp"
#include "common/logger.hpp"

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

    inline double calc_length_penalty(int l) const {
        const double length_threshold = N * N * LENGTH_THRESHOLD;
        return l > length_threshold ? (l - length_threshold) / length_threshold
                                    : 0.0;
    }

    inline double val(int l, double c_x, double c_l) const {
        if (l < N * N) l = N * N;
        const double length_penalty = calc_length_penalty(l);
        return (c_x * xc * f(l) + yc * f(l - 1) + z) / (2.0 * l)
               + c_l * length_penalty;
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

        static vector<int> multiple_visited;
        multiple_visited.clear();
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
                    multiple_visited.push_back(v);
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

    State copy() const {
        State ret;
        ret.seq                = seq;
        ret.score              = score;
        ret.visited_count      = visited_count;
        ret.visited_first_last = visited_first_last;
        return ret;
    }

    void copy_from(const State& s) {
        seq                = s.seq;
        score              = s.score;
        visited_count      = s.visited_count;
        visited_first_last = s.visited_first_last;
    }
};

namespace for_find_path {
    int visited[N_UB * N_UB];
    int ts = 1;

    void init() {
        ts = 1;
        fill(visited, visited + N * N, 0);
    }
    bool dfs(int v, int goal, vector<int16_t>& path) {
        visited[v] = ts;
        path.push_back(v);
        if (v == goal) return true;
        xorshift::shuffle(g[v]);
        for (int u : g[v]) {
            if (visited[u] == ts) continue;
            if (dfs(u, goal, path)) return true;
        }
        path.pop_back();
        return false;
    }
} // namespace for_find_path

vector<int16_t> find_path(int st, int gl) {
    using namespace for_find_path;
    static vector<int16_t> seq;
    seq.clear();
    ts++;
    dfs(st, gl, seq);
    assert(!seq.empty());

    return seq;
}
void init() {
    for (int i = 0; i < N * N; i++) {
        d_sum += D[i];
    }
    for_find_path::init();
}

// 代表頂点を返す
vector<int> find_cluster() {
    int threshold = 100;

    static int used[N_UB * N_UB];
    fill(used, used + N * N, 0);
    used[0] = 1;
    vector<int> ret;

    while (true) {
        // 1. unused なマスのうち d が最大のマスを探す、 d が 100 以下なら終了
        int max_d = -1;
        int max_v = -1;
        for (int i = 0; i < N * N; i++) {
            if (used[i]) continue;
            if (D[i] > max_d) {
                max_d = D[i];
                max_v = i;
            }
        }
        if (ret.empty() && threshold > max_d) {
            threshold = max_d / 2;
        }
        if (max_d < threshold) break;
        if (max_v == -1) break;
        ret.push_back(max_v);
        // cerr << cell[max_v].to_string() << ": " << max_d << endl;

        // 2. そのマスから隣接したマスのうち
        // 汚れ d' が log(d') >= log(d) - 1
        // を満たすもの高いものから順にクラスタに加えていく クラスタのサイズが N
        // になったら終了
        set<int> cluster;
        cluster.insert(max_v);
        priority_queue<pair<int, int>> pq;
        pq.emplace(max_d, max_v);
        used[max_v] = 1;
        while ((int)cluster.size() < N && !pq.empty()) {
            const auto [d, v] = pq.top();
            pq.pop();
            for (int u : g[v]) {
                if (used[u]) continue;
                if (log10(D[u]) >= log10(max_d) - 1) {
                    cluster.insert(u);
                    pq.emplace(D[u], u);
                    used[u] = 1;
                }
            }
        }
        while (!pq.empty()) {
            // used を戻す
            const auto v = pq.top().second;
            pq.pop();
            used[v] = 0;
        }
    }
    return ret;
}

State build_first_state() {
    State best;
    double best_score = 1e18;


    auto cluster = find_cluster();

    // 何回か回して最良のものを返す
    scheduler::Scheduler<300> timer;

    while (timer.update()) {
        State s;
        xorshift::shuffle(cluster);
        s.add_back(0);
        for (int k = 0; k < 1; k++) {
            for (int v : cluster) {
                auto path = find_path(s.seq.back(), v);
                assert(path.front() == s.seq.back());
                assert(path.back() == v);
                for (size_t p_i = 1; p_i < path.size(); p_i++) {
                    s.add_back(path[p_i]);
                }
            }
        }
        {
            auto path = find_path(s.seq.back(), 0);
            assert(path.front() == s.seq.back());
            assert(path.back() == 0);
            for (size_t p_i = 1; p_i < path.size(); p_i++) {
                s.add_back(path[p_i]);
            }
        }
        s.sync_score();
        const double score = s.score.val(s.seq.size() - 1, 0.0, 0);
        if (score < best_score) {
            best_score = score;
            best       = s;
        }
    }
    return best;
}

inline int manhattan(int v, int u) {
    auto vc = cell[v];
    auto uc = cell[u];
    return abs(vc.r - uc.r) + abs(vc.c - uc.c);
}

namespace transiton_arm {
    constexpr int TABLE_SIZE = 1000;

    enum OP {
        OP1, // 部分再構築
        OP2, // 条件付き部分再構築
        OP3, // 閉路反転

        // ここより下は変更しない
        OP_NUM,
    };

    constexpr array<pair<OP, int>, OP_NUM> prob = {
        make_pair(OP::OP1, 800),
        make_pair(OP::OP2, 100),
        make_pair(OP::OP3, 100),
    };

    static_assert(accumulate(prob.begin(), prob.end(), 0,
                             [](int acc, auto& p) { return acc + p.second; })
                      == TABLE_SIZE,
                  "sum of prob must be 1000");

    constexpr array<OP, TABLE_SIZE> gen_prob_seq() {
        array<OP, TABLE_SIZE> ret = {};
        int idx                   = 0;
        for (auto& [op, p] : prob) {
            for (int i = 0; i < p; i++) {
                ret[idx++] = op;
            }
        }
        return ret;
    }

    constexpr auto prob_seq = gen_prob_seq();

    struct Counter {
        int tries    = 0;
        int accepted = 0;
    } counter[100]; // 100 個以上遷移があると死ぬ

    void register_transition(OP op, bool accepted) {
        auto& c = counter[int(op)];
        c.tries++;
        if (accepted) c.accepted++;
    }

    void add_log() {
        for (int i = 0; i < OP_NUM; i++) {
            auto& c = counter[i];
            logger::push("op" + to_string(i + 1),
                         to_string(c.tries) + " tries");
            logger::push("op" + to_string(i + 1),
                         to_string(c.accepted) + " accepted");
        }
    }

    OP choice() { return prob_seq[xorshift::getInt(TABLE_SIZE)]; }

} // namespace transiton_arm

bool op1(State& s) {
    // 部分再構築
    const int L = s.seq.size();
    if (L < 2) return false;
    int len = xorshift::getInt(min(10, L - 1)) + 1;
    int bg  = xorshift::getInt(L - len);
    int ed  = bg + len;
    while (s.seq[bg] == s.seq[ed]) {
        len = xorshift::getInt(min(10, L - 1)) + 1;
        bg  = xorshift::getInt(L - len);
        ed  = bg + len;
    }
    assert(bg < ed);
    assert(bg >= 0);
    assert(ed < L);
    const auto new_path = find_path(s.seq[bg], s.seq[ed]);
    assert(new_path.front() == s.seq[bg]);
    assert(new_path.back() == s.seq[ed]);
    s.seq.erase(s.seq.begin() + bg, s.seq.begin() + ed + 1);
    s.seq.insert(s.seq.begin() + bg, new_path.begin(), new_path.end());
    s.sync_score();
    return true;
}

bool op2(State& s) {
    // 条件付き部分再構築
    const int L = s.seq.size();
    if (L < 2) return false;
    int len      = xorshift::getInt(min(10, L - 1)) + 1;
    const int bg = xorshift::getInt(L - len);
    const int ed = bg + len;
    assert(bg < ed);
    assert(bg >= 0);
    assert(ed < L);
    int v                        = s.seq[bg];
    constexpr int dist_threshold = 5;
    while (v == s.seq[bg] || v == s.seq[ed]
           || manhattan(v, s.seq[bg]) > dist_threshold) {
        v = xorshift::getInt(N * N);
    }
    // v に行くまでのパスを探す
    const auto new_path1 = find_path(s.seq[bg], v);
    // v から ed までのパスを探す
    const auto new_path2 = find_path(v, s.seq[ed]);
    assert(new_path1.front() == s.seq[bg]);
    assert(new_path1.back() == v);
    assert(new_path2.front() == v);
    assert(new_path2.back() == s.seq[ed]);
    s.seq.erase(s.seq.begin() + bg, s.seq.begin() + ed + 1);
    s.seq.insert(s.seq.begin() + bg, new_path1.begin(), new_path1.end());
    s.seq.insert(s.seq.begin() + bg + new_path1.size(), new_path2.begin() + 1,
                 new_path2.end());
    s.sync_score();
    return true;
}

bool op3(State& s) {
    // 閉路反転
    const int L = s.seq.size();
    if (L < 2) return false;

    for (int ti = 0; ti < 30; ti++) {
        const int i = xorshift::getInt(L);
        int j       = 10;
        for (; j < L && j < i + N * N / 10; j++) {
            if (s.seq[i] == s.seq[j]) break;
        }
        if (j >= L || s.seq[i] != s.seq[j]) continue;
        assert(s.seq[i] == s.seq[j]);
        reverse(s.seq.begin() + i + 1, s.seq.begin() + j);

        s.sync_score();
        return true;
    }
    return false;
}

State improve(State s) {
    scheduler::Scheduler<1950> timer;
    while (timer.update()) {
        double progress = timer.progress / double(timer.limit);
        // cerr << "progress: " << progress << endl;

        State t = s.copy();

        // cerr << op << endl;
        // cerr << "L: " << s.seq.size() - 1 << endl;
        // for (int i : s.seq) {
        //     cerr << cell[i].to_string() << " ";
        // }
        // cerr << endl;
        transiton_arm::OP op;
        for (bool ok = false; !ok;) {
            op = transiton_arm::choice();
            switch (op) {
                case transiton_arm::OP::OP1:
                    ok = op1(t);
                    break;
                case transiton_arm::OP::OP2:
                    ok = op2(t);
                    break;
                case transiton_arm::OP::OP3:
                    ok = op3(t);
                    break;
                default:
                    assert(false);
            }
        }


        double c_x                 = 1 + (1000 - 1) * progress;
        const double c_l           = LENGTH_PENALTY_COEFFICIENT;
        const double current_score = s.score.val(s.seq.size() - 1, c_x, c_l);
        const double next_score    = t.score.val(t.seq.size() - 1, c_x, c_l);
        const double temperture =
            calc_temperature(timer.progress, timer.limit, T_START, T_END);
        const double prob =
            calc_probability(next_score, current_score, temperture);
        const double r = xorshift::getDouble();
        if (r < prob) {
            s.copy_from(t);
            // cerr << "update: " << next_score << "(" << progress << ")"
            //      << " " << s.seq.size() << endl;
            // cerr << "L: " << s.seq.size() - 1 << endl;
            // cerr << t.score.xc << " " << t.score.yc << " " << t.score.z <<
            // endl; cerr << s.score.xc << " " << s.score.yc << " " << s.score.z
            // << endl;
            transiton_arm::register_transition(op, true);
        }
        else {
            transiton_arm::register_transition(op, false);
        }
    }
    return s;
}

void fix_unreachable(State& s) {
    static vector<int> pos[N_UB * N_UB];
    for (int i = 0; i < N * N; i++) {
        pos[i].clear();
    }
    vector<int> unreached;
    for (int i = 0; i < (int)s.seq.size(); i++) {
        pos[s.seq[i]].push_back(i);
    }
    for (int i = 0; i < N * N; i++) {
        if (pos[i].empty()) {
            unreached.push_back(i);
        }
    }

    while (!unreached.empty()) {
        cerr << "need to fix: " << unreached.size() << endl;
        xorshift::shuffle(unreached);
        for (auto v : unreached) {
            xorshift::shuffle(g[v]);
            bool connected = false;
            for (int u : g[v]) {
                if (pos[u].empty()) continue;

                int i = pos[u][xorshift::getInt(pos[u].size())];
                while (s.seq[i] != u) {
                    i++;
                    assert(i < N * N);
                }
                s.seq.insert(s.seq.begin() + i + 1, u);
                s.seq.insert(s.seq.begin() + i + 1, v);
                pos[v].push_back(i + 1);
                connected = true;
            }
            if (connected) {
                auto ptr = find(unreached.begin(), unreached.end(), v);
                swap(*ptr, unreached.back());
                unreached.pop_back();
                break;
            }
        }
    }
}

void dfs(State& state, int v, bool first = true) {
    static vector<int> visited;
    if (first) {
        visited.resize(N * N);
        fill(visited.begin(), visited.end(), 0);
    }
    visited[v] = 1;
    state.add_back(v);
    // xorshift::shuffle(g[v]);
    for (int u : g[v]) {
        if (visited[u]) continue;
        dfs(state, u, false);
        state.add_back(v);
    }
}

State sample() {
    State best;
    best.score.z = 1e18;
    for (int i = 0; i < 1; i++) {
        State state;
        dfs(state, 0);
        state.sync_score();
        if (state.score.val(state.seq.size() - 1, 0, 0)
            < best.score.val(state.seq.size() - 1, 0, 0)) {
            best.copy_from(state);
            // cerr << "update: " << best.score.val(state.seq.size() - 1, 0, 0)
            //      << endl;
        }
    }
    // exit(-1);
    return best;
}

int main() {
    cerr << fixed << setprecision(5);
    N = input(g, D, cell);
    init();

    logger::push("N", int64_t(N));

    // auto first = build_first_state();
    auto first = sample();
    auto ret   = improve(first);
    fix_unreachable(ret);
    ret.sync_score();

    output(ret.seq, cell);
    logger::push("L", int64_t(ret.seq.size() - 1));

    logger::push("score", ret.score.val(ret.seq.size() - 1, 1e10, 0));
    transiton_arm::add_log();
    logger::flush();
    if (ret.score.xc > 0) {
        exit(-1);
    }
    return 0;
}
