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

// 差分計算用構造体
struct F {
    int64_t a = 0;
    int64_t b = 0;
    inline int64_t calc(int64_t q) { return a * q * q + b * q; }
};

struct Score {
    int64_t never_visited    = 0;
    int64_t once_visited     = 0;
    int64_t multiple_visited = 0;

    int64_t len = 0;
    static F fs[L_MAX];
    static vector<int> pos[N_UB * N_UB];

    void add_multiple_visited(int64_t subseq_len, int64_t d) {
        multiple_visited += d * subseq_len * (subseq_len + 1);
    }

    void sub_multiple_visited(int64_t subseq_len, int64_t d) {
        multiple_visited -= d * subseq_len * (subseq_len + 1);
    }

    inline double calc_length_penalty() const {
        const double length_threshold = N * N * LENGTH_THRESHOLD;
        return len > length_threshold
                   ? (len - length_threshold) / length_threshold
                   : 0.0;
    }
    inline double score_with_penalty(double never_visited_penalty,
                                     double length_penalty_coef) const {
        auto l                      = max(len, int64_t(N) * N);
        const double length_penalty = calc_length_penalty();
        return (never_visited_penalty * never_visited * l * (l + 1) // 未訪問
                + once_visited * l * (l - 1) // 1 回訪問
                + multiple_visited           // 複数回訪問
                ) / (2.0 * len)
               + length_penalty_coef * length_penalty;
    }
};
F Score::fs[L_MAX];
vector<int> Score::pos[N_UB * N_UB];

template <typename Container>
Score build_score(Container seq) {
    auto score          = Score();
    score.len           = seq.size() - 1;
    score.never_visited = d_sum;
    for (int i = 0; i < N * N; i++) {
        Score::pos[i].clear();
    }
    static vector<int> multiple_visited;
    multiple_visited.clear();
    for (int i = 0; i < (int)seq.size(); i++) {
        const int v = seq[i];

        // auto& first = visited_first_last[v].first;
        switch (Score::pos[v].size()) {
            case 0:
                score.never_visited -= D[v];
                score.once_visited += D[v];
                break;
            case 1:
                score.once_visited -= D[v];
                score.add_multiple_visited(i - Score::pos[v].back() - 1, D[v]);
                multiple_visited.push_back(v);
                break;
            default:
                score.add_multiple_visited(i - Score::pos[v].back() - 1, D[v]);
                break;
        }
        Score::pos[v].push_back(i);
        Score::fs[i] = {0, 0};
    }

    // cerr << score.xc << " " << score.yc << " " << score.z << endl;

    for (int v : multiple_visited) {
        score.add_multiple_visited((seq.size() - Score::pos[v].back() - 1)
                                       + (Score::pos[v].front() - 1),
                                   D[v]);
    }


    for (int v = 0; v < N * N; v++) {
        if (Score::pos[v].size() >= 2u) {
            {
                const int last  = Score::pos[v].back();
                const int first = Score::pos[v].front();
                const int l     = (seq.size() - last - 1) + (first - 1);

                const int64_t da = D[v];
                const int64_t db = 2 * D[v] * l + D[v];
                Score::fs[0].a += da;
                Score::fs[0].b += db;
                Score::fs[first + 1].a -= da;
                Score::fs[first + 1].b -= db;
                Score::fs[last + 1].a += da;
                Score::fs[last + 1].b += db;
            }
            for (int i = 1; i < (int)Score::pos[v].size(); i++) {
                const int p      = Score::pos[v][i];
                const int q      = Score::pos[v][i - 1];
                const int l      = p - q - 1;
                const int64_t da = D[v];
                const int64_t db = 2 * D[v] * l + D[v];
                Score::fs[p + 1].a -= da;
                Score::fs[p + 1].b -= db;
                Score::fs[q + 1].a += da;
                Score::fs[q + 1].b += db;
            }
        }
    }
    for (int i = 1; i < (int)seq.size(); i++) {
        Score::fs[i].a += Score::fs[i - 1].a;
        Score::fs[i].b += Score::fs[i - 1].b;
        assert(Score::fs[i].a >= 0);
        // cerr << "fs: " << i << " " << Score::fs[i].a << " " << Score::fs[i].b
        //      << endl;
    }

    return score;
}


struct State {
    vector<int16_t> seq;
    Score score;
    State() { seq.clear(); }

    void sync_score() { score = build_score(seq); }

    inline void add_back(int v) { seq.push_back(v); }

    State copy() const {
        State ret;
        ret.seq   = seq;
        ret.score = score;
        return ret;
    }

    void copy_from(const State& s) {
        seq   = s.seq;
        score = s.score;
    }
};

namespace for_modified_state {
    int current_ts = 0;
    int timestamp[N_UB * N_UB];
    int visited_count[N_UB * N_UB];
    vector<int> pos_l[N_UB * N_UB];
    vector<int> pos_m[N_UB * N_UB];
    vector<int> pos_r[N_UB * N_UB];

} // namespace for_modified_state


struct ModifiedState {
    const State* state;
    int bg;
    int ed;
    vector<int16_t> new_subseq;
    Score score;

    int size() const {
        return state->seq.size() - (ed - bg + 1) + new_subseq.size();
    }

    int16_t operator[](int i) const {
        if (i < bg) {
            return state->seq[i];
        }
        else if (i < bg + (int)new_subseq.size()) {
            return new_subseq[i - bg];
        }
        else {
            return state->seq[i + (ed - bg + 1) - new_subseq.size()];
        }
    }

    Score calc_new_score() {
        const int64_t old_len = state->seq.size() - 1;
        // cerr << "###################" << endl;
        const int64_t q = new_subseq.size() - (ed - bg + 1); // 長さの増分
        const int64_t new_len = old_len + q;
        // cerr << "bg: " << bg << endl;
        // cerr << "ed: " << ed << endl;
        // cerr << "q: " << q << endl;

        // cerr << "old: ";
        // for (int i = 0; i < (int)state->seq.size(); i++) {
        //     cerr << state->seq[i] << ",";
        // }
        // cerr << endl;
        // cerr << "new: ";
        // for (int i = 0; i < (int)size(); i++) {
        //     cerr << this->operator[](i) << ",";
        // }
        // cerr << endl;

        Score score;
        score.never_visited    = state->score.never_visited;
        score.once_visited     = state->score.once_visited;
        score.multiple_visited = state->score.multiple_visited;
        score.len              = new_len;

        const auto& old_pos = Score::pos;

        using namespace for_modified_state;
        current_ts++;
        static vector<int> target_unique;
        target_unique.clear();

        // 使うやつだけコピー
        for (int i = bg; i <= ed; i++) {
            const int v      = state->seq[i];
            visited_count[v] = old_pos[v].size();
            if (timestamp[v] != current_ts) {
                timestamp[v] = current_ts;
                target_unique.push_back(v);
                pos_l[v].clear();
                pos_m[v].clear();
                pos_r[v].clear();
            }
        }
        for (int i = 0; i < (int)new_subseq.size(); i++) {
            const int v      = new_subseq[i];
            visited_count[v] = old_pos[v].size();
            if (timestamp[v] != current_ts) {
                timestamp[v] = current_ts;
                target_unique.push_back(v);
                pos_l[v].clear();
                pos_m[v].clear();
                pos_r[v].clear();
            }
            pos_m[v].push_back(bg + i);
        }
        auto f = Score::fs[bg];
        // cerr << "before f: " << f.a << " " << f.b << endl;
        for (int v : target_unique) {
            for (int i = 0; i < (int)old_pos[v].size(); i++) {
                const int p = old_pos[v][i];
                if (p < bg) {
                    pos_l[v].push_back(p);
                }
                else if (p > ed) {
                    pos_r[v].push_back(p + q);
                }
                else {
                    switch (visited_count[v]) {
                        case 1:
                            score.never_visited += D[v];
                            score.once_visited -= D[v];
                            break;
                        case 2:
                            score.once_visited += D[v];
                            break;
                        default:
                            // nankaareba
                            break;
                    }

                    visited_count[v]--;
                }
                if (old_pos[v].size() < 2) continue;
                const int pp = (i == 0) ? old_pos[v].back() - old_len // 怪しい
                                        : old_pos[v][i - 1];


                score.sub_multiple_visited(p - pp - 1, D[v]);

                if ((pp < bg && p >= bg) || (pp + old_len < bg)) { // 怪しい
                    f.a -= D[v];
                    f.b -= (p - pp - 1) * 2 * D[v] + D[v];
                }
            }
        }

        for (int i = 0; i < (int)new_subseq.size(); i++) {
            const int v = new_subseq[i];
            switch (visited_count[v]) {
                case 0:
                    score.never_visited -= D[v];
                    score.once_visited += D[v];
                    break;
                case 1:
                    score.once_visited -= D[v];
                    // score.multiple_visited += D[v] * (i + 1) * (i + 2);
                    break;
                default:
                    // score.multiple_visited += D[v] * (i + 1) * (i + 2);
                    break;
            }
            visited_count[v]++;
        }

        for (int v : target_unique) {
            if (visited_count[v] < 2) {
                continue;
            }
            int first = -1;
            int last  = -1;
            {
                const auto& pos = pos_l[v];
                for (int i = 0; i < (int)pos.size(); i++) {
                    if (first == -1)
                        first = pos[i];
                    else
                        score.add_multiple_visited(pos[i] - last - 1, D[v]);
                    last = pos[i];
                }
            }
            {
                const auto& pos = pos_m[v];
                for (int i = 0; i < (int)pos.size(); i++) {
                    if (first == -1)
                        first = pos[i];
                    else
                        score.add_multiple_visited(pos[i] - last - 1, D[v]);
                    last = pos[i];
                }
            }
            {
                const auto& pos = pos_r[v];
                for (int i = 0; i < (int)pos.size(); i++) {
                    if (first == -1)
                        first = pos[i];
                    else
                        score.add_multiple_visited(pos[i] - last - 1, D[v]);
                    last = pos[i];
                }
            }
            score.add_multiple_visited((new_len - last) + (first - 1), D[v]);
        }
        score.multiple_visited += f.calc(q);

        // cerr << "after f: " << f.a << " " << f.b << endl;
        // cerr << "f.calc(q): " << f.calc(q) / (2 * new_len) << endl;
        // cerr << "detail: " << score.never_visited << " " <<
        // score.once_visited
        //      << " " << score.multiple_visited << endl;

        return score;
    }

    void sync_score() { score = calc_new_score(); }

    State build_state() {
        State ret = state->copy();
        ret.seq   = state->seq;
        ret.seq.erase(ret.seq.begin() + bg, ret.seq.begin() + ed + 1);
        ret.seq.insert(ret.seq.begin() + bg, new_subseq.begin(),
                       new_subseq.end());
        ret.sync_score();
        return ret;
    }
};

namespace for_find_path {
    int visited[N_UB * N_UB];
    int ts = 1;
    int dist[N_UB * N_UB][N_UB * N_UB];

    void init() {
        ts = 1;
        fill(visited, visited + N * N, 0);
        queue<int> q;
        for (int i = 0; i < N * N; i++) {
            fill(dist[i], dist[i] + N * N, N_UB * N_UB);
            dist[i][i] = 0;
            q.push(i);
            while (!q.empty()) {
                int v = q.front();
                q.pop();
                for (int u : g[v]) {
                    if (dist[i][u] <= dist[i][v] + 1) continue;
                    dist[i][u] = dist[i][v] + 1;
                    q.push(u);
                }
            }
        }
    }
    inline bool dfs(int v, int goal, vector<int16_t>& path) {
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

    inline void shortest_path(int v, int goal, vector<int16_t>& path) {
        path.push_back(v);
        while (v != goal) {
            int min_dist = N_UB * N_UB;
            int min_v    = -1;
            xorshift::shuffle(g[v]);
            for (int u : g[v]) {
                if (dist[goal][u] < min_dist) {
                    min_dist = dist[goal][u];
                    min_v    = u;
                }
            }
            assert(min_v != -1);
            path.push_back(min_v);
            v = min_v;
        }
    }
} // namespace for_find_path

vector<int16_t> find_path(int st, int gl, bool use_shortest_path = false) {
    using namespace for_find_path;
    static vector<int16_t> seq;
    seq.clear();
    ts++;
    if (use_shortest_path)
        shortest_path(st, gl, seq);
    else
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
        OP4, // 閉路追加
        OP5, // 閉路削除
        OP6, // 部分再構築(最短経路)
        // ここより下は変更しない
        OP_NUM,
    };

    constexpr array<pair<OP, int>, OP_NUM> prob = {
        make_pair(OP::OP1, OP1_P), make_pair(OP::OP2, OP2_P),
        make_pair(OP::OP3, OP3_P), make_pair(OP::OP4, OP4_P),
        make_pair(OP::OP5, OP5_P), make_pair(OP::OP6, OP6_P),
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
        double score = 0;
    } counter[100]; // 100 個以上遷移があると死ぬ

    void register_transition(OP op, bool accepted, double score = 0) {
        auto& c = counter[int(op)];
        c.tries++;
        if (accepted) c.accepted++;
        c.score += score;
    }

    void add_log() {
        for (int i = 0; i < OP_NUM; i++) {
            auto& c = counter[i];
            logger::push("op" + to_string(i + 1),
                         to_string(c.tries) + " tries");
            logger::push("op" + to_string(i + 1),
                         to_string(c.accepted) + " accepted");
            logger::push("op" + to_string(i + 1),
                         to_string(c.score) + " point");
            if (c.tries > 0) {
                logger::push("op" + to_string(i + 1),
                             to_string(c.score / double(c.tries))
                                 + " point/try");
            }
        }
    }

    OP choice() { return prob_seq[xorshift::getInt(TABLE_SIZE)]; }

} // namespace transiton_arm

optional<ModifiedState> op1(const State& s, bool use_shortest_path) {
    // cerr << "op1" << endl;
    // 部分再構築
    const int L = s.seq.size();
    if (L < 2) return nullopt;
    int len = 1;
    int bg  = 0;
    int ed  = 0;
    while (s.seq[bg] == s.seq[ed]) {
        len = xorshift::getInt(min(10, L - 1)) + 1; // 増やしていいかも
        bg  = xorshift::getInt(L - len);
        ed  = bg + len;
    }
    assert(bg < ed);
    assert(bg >= 0);
    assert(ed < L);
    const auto new_path = find_path(s.seq[bg], s.seq[ed], use_shortest_path);
    assert(new_path.front() == s.seq[bg]);
    assert(new_path.back() == s.seq[ed]);
    int n_bg = 0;
    int n_ed = new_path.size() - 1;
    while (n_bg <= n_ed && bg <= ed && s.seq[bg] == new_path[n_bg]) {
        bg++;
        n_bg++;
    }
    while (n_bg <= n_ed && bg <= ed && s.seq[ed] == new_path[n_ed]) {
        ed--;
        n_ed--;
    }
    if (bg > ed && n_bg > n_ed) return nullopt;
    if (bg >= (int)s.seq.size()) {
        bg--;
        ed--;
        n_bg--;
        n_ed--;
    }
    ModifiedState ret;
    ret.state = &s;
    ret.bg    = bg;
    ret.ed    = ed;
    ret.new_subseq =
        vector<int16_t>(new_path.begin() + n_bg, new_path.begin() + n_ed + 1);
    ret.sync_score();
    return ret;
}

optional<ModifiedState> op1(const State& s) { return op1(s, false); }

optional<ModifiedState> op6(const State& s) { return op1(s, true); }

optional<ModifiedState> op2(const State& s) {
    // cerr << "op2" << endl;
    // 条件付き部分再構築
    const int L = s.seq.size();
    if (L < 2) return nullopt;
    int len = xorshift::getInt(min(10, L - 1)) + 1;
    int bg  = xorshift::getInt(L - len);
    int ed  = bg + len;
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
    auto new_path1 = find_path(s.seq[bg], v);
    // v から ed までのパスを探す
    const auto new_path2 = find_path(v, s.seq[ed]);
    assert(new_path1.front() == s.seq[bg]);
    assert(new_path1.back() == v);
    assert(new_path2.front() == v);
    assert(new_path2.back() == s.seq[ed]);
    new_path1.insert(new_path1.end(), new_path2.begin() + 1, new_path2.end());
    int n_bg = 0;
    int n_ed = new_path1.size() - 1;
    while (n_bg <= n_ed && bg <= ed && s.seq[bg] == new_path1[n_bg]) {
        bg++;
        n_bg++;
    }
    while (n_bg <= n_ed && bg <= ed && s.seq[ed] == new_path1[n_ed]) {
        ed--;
        n_ed--;
    }
    if (bg > ed && n_bg > n_ed) return nullopt;
    if (bg >= (int)s.seq.size()) {
        bg--;
        ed--;
        n_bg--;
        n_ed--;
    }

    ModifiedState ret;
    ret.state = &s;
    ret.bg    = bg;
    ret.ed    = ed;
    ret.new_subseq =
        vector<int16_t>(new_path1.begin() + n_bg, new_path1.begin() + n_ed + 1);
    ret.sync_score();
    return ret;
}

optional<ModifiedState> op3(const State& s) {
    // cerr << "op3" << endl;
    // 閉路反転
    const int L = s.seq.size();
    if (L < 2) return nullopt;

    for (int ti = 0; ti < 30; ti++) {
        int bg = xorshift::getInt(L);
        int ed = bg + 10;
        for (; ed < L && ed < bg + N * N / 10; ed++) {
            if (s.seq[bg] == s.seq[ed]) break;
        }
        if (ed >= L || s.seq[bg] != s.seq[ed]) continue;
        assert(s.seq[bg] == s.seq[ed]);
        auto new_path =
            vector<int16_t>(s.seq.begin() + bg, s.seq.begin() + ed + 1);
        reverse(new_path.begin(), new_path.end());
        int n_bg = 0;
        int n_ed = new_path.size() - 1;
        while (n_bg <= n_ed && bg <= ed && s.seq[bg] == new_path[n_bg]) {
            bg++;
            n_bg++;
        }
        while (n_bg <= n_ed && bg <= ed && s.seq[ed] == new_path[n_ed]) {
            ed--;
            n_ed--;
        }
        if (bg > ed && n_bg > n_ed) return nullopt;
        if (bg >= (int)s.seq.size()) {
            bg--;
            ed--;
            n_bg--;
            n_ed--;
        }
        // cerr << "bg: " << bg << endl;
        // cerr << "ed: " << ed << endl;
        // cerr << "n_bg: " << n_bg << endl;
        // cerr << "n_ed: " << n_ed << endl;
        ModifiedState ret;
        ret.state      = &s;
        ret.bg         = bg;
        ret.ed         = ed;
        ret.new_subseq = vector<int16_t>(new_path.begin() + n_bg,
                                         new_path.begin() + n_ed + 1);

        ret.sync_score();
        return ret;
    }
    return nullopt;
}

optional<ModifiedState> op4(const State& s) {
    // cerr << "op4" << endl;
    // 閉路構築
    const int p = xorshift::getInt(s.seq.size() - 1) + 1;
    int v       = s.seq[p];
    while (v == s.seq[p] || manhattan(v, s.seq[p]) > 5) {
        v = xorshift::getInt(N * N);
    }

    auto new_path = find_path(s.seq[p], v);
    assert(new_path.front() == s.seq[p]);
    assert(new_path.back() == v);
    const auto new_path2 = find_path(v, s.seq[p]);
    assert(new_path2.front() == v);
    assert(new_path2.back() == s.seq[p]);
    new_path.insert(new_path.end(), new_path2.begin() + 1, new_path2.end() - 1);
    int bg = p;
    int ed = p - 1;
    ModifiedState ret;
    ret.state      = &s;
    ret.bg         = bg;
    ret.ed         = ed;
    ret.new_subseq = new_path;

    ret.sync_score();
    return ret;
}

optional<ModifiedState> op5(const State& s) {
    // 閉路削除
    const int L = s.seq.size();
    if (L < 2) return nullopt;

    for (int ti = 0; ti < 30; ti++) {
        int bg = xorshift::getInt(L);
        int ed = bg + 10;
        for (; ed < L && ed < bg + N * 5; ed++) {
            if (s.seq[bg] == s.seq[ed]) break;
        }
        if (ed >= L || s.seq[bg] != s.seq[ed]) continue;
        assert(s.seq[bg] == s.seq[ed]);
        ModifiedState ret;
        ret.state      = &s;
        ret.bg         = bg + 1;
        ret.ed         = ed;
        ret.new_subseq = vector<int16_t>();

        ret.sync_score();
        return ret;
    }
    return nullopt;
}

State improve(State s) {
    scheduler::Scheduler<1950> timer;
    // output(s.seq, cell);
    while (timer.update()) {
        double progress = timer.progress / double(timer.limit);
        // cerr << "progress: " << progress << endl;

        // cerr << op << endl;
        // cerr << "L: " << s.seq.size() - 1 << endl;
        // for (int i : s.seq) {
        //     cerr << cell[i].to_string() << " ";
        // }
        // cerr << endl;
        transiton_arm::OP op;
        optional<ModifiedState> t = nullopt;
        while (!t.has_value()) {
            op = transiton_arm::choice();
            switch (op) {
                case transiton_arm::OP::OP1:
                    t = op1(s);
                    break;
                case transiton_arm::OP::OP2:
                    t = op2(s);
                    break;
                case transiton_arm::OP::OP3:
                    t = op3(s);
                    break;
                case transiton_arm::OP::OP4:
                    t = op4(s);
                    break;
                case transiton_arm::OP::OP5:
                    t = op5(s);
                    break;
                case transiton_arm::OP::OP6:
                    t = op6(s);
                    break;
                default:
                    assert(false);
                    exit(-1);
            }
        }


        double c_x                 = 1 + (1000 - 1) * progress;
        const double c_l           = LENGTH_PENALTY_COEFFICIENT;
        const double current_score = s.score.score_with_penalty(c_x, c_l);
        const double next_score = t.value().score.score_with_penalty(c_x, c_l);
        const double temperture =
            calc_temperature(timer.progress, timer.limit, T_START, T_END);
        double prob = calc_probability(next_score, current_score, temperture);
        if (current_score == next_score) prob = 0.5;
        const double r = xorshift::getDouble();
        // cerr << current_score << " <-> " << next_score << endl;

        if (r < prob) {
            s.copy_from(t.value().build_state());
            // cerr << "actual: " << t.value().score.score_with_penalty(c_x,
            // c_l)
            //      << endl;
            // cerr << "expected: " << s.score.score_with_penalty(c_x, c_l)
            //      << endl;
            // cerr << "actual_detail: " << t.value().score.never_visited << " "
            //      << t.value().score.once_visited << " "
            //      << t.value().score.multiple_visited << endl;
            // cerr << "expected_detail: " << s.score.never_visited << " "
            //      << s.score.once_visited << " " << s.score.multiple_visited
            //      << endl;
            // output(s.seq, cell);
            if (abs(s.score.score_with_penalty(c_x, c_l) - next_score) > 1e-5) {
                // なんかしらんけど一部のオプションで誤差が出る
                cerr << "ng" << endl;
                exit(-1);
            }
            // cerr << "update: " << next_score << "(" << temperture << ")"
            //      << endl;
            //
            //      << " " << s.seq.size() << endl;
            // cerr << "L: " << s.seq.size() - 1 << endl;
            // cerr << t.score.xc << " " << t.score.yc << " " << t.score.z
            // << endl; cerr << s.score.xc << " " << s.score.yc << " " <<
            // s.score.z
            // << endl;
            transiton_arm::register_transition(op, true,
                                               next_score - current_score);
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
                    // assert(i < N * N);
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
    best.score.multiple_visited = 1e18;
    best.score.len              = 2;
    for (int i = 0; i < 1; i++) {
        State state;
        dfs(state, 0);
        state.sync_score();
        if (state.score.score_with_penalty(0, 0)
            < best.score.score_with_penalty(0, 0)) {
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

    logger::push("score", ret.score.score_with_penalty(1e10, 0));
    transiton_arm::add_log();
    logger::flush();
    if (ret.score.never_visited > 0) {
        exit(-1);
    }
    return 0;
}
