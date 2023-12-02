namespace {
#include <iostream>
#include <sstream>
#include <vector>

    using namespace std;

    constexpr int dr[] = {-1, 1, 0, 0};
    constexpr int dc[] = {0, 0, -1, 1};
    const string ds    = "UDLR";
    constexpr int UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3;

    struct Cell {
        int r;
        int c;
        int id;
    };

    /// @return N
    int input(vector<vector<int>>& edges, int d[], Cell c[]) {
        int n;
        cin >> n;

        // init c
        for (int i = 0; i < n * n; ++i) {
            c[i].id = i;
            c[i].r  = i / n;
            c[i].c  = i % n;
        }

        vector<vector<int>> g(n * n, vector<int>(4, 0));
        // input outside walls
        for (int i = 0; i < n * n; ++i) {
            if (c[i].r == 0) {
                g[i][UP] = 1;
            }
            if (c[i].r == n - 1) {
                g[i][DOWN] = 1;
            }
            if (c[i].c == 0) {
                g[i][LEFT] = 1;
            }
            if (c[i].c == n - 1) {
                g[i][RIGHT] = 1;
            }
        }

        // input horizontal walls
        for (int i = 0; i < n - 1; ++i) {
            string s;
            cin >> s;
            assert((int)s.size() == n);
            for (int j = 0; j < n; ++j) {
                const int id = i * n + j;
                if (s[j] == '1') {
                    g[id][DOWN]   = 1;
                    g[id + n][UP] = 1;
                }
            }
        }
        // input vertical walls
        for (int i = 0; i < n; ++i) {
            string s;
            cin >> s;
            assert((int)s.size() == n - 1);
            for (int j = 0; j < n - 1; ++j) {
                const int id = i * n + j;
                if (s[j] == '1') {
                    g[id][RIGHT]    = 1;
                    g[id + 1][LEFT] = 1;
                }
            }
        }
        // g を元に edge を作る
        edges.clear();
        edges.resize(n * n);
        for (int i = 0; i < n * n; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (g[i][j] == 0) {
                    edges[i].push_back(i + dr[j] * n + dc[j]);
                    assert(edges[i].back() >= 0 && edges[i].back() < n * n);
                }
            }
        }

        // input d
        for (int i = 0; i < n * n; ++i) {
            cin >> d[i];
        }

        return n;
    }

    template <typename T>
    void output(vector<T> seq, Cell* c) {
        stringstream ss;
        const int L = seq.size();
        for (int i = 0; i + 1 < L; ++i) {
            const auto cur = c[seq[i]];
            const auto nxt = c[seq[i + 1]];

            int d = -1;
            for (int j = 0; j < 4; ++j) {
                if (cur.r + dr[j] == nxt.r && cur.c + dc[j] == nxt.c) {
                    d = j;
                    break;
                }
            }
            assert(d != -1);
            ss << ds[d];
        }
        cout << ss.str() << endl;
    }
} // namespace
