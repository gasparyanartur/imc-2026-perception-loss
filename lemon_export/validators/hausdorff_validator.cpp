/* Hausdorff distance validator — samples N points from A, finds closest on B, takes max.
 * Plus reverse. Plus mesh validity checks.
 */
#include <bits/stdc++.h>
using namespace std;

struct Vec3 { double x=0, y=0, z=0; Vec3()=default; Vec3(double x_,double y_,double z_):x(x_),y(y_),z(z_){}
    Vec3 operator+(const Vec3& o)const{return{x+o.x,y+o.y,z+o.z};}
    Vec3 operator-(const Vec3& o)const{return{x-o.x,y-o.y,z-o.z};}
    Vec3 operator*(double s)const{return{x*s,y*s,z*s};}};

static inline double dot(const Vec3& a,const Vec3& b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static inline Vec3 cross(const Vec3& a,const Vec3& b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.z};}
static inline double norm(const Vec3& v){return sqrt(max(0.0,dot(v,v)));}

struct Mesh { vector<Vec3> verts; vector<array<int,3>> faces; };
static Mesh read_mesh(const char* fn){
    FILE* f=fopen(fn,"r");
    if(!f){fprintf(stderr,"Cannot open %s\n",fn);exit(1);}
    vector<char> buf; buf.reserve(1u<<24);
    char chunk[1<<16]; size_t n;
    while((n=fread(chunk,1,sizeof(chunk),f))>0) buf.insert(buf.end(),chunk,chunk+n);
    fclose(f);
    buf.push_back('\0');
    char* p=buf.data();
    int V=(int)strtol(p,&p,10), F=(int)strtol(p,&p,10);
    Mesh m; m.verts.resize(V); m.faces.resize(F);
    for(int i=0;i<V;++i){while(*p&&*p<=' ')++p;++p;m.verts[i].x=strtod(p,&p);m.verts[i].y=strtod(p,&p);m.verts[i].z=strtod(p,&p);}
    for(int i=0;i<F;++i){while(*p&&*p<=' ')++p;++p;m.faces[i]={(int)strtol(p,&p,10)-1,(int)strtol(p,&p,10)-1,(int)strtol(p,&p,10)-1};}
    return m;
}

// Point-triangle closest distance squared
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

// One-way Hausdorff: max over N points of A, min distance to B mesh
static double onewayHausdorff(const Mesh& A, const Mesh& B, int N){
    double worst=0.0;
    int step=max(1,(int)A.verts.size()/N);
    for(int i=0;i<(int)A.verts.size();i+=step){
        const Vec3& p=A.verts[i];
        double best=1e30;
        for(const auto& f: B.faces){
            double d=pointTriDist2(p, B.verts[f[0]], B.verts[f[1]], B.verts[f[2]]);
            if(d<best) best=d;
        }
        if(best>worst) worst=best;
    }
    return sqrt(worst);
}

int main(int argc, char**argv){
    if(argc<3)return 1;
    int N = argc>3 ? atoi(argv[3]) : 500;
    Mesh A=read_mesh(argv[1]);
    Mesh B=read_mesh(argv[2]);
    
    // Compute mesh validity
    int degen_faces=0;
    int bad_idx=0;
    for(const auto& f: A.faces){
        Vec3 n=cross(A.verts[f[1]]-A.verts[f[0]], A.verts[f[2]]-A.verts[f[0]]);
        if(norm(n)<1e-12) degen_faces++;
        for(int k=0;k<3;++k) if(f[k]<0||f[k]>=(int)A.verts.size()) bad_idx++;
    }
    
    // Compute bounds for B
    Vec3 mn=B.verts[0], mx=B.verts[0];
    for(auto&p: B.verts){mn.x=min(mn.x,p.x);mn.y=min(mn.y,p.y);mn.z=min(mn.z,p.z);mx.x=max(mx.x,p.x);mx.y=max(mx.y,p.y);mx.z=max(mx.z,p.z);}
    double diag=sqrt(dot(mx-mn,mx-mn));
    
    // One-way distances (sampled)
    double hAB = onewayHausdorff(A, B, N);
    double hBA = onewayHausdorff(B, A, N);
    double hSym = max(hAB, hBA);
    double limit = 0.05 * diag;
    
    printf("# Hausdorff Analysis (N=%d samples each direction)\n", N);
    printf("# Input verts: A=%zu B=%zu\n", A.verts.size(), B.verts.size());
    printf("# Diagonal: %.6f\n", diag);
    printf("# Hausdorff A→B: %.6f (max sample error)\n", hAB);
    printf("# Hausdorff B→A: %.6f (max sample error)\n", hBA);
    printf("# Hausdorff symmetric: %.6f\n", hSym);
    printf("# Hausdorff limit (5%% diag): %.6f\n", limit);
    printf("# Hausdorff usage: %.2f%% of limit\n", 100.0*hSym/limit);
    printf("# \n");
    printf("# Mesh validity (input A):\n");
    printf("# Total faces: %zu\n", A.faces.size());
    printf("# Degenerate (area too small): %d\n", degen_faces);
    printf("# Bad indices: %d\n", bad_idx);
    
    if (hSym > limit) {
        printf("\nFAIL: Hausdorff exceeds 5%% diagonal!\n");
        return 1;
    }
    if (degen_faces > 0 || bad_idx > 0) {
        printf("\nWARNING: Mesh has validity issues\n");
        return 2;
    }
    printf("\nPASS: Hausdorff OK, mesh valid\n");
    return 0;
}
