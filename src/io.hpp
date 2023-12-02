namespace {
#include <iostream>
#include <sstream>
#include <vector>

    using namespace std;

    vector<vector<int>> input() {
        int n, m;
        cin >> n >> m;
        vector<vector<int>> b(n, vector<int>(n / m));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n / m; ++j) {
                cin >> b[i][j];
                b[i][j]--;
            }
        }
        return b;
    }

    void output(vector<pair<int, int>> commands) {
        stringstream ss;
        for (auto& c : commands) {
            ss << c.first + 1 << " " << c.second + 1 << "\n";
        }
        cout << ss.str();
        cout.flush();
    }
} // namespace
