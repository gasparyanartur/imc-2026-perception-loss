/* Unified IMC 2026 mesh-simplification diagnostic.
 *
 * One binary reads both meshes once, then reports:
 *   - topology / validity (no thresholds, just counts)
 *   - geometric reduction stats and Hausdorff (uniform-grid accelerated)
 *   - six axial-view normal/depth SSIM renders in parallel
 *   - per-view foreground pixel counts and silhouette IoU
 *
 * Usage:
 *   evaluator ORIGINAL SIMPLIFIED [--render-res=N] [--ss=N]
 *                                [--hausdorff-samples=K] [--grid-resolution=K]
 *                                [--threads=N] [--profile]
 *
 * Output: structured KEY=VALUE lines on stdout. No PASS/FAIL printed;
 * the orchestrator decides based on the metrics.
 */
#include <bits/stdc++.h>
using namespace std;

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3()=default;
    Vec3(double x_,double y_,double z_):x(x_),y(y_),z(z_){}
    double operator[](int i) const { return i==0?x:(i==1?y:z); }
    double& operator[](int i) { return i==0?x:(i==1?y:z); }
    Vec3 operator+(const Vec3& o)const{return{x+o.x,y+o.y,z+o.z};}
    Vec3 operator-(const Vec3& o)const{return{x-o.x,y-o.y,z-o.z};}
    Vec3 operator*(double s)const{return{x*s,y*s,z*s};}
};

static inline double dot(const Vec3& a, const Vec3& b){
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
static inline Vec3 cross(const Vec3& a, const Vec3& b){
    return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
static inline double norm(const Vec3& v){
    return sqrt(max(0.0, dot(v, v)));
}
static inline Vec3 vmin(const Vec3& a, const Vec3& b){
    return Vec3(min(a.x,b.x),min(a.y,b.y),min(a.z,b.z));
}
static inline Vec3 vmax(const Vec3& a, const Vec3& b){
    return Vec3(max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));
}

struct Mesh {
    vector<Vec3> verts;
    vector<array<int,3>> faces;
    Vec3 aabb_min, aabb_max, centroid;
    double aabb_diag=0.0;
    double surface_area=0.0;
    double mean_edge_len=0.0;
    double max_edge_len=0.0;
    double min_tri_area=1e30;
    double max_tri_area=0.0;
    double mean_tri_area=0.0;
    long long nonmanifold_edges=0;
    long long boundary_edges=0;
    long long repeated_face_triples=0;
    long long degenerate_face_count=0;
    long long bad_indices=0;
    long long orientation_errors=0;
    long long sharp_edges_60=0;
    long long euler_chi=0;
    long long genus=-1;
    long long boundary_loops=0;
};

static Mesh read_mesh(const char* fn){
    FILE* f=fopen(fn,"r");
    if(!f){fprintf(stderr,"Cannot open %s\n",fn);exit(1);}
    vector<char> buf; buf.reserve(1u<<24);
    char chunk[1<<16]; size_t n;
    while((n=fread(chunk,1,sizeof(chunk),f))>0)
        buf.insert(buf.end(),chunk,chunk+n);
    fclose(f);
    buf.push_back('\0');
    char* p=buf.data();
    int V=(int)strtol(p,&p,10);
    int F=(int)strtol(p,&p,10);
    Mesh m; m.verts.resize(V); m.faces.resize(F);
    for(int i=0;i<V;++i){
        while(*p&&*p<=' ')++p; ++p;
        m.verts[i].x=strtod(p,&p);
        m.verts[i].y=strtod(p,&p);
        m.verts[i].z=strtod(p,&p);
    }
    for(int i=0;i<F;++i){
        while(*p&&*p<=' ')++p; ++p;
        int a=(int)strtol(p,&p,10)-1;
        int b=(int)strtol(p,&p,10)-1;
        int c=(int)strtol(p,&p,10)-1;
        m.faces[i]={a,b,c};
    }
    double sum_x=0,sum_y=0,sum_z=0;
    m.aabb_min = m.aabb_max = m.verts[0];
    for(const auto& v: m.verts){
        sum_x+=v.x; sum_y+=v.y; sum_z+=v.z;
        m.aabb_min = vmin(m.aabb_min, v);
        m.aabb_max = vmax(m.aabb_max, v);
    }
    if (V>0) m.centroid = Vec3(sum_x/V, sum_y/V, sum_z/V);
    m.aabb_diag = norm(m.aabb_max - m.aabb_min);
    double elen_sum=0; long elen_cnt=0;
    for(const auto& f: m.faces){
        const Vec3& a=m.verts[f[0]];
        const Vec3& b=m.verts[f[1]];
        const Vec3& c=m.verts[f[2]];
        for(int k=0;k<3;++k)
            if (f[k]<0||f[k]>=V) m.bad_indices++;
        Vec3 ab=b-a, ac=c-a;
        Vec3 n=cross(ab,ac);
        double nl=norm(n);
        if (nl<1e-12) m.degenerate_face_count++;
        double area=0.5*nl;
        m.surface_area += area;
        if (area < m.min_tri_area) m.min_tri_area = area;
        if (area > m.max_tri_area) m.max_tri_area = area;
        m.mean_tri_area += area;
        double e1=norm(ab), e2=norm(c-b), e3=norm(a-c);
        elen_sum += e1+e2+e3; elen_cnt += 3;
        m.max_edge_len = max({m.max_edge_len, e1, e2, e3});
    }
    if (m.faces.empty()) m.min_tri_area=0;
    if (!m.faces.empty()) m.mean_tri_area /= (double)m.faces.size();
    if (elen_cnt>0) m.mean_edge_len = elen_sum / elen_cnt;
    return m;
}

static inline uint64_t edge_key(int a, int b){
    int lo=min(a,b), hi=max(a,b);
    return (uint64_t)(uint32_t)lo | ((uint64_t)(uint32_t)hi<<32);
}
static inline uint64_t face_key(int a, int b, int c){
    int v[3]={a,b,c};
    sort(v, v+3);
    return ((uint64_t)(uint32_t)v[0]<<42) ^ ((uint64_t)(uint32_t)v[1]<<21) ^ (uint64_t)(uint32_t)v[2];
}

static void compute_topology(Mesh& m){
    unordered_map<uint64_t, int> undirected;
    undirected.reserve(m.faces.size()*3);
    unordered_map<uint64_t, int> directed;
    directed.reserve(m.faces.size()*3);
    unordered_set<uint64_t> face_set;
    face_set.reserve(m.faces.size()*2);
    long long repeated_faces=0;
    vector<Vec3> face_normals(m.faces.size());
    for(size_t i=0;i<m.faces.size();++i){
        const auto& f=m.faces[i];
        const Vec3& a=m.verts[f[0]];
        const Vec3& b=m.verts[f[1]];
        const Vec3& c=m.verts[f[2]];
        Vec3 n=cross(b-a, c-a);
        double nl=norm(n);
        face_normals[i] = nl>1e-30 ? n*(1.0/nl) : Vec3(0,0,0);
        if (!face_set.insert(face_key(f[0],f[1],f[2])).second) repeated_faces++;
        for(int k=0;k<3;++k){
            int a0=f[k], b0=f[(k+1)%3];
            undirected[edge_key(a0,b0)]++;
            uint64_t dk = ((uint64_t)(uint32_t)a0<<32) | (uint32_t)b0;
            directed[dk]++;
        }
    }
    long long boundary=0, nm=0, orient_err=0;
    for(const auto& kv : undirected){
        long long c = kv.second;
        if (c==2) {/* manifold */}
        else if (c==1){ boundary++; }
        else { nm++; }
    }
    for(const auto& kv : directed){
        if (kv.second!=1) orient_err++;
    }
    m.nonmanifold_edges = nm;
    m.boundary_edges = boundary;
    m.repeated_face_triples = repeated_faces;
    m.orientation_errors = orient_err;

    unordered_map<uint64_t, pair<int,int>> edge_face;
    edge_face.reserve(undirected.size()*2);
    for(size_t i=0;i<m.faces.size();++i){
        const auto& f=m.faces[i];
        for(int k=0;k<3;++k){
            int a0=f[k], b0=f[(k+1)%3];
            uint64_t k0 = edge_key(a0,b0);
            auto it = edge_face.find(k0);
            if (it==edge_face.end()){
                edge_face[k0] = {(int)i, -1};
            } else if (it->second.second==-1){
                it->second.second = (int)i;
            }
        }
    }
    long long sharp=0;
    for(const auto& kv : edge_face){
        const auto& p = kv.second;
        if (p.second<0) continue;
        double d = dot(face_normals[p.first], face_normals[p.second]);
        if (d <= 0.5) sharp++;
    }
    m.sharp_edges_60 = sharp;

    long V = (long)m.verts.size();
    long E = (long)undirected.size();
    long F = (long)m.faces.size();
    long chi = V - E + F;
    m.euler_chi = chi;
    if (boundary==0 && nm==0 && orient_err==0 && F>0){
        m.genus = (2 - chi)/2;
        m.boundary_loops = 0;
    } else {
        m.genus = -1;
        m.boundary_loops = boundary;
    }
}

static double pointTriDist2(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c){
    Vec3 ab=b-a, ac=c-a, ap=p-a;
    double d1=dot(ab,ap), d2=dot(ac,ap);
    if(d1<=0.0&&d2<=0.0) return dot(ap,ap);
    Vec3 bp=p-b;
    double d3=dot(ab,bp), d4=dot(ac,bp);
    if(d3>=0.0&&d4<=d3) return dot(bp,bp);
    double vc=d1*d4-d3*d2;
    if(vc<=0.0&&d1>=0.0&&d3<=0.0){double v=d1/(d1-d3); Vec3 q=a+ab*v; return dot(p-q,p-q);}
    Vec3 cp=p-c;
    double d5=dot(ab,cp), d6=dot(ac,cp);
    if(d6>=0.0&&d5<=d6) return dot(cp,cp);
    double vb=d5*d2-d1*d6;
    if(vb<=0.0&&d2>=0.0&&d6<=0.0){double w=d2/(d2-d6); Vec3 q=a+ac*w; return dot(p-q,p-q);}
    double va=d3*d6-d5*d4;
    if(va<=0.0&&(d4-d3)>=0.0&&(d5-d6)>=0.0){double w=(d4-d3)/((d4-d3)+(d5-d6)); Vec3 q=b+(c-b)*w; return dot(p-q,p-q);}
    Vec3 n=cross(ab,ac);
    double nn=dot(n,n);
    if(nn<1e-24) return 1e30;
    double dist=dot(p-a,n);
    return (dist*dist)/nn;
}

struct TriGrid {
    Vec3 mn, mx, cell_size;
    int nx=0, ny=0, nz=0;
    int total_cells=0;
    long long total_insertions=0;
    vector<vector<int>> cells;

    void build(const Mesh& m, int target_n_per_axis){
        if (m.faces.empty()) return;
        mn = m.aabb_min; mx = m.aabb_max;
        Vec3 pad = (mx-mn)*1e-4 + Vec3(1e-6,1e-6,1e-6);
        mn = mn - pad; mx = mx + pad;
        Vec3 ext = mx - mn;
        int target = max(8, target_n_per_axis);
        double vol = ext.x*ext.y*ext.z;
        double cell_vol = max(1e-12, vol / (double)target);
        double side = cbrt(cell_vol);
        nx = max(2, (int)ceil(ext.x / side));
        ny = max(2, (int)ceil(ext.y / side));
        nz = max(2, (int)ceil(ext.z / side));
        const int AXIS_CAP = 256;
        nx = min(nx, AXIS_CAP);
        ny = min(ny, AXIS_CAP);
        nz = min(nz, AXIS_CAP);
        cell_size = Vec3(ext.x/nx, ext.y/ny, ext.z/nz);
        total_cells = nx*ny*nz;
        cells.assign(total_cells, {});
        for(size_t i=0;i<m.faces.size();++i){
            const Vec3& a=m.verts[m.faces[i][0]];
            const Vec3& b=m.verts[m.faces[i][1]];
            const Vec3& c=m.verts[m.faces[i][2]];
            Vec3 tmn = vmin(vmin(a,b), c);
            Vec3 tmx = vmax(vmax(a,b), c);
            int x0 = max(0, (int)floor((tmn.x - mn.x)/cell_size.x));
            int x1 = min(nx-1, (int)floor((tmx.x - mn.x)/cell_size.x));
            int y0 = max(0, (int)floor((tmn.y - mn.y)/cell_size.y));
            int y1 = min(ny-1, (int)floor((tmx.y - mn.y)/cell_size.y));
            int z0 = max(0, (int)floor((tmn.z - mn.z)/cell_size.z));
            int z1 = min(nz-1, (int)floor((tmx.z - mn.z)/cell_size.z));
            for(int z=z0;z<=z1;++z)
                for(int y=y0;y<=y1;++y)
                    for(int x=x0;x<=x1;++x){
                        int idx = (z*ny + y)*nx + x;
                        cells[idx].push_back((int)i);
                        total_insertions++;
                    }
        }
    }

    double closest_sq(const Mesh& m, const Vec3& p, double init_best) const{
        if (cells.empty() || m.faces.empty()) return init_best;
        double best = init_best;
        int cx = (int)floor((p.x - mn.x)/cell_size.x);
        int cy = (int)floor((p.y - mn.y)/cell_size.y);
        int cz = (int)floor((p.z - mn.z)/cell_size.z);
        if (cx<0||cx>=nx||cy<0||cy>=ny||cz<0||cz>=nz){
            // Point outside grid: brute force fallback (rare)
            for(const auto& fa : m.faces){
                double d2 = pointTriDist2(p, m.verts[fa[0]], m.verts[fa[1]], m.verts[fa[2]]);
                if (d2 < best && d2 < 1e20) best = d2;
            }
            return best;
        }
        const int RING_LIMIT = 6;
        for(int r=0;r<=RING_LIMIT;++r){
            bool ring_useful = false;
            for(int dz=-r;dz<=r;++dz){
                for(int dy=-r;dy<=r;++dy){
                    for(int dx=-r;dx<=r;++dx){
                        if (max(abs(dx),max(abs(dy),abs(dz))) != r) continue;
                        int x=cx+dx, y=cy+dy, z=cz+dz;
                        if (x<0||x>=nx||y<0||y>=ny||z<0||z>=nz) continue;
                        int idx = (z*ny + y)*nx + x;
                        for(int ti : cells[idx]){
                            const Vec3& a=m.verts[m.faces[ti][0]];
                            const Vec3& b=m.verts[m.faces[ti][1]];
                            const Vec3& c=m.verts[m.faces[ti][2]];
                            double d2 = pointTriDist2(p, a, b, c);
                            if (d2 < best) best = d2;
                        }
                        ring_useful = true;
                    }
                }
            }
            double ring_min_aabb = ring_useful
                ? (double)r * min({cell_size.x, cell_size.y, cell_size.z})
                : 0.0;
            if (ring_min_aabb*ring_min_aabb > best) break;
        }
        return best;
    }
};

struct Camera {
    Vec3 eye, right, up, fwd;
};

static Camera make_camera(int view){
    const double D=
2.5;
    Camera cam;
    switch(view){
        case 0: cam.eye=Vec3( D,0,0); break;
        case 1: cam.eye=Vec3(-D,0,0); break;
        case 2: cam.eye=Vec3( 0, D,0); break;
        case 3: cam.eye=Vec3( 0,-D,0); break;
        case 4: cam.eye=Vec3( 0,0, D); break;
        case 5: cam.eye=Vec3( 0,0,-D); break;
    }
    Vec3 f = cam.eye * -1.0;
    double fl = norm(f);
    if (fl<1e-12) f = Vec3(0,0,-1);
    else f = f * (1.0/fl);
    Vec3 wu = fabs(f.z)>0.9 ? Vec3(0,1,0) : Vec3(0,0,1);
    Vec3 r = cross(wu, f);
    double rl = norm(r);
    if (rl<1e-12) r = Vec3(1,0,0);
    else r = r * (1.0/rl);
    Vec3 u = cross(f, r);
    double ul = norm(u);
    if (ul<1e-12) u = Vec3(0,1,0);
    else u = u * (1.0/ul);
    cam.fwd = f; cam.right = r; cam.up = u;
    return cam;
}

static void render_ssaa(const Mesh& m, const Camera& cam, int IMG, int SS,
                        vector<unsigned char>& nrm, vector<unsigned char>& dep){
    int SUP = IMG*SS;
    nrm.assign(IMG*IMG*3, 127);
    dep.assign(IMG*IMG, 255);
    if (m.faces.empty()) return;
    const double FOCAL = 800;
    int V = (int)m.verts.size();
    vector<double> u(V), v(V), z(V);
    for(int i=0;i<V;++i){
        Vec3 r = m.verts[i] - cam.eye;
        z[i] = dot(r, cam.fwd);
        if (z[i] < 1e-8) { u[i]=0; v[i]=0; continue; }
        u[i] = FOCAL * dot(r, cam.right) / z[i] + IMG*0.5;
        v[i] = FOCAL * dot(r, cam.up)    / z[i] + IMG*0.5;
    }
    vector<float> zb(SUP*SUP, 1e20f);
    vector<double> nxa(SUP*SUP, 0), nya(SUP*SUP, 0), nza(SUP*SUP, 0);
    vector<float> invzb(SUP*SUP, 0);
    for(const auto& fa : m.faces){
        int a=fa[0], b=fa[1], c=fa[2];
        if (z[a]<1e-8||z[b]<1e-8||z[c]<1e-8) continue;
        double ua=u[a]*SS, ub=u[b]*SS, uc=u[c]*SS;
        double va=v[a]*SS, vb=v[b]*SS, vc=v[c]*SS;
        int x0=max(0,(int)floor(min({ua,ub,uc})));
        int x1=min(SUP-1,(int)ceil(max({ua,ub,uc})));
        int y0=max(0,(int)floor(min({va,vb,vc})));
        int y1=min(SUP-1,(int)ceil(max({va,vb,vc})));
        if (x0>x1||y0>y1) continue;
        double A=(ub-ua)*(vc-va) - (uc-ua)*(vb-va);
        if (fabs(A)<1e-12) continue;
        double sgn = (A>0) ? 1.0 : -1.0;
        Vec3 nr=cross(m.verts[b]-m.verts[a], m.verts[c]-m.verts[a]);
        double nl=norm(nr); if (nl<1e-30) continue;
        nr = nr * (1.0/nl);
        double invza=1.0/z[a], invzb_=1.0/z[b], invzc=1.0/z[c];
        for(int j=y0;j<=y1;++j){
            double sy=j+0.5;
            for(int i=x0;i<=x1;++i){
                double sx=i+0.5;
                double w0=((ub-uc)*(sy-vc) + (vc-vb)*(sx-uc)) / A * sgn;
                double w1=((uc-ua)*(sy-vc) + (va-vc)*(sx-uc)) / A * sgn;
                double w2=1.0-w0-w1;
                if (w0<0||w1<0||w2<0) continue;
                double iz=w0*invza + w1*invzb_ + w2*invzc;
                if (iz<=0) continue;
                double zz=1.0/iz;
                int idx=j*SUP+i;
                if (zz<zb[idx]){
                    zb[idx]=(float)zz;
                    nxa[idx]=nr.x; nya[idx]=nr.y; nza[idx]=nr.z;
                    invzb[idx]=(float)iz;
                }
            }
        }
    }
    for(int j=0;j<IMG;++j){
        for(int i=0;i<IMG;++i){
            double rx=0, ry=0, rz=0, rd=0;
            int total=0;
            for(int dy=0;dy<SS;++dy) for(int dx=0;dx<SS;++dx){
                int sx=i*SS+dx, sy=j*SS+dy;
                int idx=sy*SUP+sx;
                if (zb[idx]<1e19){
                    rx+=nxa[idx]; ry+=nya[idx]; rz+=nza[idx];
                    rd+=invzb[idx];
                    total++;
                }
            }
            int didx=j*IMG+i;
            if (total>0){
                double len=sqrt(rx*rx+ry*ry+rz*rz);
                if (len<1e-9) len=1;
                rx/=len; ry/=len; rz/=len;
                nrm[didx*3+0]=(unsigned char)max(0,min(255,(int)round((rx+1.0)*127.5)));
                nrm[didx*3+1]=(unsigned char)max(0,min(255,(int)round((ry+1.0)*127.5)));
                nrm[didx*3+2]=(unsigned char)max(0,min(255,(int)round((rz+1.0)*127.5)));
                double dv = rd/total * 255;
                if (dv<0) dv=0; if (dv>255) dv=255;
                dep[didx]=(unsigned char)dv;
            }
        }
    }
}

constexpr int SSIM_W = 11, SSIM_HW = 5;
constexpr double SSIM_C1 = (0.01*255)*(0.01*255);
constexpr double SSIM_C2 = (0.03*255)*(0.03*255);
static double ssim_gw[SSIM_W*SSIM_W];
static void init_ssim_gw(){
    constexpr double sigma = 1.5;
    double sum = 0;
    for(int dy=-SSIM_HW;dy<=SSIM_HW;++dy)
        for(int dx=-SSIM_HW;dx<=SSIM_HW;++dx){
            double r2 = dx*dx+dy*dy;
            int idx = (dy+SSIM_HW)*SSIM_W + (dx+SSIM_HW);
            ssim_gw[idx] = exp(-r2/(2*sigma*sigma));
            sum += ssim_gw[idx];
        }
    for(int i=0;i<SSIM_W*SSIM_W;++i) ssim_gw[i] /= sum;
}

static double ssim_channel(const unsigned char* a, const unsigned char* b,
                           unsigned char bg, int IMG){
    double tot=0; long cnt=0;
    for(int j=SSIM_HW;j<IMG-SSIM_HW;++j){
        for(int i=SSIM_HW;i<IMG-SSIM_HW;++i){
            int ctr = j*IMG + i;
            if (a[ctr]==bg && b[ctr]==bg) continue;
            double mx=0, my=0;
            for(int dy=-SSIM_HW;dy<=SSIM_HW;++dy)
                for(int dx=-SSIM_HW;dx<=SSIM_HW;++dx){
                    int p = (j+dy)*IMG + (i+dx);
                    double w = ssim_gw[(dy+SSIM_HW)*SSIM_W + (dx+SSIM_HW)];
                    mx += w*a[p];
                    my += w*b[p];
                }
            double vx=0, vy=0, cov=0;
            for(int dy=-SSIM_HW;dy<=SSIM_HW;++dy)
                for(int dx=-SSIM_HW;dx<=SSIM_HW;++dx){
                    int p = (j+dy)*IMG + (i+dx);
                    double w = ssim_gw[(dy+SSIM_HW)*SSIM_W + (dx+SSIM_HW)];
                    double da = a[p]-mx, db = b[p]-my;
                    vx += w*da*da;
                    vy += w*db*db;
                    cov += w*da*db;
                }
            double num = (2*mx*my + SSIM_C1) * (2*cov + SSIM_C2);
            double den = (mx*mx + my*my + SSIM_C1) * (vx + vy + SSIM_C2);
            if (den<=0) continue;
            tot += num/den;
            cnt++;
        }
    }
    return cnt>0 ? tot/cnt : 1.0;
}

static double mask_iou(const unsigned char* a_fg, const unsigned char* b_fg,
                       int IMG, unsigned char bg){
    long ai=0, bi=0, inter=0;
    int total = IMG*IMG;
    for(int k=0;k<total;++k){
        bool A = a_fg[k] != bg;
        bool B = b_fg[k] != bg;
        if (A) ai++;
        if (B) bi++;
        if (A && B) inter++;
    }
    long uni = ai + bi - inter;
    return uni>0 ? (double)inter / (double)uni : 1.0;
}

struct HausdorffResult {
    double h_AB = 0.0;
    double h_BA = 0.0;
    double sym  = 0.0;
    long long samples_used = 0;
    int grid_n = 0;
};

static HausdorffResult compute_hausdorff(const Mesh& A, const Mesh& B,
                                         int samples, int grid_resolution){
    HausdorffResult res;
    if (A.faces.empty() || B.faces.empty()) return res;
    TriGrid grid;
    grid.build(B, grid_resolution);
    res.grid_n = grid.total_cells;
    int step = max(1, (int)A.verts.size() / max(1, samples));
    double worst_AB = 0.0;
    long long used = 0;
    for(int i=0;i<(int)A.verts.size(); i += step){
        const Vec3& p = A.verts[i];
        double best_sq = grid.closest_sq(B, p, 1e10);
        if (best_sq > worst_AB && best_sq < 1e20) worst_AB = best_sq;
        used++;
    }
    res.h_AB = sqrt(worst_AB);
    res.samples_used = used;
    step = max(1, (int)B.verts.size() / max(1, samples));
    TriGrid gridA;
    gridA.build(A, grid_resolution);
    double worst_BA = 0.0;
    for(int i=0;i<(int)B.verts.size(); i += step){
        const Vec3& p = B.verts[i];
        double best_sq = gridA.closest_sq(A, p, 1e10);
        if (best_sq > worst_BA && best_sq < 1e20) worst_BA = best_sq;
    }
    res.h_BA = sqrt(worst_BA);
    res.sym = max(res.h_AB, res.h_BA);
    return res;
}

struct PerView {
    double normal_ssim = 0.0;
    double depth_ssim  = 0.0;
    double combined_ssim = 0.0;
    double normal_iou  = 1.0;
    double depth_iou   = 1.0;
    long long n_fg_orig = 0;
    long long n_fg_simp = 0;
};

struct RenderPair {
    vector<unsigned char> orig_nrm, orig_dep, simp_nrm, simp_dep;
    int IMG = 0;
};

static RenderPair render_pair_for_view(const Mesh& A, const Mesh& B,
                                       const Camera& cam, int IMG, int SS){
    RenderPair r; r.IMG = IMG;
    render_ssaa(A, cam, IMG, SS, r.orig_nrm, r.orig_dep);
    render_ssaa(B, cam, IMG, SS, r.simp_nrm, r.simp_dep);
    return r;
}

static PerView score_pair(const RenderPair& r){
    PerView pv;
    int IMG = r.IMG;
    pv.n_fg_orig = 0; pv.n_fg_simp = 0;
    int total = IMG*IMG;
    for(int k=0;k<total;++k){
        if (r.orig_dep[k] != 255) pv.n_fg_orig++;
        if (r.simp_dep[k] != 255) pv.n_fg_simp++;
    }
    pv.normal_iou = mask_iou(r.orig_nrm.data(), r.simp_nrm.data(), IMG, 127);
    pv.depth_iou  = mask_iou(r.orig_dep.data(), r.simp_dep.data(), IMG, 255);
    vector<unsigned char> oR(IMG*IMG), oG(IMG*IMG), oB(IMG*IMG);
    vector<unsigned char> sR(IMG*IMG), sG(IMG*IMG), sB(IMG*IMG);
    for(int k=0;k<IMG*IMG;++k){
        oR[k]=r.orig_nrm[k*3+0]; oG[k]=r.orig_nrm[k*3+1]; oB[k]=r.orig_nrm[k*3+2];
        sR[k]=r.simp_nrm[k*3+0]; sG[k]=r.simp_nrm[k*3+1]; sB[k]=r.simp_nrm[k*3+2];
    }
    double nr = ssim_channel(oR.data(), sR.data(), 127, IMG);
    double ng = ssim_channel(oG.data(), sG.data(), 127, IMG);
    double nb = ssim_channel(oB.data(), sB.data(), 127, IMG);
    pv.normal_ssim = (nr+ng+nb)/3.0;
    pv.depth_ssim  = ssim_channel(r.orig_dep.data(), r.simp_dep.data(), 255, IMG);
    pv.combined_ssim = 0.5 * (pv.normal_ssim + pv.depth_ssim);
    return pv;
}

static pair<int,int> pick_render_params(long max_V, int force_res, int force_ss){
    int res = 1024, ss = 4;
    if (max_V >= 1000000){ res = 192; ss = 1; }
    else if (max_V >= 300000){ res = 256; ss = 2; }
    else if (max_V >= 100000){ res = 384; ss = 2; }
    if (force_res>0) res = force_res;
    if (force_ss>0) ss = force_ss;
    return {res, ss};
}

int main(int argc, char** argv){
    if (argc<3){
        fprintf(stderr, "usage: %s ORIGINAL SIMPLIFIED [options]\n", argv[0]);
        return 2;
    }
    const char* pathA = argv[1];
    const char* pathB = argv[2];
    int force_res=0, force_ss=0;
    int hausdorff_samples = 500;
    int grid_resolution = 96;
    int threads = 0;
    bool profile = false;
    for(int i=3;i<argc;++i){
        string a = argv[i];
        if (a.rfind("--render-res=",0)==0) force_res = atoi(a.c_str()+13);
        else if (a.rfind("--ss=",0)==0) force_ss = atoi(a.c_str()+5);
        else if (a.rfind("--hausdorff-samples=",0)==0)
            hausdorff_samples = atoi(a.c_str()+20);
        else if (a.rfind("--grid-resolution=",0)==0)
            grid_resolution = atoi(a.c_str()+18);
        else if (a.rfind("--threads=",0)==0) threads = atoi(a.c_str()+10);
        else if (a=="--profile") profile = true;
    }
    if (threads<=0) threads = max(1u, min(6u, thread::hardware_concurrency()));
    init_ssim_gw();

    auto t_start = chrono::steady_clock::now();
    Mesh A = read_mesh(pathA);
    Mesh B = read_mesh(pathB);
    auto t_read = chrono::steady_clock::now();
    compute_topology(A);
    compute_topology(B);
    auto t_topo = chrono::steady_clock::now();

    long V_orig = (long)A.verts.size();
    long V_simp = (long)B.verts.size();
    long maxV = max(V_orig, V_simp);
    auto [IMG, SS] = pick_render_params(maxV, force_res, force_ss);

    int haus_used = hausdorff_samples;
    if (maxV > 800000) haus_used = min(hausdorff_samples, 200);
    else if (maxV > 200000) haus_used = min(hausdorff_samples, 300);

    HausdorffResult hr = compute_hausdorff(A, B, haus_used, grid_resolution);
    auto t_haus = chrono::steady_clock::now();

    vector<Camera> cams(6);
    for(int v=0;v<6;++v) cams[v] = make_camera(v);
    vector<PerView> views(6);
    vector<RenderPair> pairs(6);
    {
        auto render_one = [&](int v){
            pairs[v] = render_pair_for_view(A, B, cams[v], IMG, SS);
        };
        vector<thread> ts;
        ts.reserve(min(threads, 6));
        for(int v=0;v<6;++v){
            if ((int)ts.size() >= threads){
                ts.front().join(); ts.erase(ts.begin());
            }
            ts.emplace_back(render_one, v);
        }
        for(auto& t : ts) t.join();
    }
    auto t_render = chrono::steady_clock::now();

    for(int v=0;v<6;++v) views[v] = score_pair(pairs[v]);
    auto t_ssim = chrono::steady_clock::now();

    double normal_avg=0, depth_avg=0;
    for(int v=0;v<6;++v){
        normal_avg += views[v].normal_ssim;
        depth_avg  += views[v].depth_ssim;
    }
    normal_avg /= 6.0;
    depth_avg  /= 6.0;
    double final_ssim = 0.5 * (normal_avg + depth_avg);
    double limit_5pct = 0.05 * A.aabb_diag;
    double usage_pct = (limit_5pct>0) ? 100.0 * hr.sym / limit_5pct : 0.0;
    long Vdelta = V_orig - V_simp;
    double reduction_pct = (V_orig>0) ? 100.0 * (double)Vdelta / (double)V_orig : 0.0;
    long Fdelta = (long)A.faces.size() - (long)B.faces.size();
    double freduction_pct = (A.faces.size()>0)
        ? 100.0 * (double)Fdelta / (double)A.faces.size() : 0.0;

    printf("# Unified IMC 2026 evaluator\n");
    printf("ORIGINAL_VERTICES=%ld\n", V_orig);
    printf("SIMPLIFIED_VERTICES=%ld\n", V_simp);
    printf("ORIGINAL_FACES=%zu\n", A.faces.size());
    printf("SIMPLIFIED_FACES=%zu\n", B.faces.size());
    printf("VERTEX_REDUCTION_PCT=%.6f\n", reduction_pct);
    printf("FACE_REDUCTION_PCT=%.6f\n", freduction_pct);
    printf("AABB_DIAGONAL=%.6f\n", A.aabb_diag);
    printf("SURFACE_AREA_ORIG=%.6f\n", A.surface_area);
    printf("SURFACE_AREA_SIMP=%.6f\n", B.surface_area);
    printf("BAD_INDICES_SIMP=%lld\n", B.bad_indices);
    printf("REPEATED_FACES_SIMP=%lld\n", B.repeated_face_triples);
    printf("DEGENERATE_FACES_SIMP=%lld\n", B.degenerate_face_count);
    printf("NONMANIFOLD_EDGES_SIMP=%lld\n", B.nonmanifold_edges);
    printf("BOUNDARY_EDGES_SIMP=%lld\n", B.boundary_edges);
    printf("ORIENTATION_ERRORS_SIMP=%lld\n", B.orientation_errors);
    printf("EULER_CHI_SIMP=%lld\n", B.euler_chi);
    printf("GENUS_SIMP=%lld\n", B.genus);
    printf("SHARP_EDGES_60_SIMP=%lld\n", B.sharp_edges_60);
    printf("MEAN_EDGE_LEN_SIMP=%.6f\n", B.mean_edge_len);
    printf("MAX_EDGE_LEN_SIMP=%.6f\n", B.max_edge_len);
    printf("MIN_TRI_AREA_SIMP=%.9f\n", B.min_tri_area);
    printf("MAX_TRI_AREA_SIMP=%.6f\n", B.max_tri_area);
    printf("MEAN_TRI_AREA_SIMP=%.6f\n", B.mean_tri_area);
    printf("HAUSDORFF_LIMIT_5PCT=%.6f\n", limit_5pct);
    printf("HAUSDORFF_AB=%.6f\n", hr.h_AB);
    printf("HAUSDORFF_BA=%.6f\n", hr.h_BA);
    printf("HAUSDORFF_SYM=%.6f\n", hr.sym);
    printf("HAUSDORFF_USAGE_PCT=%.4f\n", usage_pct);
    printf("HAUSDORFF_SAMPLES=%lld\n", hr.samples_used);
    printf("HAUSDORFF_GRID_CELLS=%d\n", hr.grid_n);
    printf("RENDER_RES=%d\n", IMG);
    printf("RENDER_SS=%d\n", SS);
    printf("RENDER_THREADS=%d\n", threads);
    printf("NORMAL_SSIM=%.6f\n", normal_avg);
    printf("DEPTH_SSIM=%.6f\n", depth_avg);
    printf("FINAL_SSIM=%.6f\n", final_ssim);

    long long fg_orig_total=0, fg_simp_total=0;
    double iou_n_sum=0, iou_d_sum=0;
    for(int v=0;v<6;++v){
        const PerView& pv = views[v];
        printf("V%d_NORMAL_SSIM=%.6f V%d_DEPTH_SSIM=%.6f V%d_COMBINED_SSIM=%.6f "
               "V%d_NORM_IOU=%.6f V%d_DEPTH_IOU=%.6f "
               "V%d_FG_ORIG=%lld V%d_FG_SIMP=%lld\n",
               v, pv.normal_ssim,
               v, pv.depth_ssim,
               v, pv.combined_ssim,
               v, pv.normal_iou,
               v, pv.depth_iou,
               v, pv.n_fg_orig,
               v, pv.n_fg_simp);
        fg_orig_total += pv.n_fg_orig;
        fg_simp_total += pv.n_fg_simp;
        iou_n_sum += pv.normal_iou;
        iou_d_sum += pv.depth_iou;
    }
    printf("FG_PIXELS_ORIG_SUM=%lld\n", fg_orig_total);
    printf("FG_PIXELS_SIMP_SUM=%lld\n", fg_simp_total);
    printf("NORMAL_IOU_AVG=%.6f\n", iou_n_sum/6.0);
    printf("DEPTH_IOU_AVG=%.6f\n", iou_d_sum/6.0);

    if (profile){
        auto us = [](chrono::steady_clock::time_point a,
                     chrono::steady_clock::time_point b){
            return chrono::duration_cast<chrono::microseconds>(b-a).count();
        };
        printf("PROFILE_READ_US=%lld\n", (long long)us(t_start, t_read));
        printf("PROFILE_TOPO_US=%lld\n", (long long)us(t_read, t_topo));
        printf("PROFILE_HAUSDORFF_US=%lld\n", (long long)us(t_topo, t_haus));
        printf("PROFILE_RENDER_US=%lld\n", (long long)us(t_haus, t_render));
        printf("PROFILE_SSIM_US=%lld\n", (long long)us(t_render, t_ssim));
    }
    return 0;
}
