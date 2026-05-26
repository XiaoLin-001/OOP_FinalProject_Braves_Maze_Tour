#include "MazeGenerator.h"

#include <cstdlib>
#include <utility>

namespace MazeGen {

struct UnionFind {
    std::vector<int> parent;
    std::vector<int> rank;

    UnionFind(int n) : parent(n), rank(n, 0) {
        for (int i = 0; i < n; ++i) parent[i] = i;
    }

    int find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }

    bool unite(int a, int b) {
        int ra = find(a), rb = find(b);
        if (ra == rb) return false;
        if (rank[ra] < rank[rb]) std::swap(ra, rb);
        parent[rb] = ra;
        if (rank[ra] == rank[rb]) rank[ra]++;
        return true;
    }
};

template <typename T>
static void shuffleVec(std::vector<T>& v) {
    for (size_t i = v.size(); i > 1; --i) {
        size_t j = rand() % i;
        std::swap(v[i - 1], v[j]);
    }
}

std::vector<std::vector<int>> generateDFS(int n) {
    std::vector<std::vector<int>> g(n, std::vector<int>(n, 1));
    int m = (n - 1) / 2;

    std::vector<std::vector<bool>> visited(m, std::vector<bool>(m, false));
    std::vector<std::pair<int, int>> stack;

    stack.push_back(std::make_pair(0, 0));
    visited[0][0] = true;
    g[1][1] = 0;

    const int dr[4] = { -1, 1, 0, 0 };
    const int dc[4] = { 0, 0, -1, 1 };

    while (!stack.empty()) {
        int rr = stack.back().first;
        int cc = stack.back().second;

        int order[4] = { 0, 1, 2, 3 };
        for (int i = 3; i > 0; --i) { int j = rand() % (i + 1); std::swap(order[i], order[j]); }

        bool advanced = false;
        for (int k = 0; k < 4; ++k) {
            int d = order[k];
            int nr = rr + dr[d], nc = cc + dc[d];
            if (nr < 0 || nr >= m || nc < 0 || nc >= m || visited[nr][nc])
                continue;

            int gr = 2 * rr + 1, gc = 2 * cc + 1;
            int ngr = 2 * nr + 1, ngc = 2 * nc + 1;
            g[(gr + ngr) / 2][(gc + ngc) / 2] = 0;
            g[ngr][ngc] = 0;

            visited[nr][nc] = true;
            stack.push_back(std::make_pair(nr, nc));
            advanced = true;
            break;
        }
        if (!advanced) stack.pop_back();
    }

    g[n - 2][n - 2] = 2;
    return g;
}

std::vector<std::vector<int>> generateKruskal(int n) {
    std::vector<std::vector<int>> g(n, std::vector<int>(n, 1));
    int m = (n - 1) / 2;

    for (int rr = 0; rr < m; ++rr)
        for (int cc = 0; cc < m; ++cc)
            g[2 * rr + 1][2 * cc + 1] = 0;

    struct Edge { int r, c, d; };
    std::vector<Edge> edges;
    for (int rr = 0; rr < m; ++rr) {
        for (int cc = 0; cc < m; ++cc) {
            if (cc + 1 < m) { Edge e = { rr, cc, 0 }; edges.push_back(e); }
            if (rr + 1 < m) { Edge e = { rr, cc, 1 }; edges.push_back(e); }
        }
    }
    shuffleVec(edges);

    UnionFind uf(m * m);
    for (size_t i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];
        int nr = e.r + (e.d == 1 ? 1 : 0);
        int nc = e.c + (e.d == 0 ? 1 : 0);
        int a = e.r * m + e.c;
        int b = nr * m + nc;

        if (uf.unite(a, b)) {
            int gr = 2 * e.r + 1, gc = 2 * e.c + 1;
            int ngr = 2 * nr + 1, ngc = 2 * nc + 1;
            g[(gr + ngr) / 2][(gc + ngc) / 2] = 0;
        }
    }

    g[n - 2][n - 2] = 2;
    return g;
}

}
