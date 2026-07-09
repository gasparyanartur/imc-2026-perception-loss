#include <bits/stdc++.h>
using namespace std;

struct Mesh {
    vector<array<double, 3>> vertices;
    vector<array<int, 3>> faces;
};

static Mesh read_mesh(const char* path) {
    ifstream in(path);
    if (!in) throw runtime_error(string("cannot open ") + path);
    int nv, nf;
    if (!(in >> nv >> nf) || nv < 0 || nf < 0)
        throw runtime_error("invalid mesh header");
    Mesh mesh;
    mesh.vertices.resize(nv);
    mesh.faces.resize(nf);
    string tag;
    for (auto& v : mesh.vertices) {
        if (!(in >> tag >> v[0] >> v[1] >> v[2]))
            throw runtime_error("invalid vertex record");
    }
    for (auto& f : mesh.faces) {
        if (!(in >> tag >> f[0] >> f[1] >> f[2]))
            throw runtime_error("invalid face record");
        --f[0]; --f[1]; --f[2];
    }
    return mesh;
}

struct Edge {
    int a, b;
    bool operator<(const Edge& other) const {
        return tie(a, b) < tie(other.a, other.b);
    }
};

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    try {
        Mesh original = read_mesh(argv[1]);
        Mesh simplified = read_mesh(argv[2]);
        int bad_indices = 0, degenerate = 0, repeated = 0;
        map<Edge, int> undirected, directed;
        for (const auto& f : simplified.faces) {
            for (int k = 0; k < 3; ++k)
                if (f[k] < 0 || f[k] >= (int)simplified.vertices.size())
                    ++bad_indices;
            if (f[0] == f[1] || f[1] == f[2] || f[0] == f[2]) ++repeated;
            if (bad_indices == 0) {
                auto ab = simplified.vertices[f[1]];
                auto aa = simplified.vertices[f[0]];
                auto ac = simplified.vertices[f[2]];
                array<double, 3> u{ab[0]-aa[0], ab[1]-aa[1], ab[2]-aa[2]};
                array<double, 3> v{ac[0]-aa[0], ac[1]-aa[1], ac[2]-aa[2]};
                array<double, 3> n{u[1]*v[2]-u[2]*v[1],
                                   u[2]*v[0]-u[0]*v[2],
                                   u[0]*v[1]-u[1]*v[0]};
                double area2 = n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
                if (area2 <= 1e-24) ++degenerate;
            }
            Edge e[3] = {{f[0], f[1]}, {f[1], f[2]}, {f[2], f[0]}};
            for (const auto& edge : e) {
                ++directed[edge];
                Edge canonical{min(edge.a, edge.b), max(edge.a, edge.b)};
                ++undirected[canonical];
            }
        }
        int nonmanifold = 0, orientation = 0;
        for (const auto& [edge, count] : undirected) if (count != 2) ++nonmanifold;
        for (const auto& [edge, count] : directed) if (count != 1) ++orientation;
        bool count_ok = !simplified.vertices.empty() &&
                        simplified.vertices.size() <= original.vertices.size();
        bool ok = count_ok && !simplified.faces.empty() && !bad_indices &&
                  !repeated && !degenerate && !nonmanifold && !orientation;
        cout << "VALIDITY=" << (ok ? "VALID" : "INVALID") << '\n';
        cout << "ORIGINAL_VERTICES=" << original.vertices.size() << '\n';
        cout << "SIMPLIFIED_VERTICES=" << simplified.vertices.size() << '\n';
        cout << "SIMPLIFIED_FACES=" << simplified.faces.size() << '\n';
        cout << "BAD_INDICES=" << bad_indices << '\n';
        cout << "REPEATED_FACES=" << repeated << '\n';
        cout << "DEGENERATE_FACES=" << degenerate << '\n';
        cout << "NONMANIFOLD_EDGES=" << nonmanifold << '\n';
        cout << "ORIENTATION_ERRORS=" << orientation << '\n';
        return ok ? 0 : 1;
    } catch (const exception& exc) {
        cerr << "mesh_validity: " << exc.what() << '\n';
        return 2;
    }
}
