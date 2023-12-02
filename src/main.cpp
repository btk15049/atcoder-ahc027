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


using pile_type  = vector<int>;
using piles_type = vector<pile_type>;

piles_type piles;

struct Pos {
    int pile;
    int i; // 下からいくつか
};

struct Board {
    piles_type piles;
    Pos pos[N];

    Board(piles_type piles) : piles(piles) {
        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < N / M; ++j) {
                pos[piles[i][j]] = {i, j};
            }
        }
    }

    int head(int pile) const { return piles[pile].back(); }
};

/// @return costs
int move_piles(Board& b, int from, int i, int to) {
    // add p[from][i:] back of p[to]
    auto& from_pile = b.piles[from];
    auto& to_pile   = b.piles[to];
    int len         = from_pile.size() - i;
    int cost        = len + 1;
    if (from == to) {
        return cost;
    }
    to_pile.insert(to_pile.end(), from_pile.begin() + i, from_pile.end());
    from_pile.erase(from_pile.begin() + i, from_pile.end());
    for (int j = 0; j < len; ++j) {
        b.pos[to_pile[to_pile.size() - len + j]].pile = to;
        b.pos[to_pile[to_pile.size() - len + j]].i = to_pile.size() - len + j;
    }

    return cost;
}

/// @return costs
int move_piles(Board& b, int num, int to) {
    auto from = b.pos[num].pile;
    auto i    = b.pos[num].i;
    return move_piles(b, from, i, to);
}

void pop(Board& b, int v) {
    auto& pos = b.pos[v];
    assert(b.head(pos.pile) == v);
    b.piles[pos.pile].pop_back();
    pos.i = -1;
}

struct Result {
    int cost = 0;
    vector<pair<int, int>> commands;

    void move_commmand(Board& b, int v, int to) {
        cost += move_piles(b, v, to);
        commands.emplace_back(v, to);
    }

    void harvest_command(Board& b, int v) {
        pop(b, v);
        commands.emplace_back(v, -1);
    }

    int size() const { return commands.size(); }
    int actual_score() const { return 10000 - cost; }
};

struct State {
    vector<pair<int, int>> order;
    Result result;

    State getCopy() const {
        State ret;
        ret.order = order;
        return ret;
    }
};

Result simulate(piles_type piles, vector<pair<int, int>> order) {
    Board board(piles);
    Result ret;
    int cur = 0;
    for (auto& p : order) {
        while (cur < N && board.head(board.pos[cur].pile) == cur) {
            ret.harvest_command(board, cur);
            cur++;
        }

        if (board.pos[p.first].i != -1) {
            if (board.head(board.pos[p.first].pile) != p.first) {
                int above = board.piles[board.pos[p.first].pile]
                                       [board.pos[p.first].i + 1];
                ret.move_commmand(board, above, p.second);
            }
        }

        while (cur < N && board.head(board.pos[cur].pile) == cur) {
            ret.harvest_command(board, cur);
            cur++;
        }
    }
    if (cur < N) {
        ret.cost = 10000000 - cur;
        ret.commands.clear();
    }

    return ret;
}

State change_pile(State& next) {
    int i                = xorshift::getInt(N);
    int b                = xorshift::getInt(M);
    next.order[i].second = b;
    return next;
}

State rotate_order(State& next) {
    int i        = xorshift::getInt(N - 2);
    int range_bg = i + 2;
    int range_ed = min(i + 5, N);
    int j        = xorshift::getInt(range_ed - range_bg) + range_bg;
    int k        = xorshift::getInt(j - i) + i;
    // rotate to [i,k], [k+1,j] => [k+1,j], [i,k]
    reverse(next.order.begin() + i, next.order.begin() + k + 1);
    reverse(next.order.begin() + k + 1, next.order.begin() + j + 1);
    reverse(next.order.begin() + i, next.order.begin() + j + 1);


    return next;
}

Result solve(piles_type input) {
    // 初期解
    State state;
    state.order.clear();
    for (int i = 0; i < N; ++i) {
        state.order.emplace_back(i, xorshift::getInt(M));
    }
    state.result = simulate(input, state.order);

    for (scheduler::Scheduler<1900> scheduler; scheduler.update();) {
        auto next = state.getCopy();

        {
            double r = xorshift::getDouble();

            if (r < 1) {
                change_pile(next);
            }
            else {
                rotate_order(next);
            }
        }

        next.result = simulate(input, next.order);

        bool ok = false;

        int score     = state.result.actual_score();
        int new_score = next.result.actual_score();

        if (score < 0) {
            ok |= score < new_score;
        }
        else {
            double r = xorshift::getDouble();
            double t = calc_temperature(scheduler.progress, scheduler.limit,
                                        T_START, T_END);
            ok |= r < calc_probability(score, new_score, t);
        }

        if (ok) {
            state = next;
            // DBG("update: " << state.result.actual_score());
        }
    }

    return state.result;
}

int main() {
    auto i   = input();
    auto ret = solve(i);
    output(ret.commands);
    cerr << "cost: " << ret.cost << endl;
    cerr << "score: " << ret.actual_score() << endl;
    return 0;
}
