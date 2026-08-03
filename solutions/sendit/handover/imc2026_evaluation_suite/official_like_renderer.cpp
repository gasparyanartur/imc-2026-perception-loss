#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

static constexpr int VIEWS = 6;
static constexpr double CAM_D = 2.5;
static constexpr double FOCAL_1024 = 800.0;
static constexpr double BG_NORMAL = 127.5;
static constexpr double BG_DEPTH = 255.0;
static constexpr int SSIM_RAD = 5;
static constexpr int SSIM_WIN = 11;
static constexpr double C1 = (0.01 * 255.0) * (0.01 * 255.0);
static constexpr double C2 = (0.03 * 255.0) * (0.03 * 255.0);
static constexpr double EPS = 1e-15;

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3() = default;
    Vec3(double X,double Y,double Z):x(X),y(Y),z(Z){}
    Vec3 operator+(const Vec3&o)const{return {x+o.x,y+o.y,z+o.z};}
    Vec3 operator-(const Vec3&o)const{return {x-o.x,y-o.y,z-o.z};}
    Vec3 operator*(double s)const{return {x*s,y*s,z*s};}
    Vec3 operator/(double s)const{return {x/s,y/s,z/s};}
};
static double dot(const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 cross(const Vec3&a,const Vec3&b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static double norm(const Vec3&a){return sqrt(dot(a,a));}
static Vec3 unit(const Vec3&a){double n=norm(a);return n>EPS?a/n:Vec3{};}

struct Face{int v[3];};
struct Mesh{vector<Vec3> v; vector<Face> f;};

static Mesh readMesh(const string& path){
    ifstream in(path);
    if(!in) throw runtime_error("cannot open mesh: "+path);
    int nv,nf; in>>nv>>nf;
    if(!in || nv<0 || nf<0) throw runtime_error("bad mesh header: "+path);
    Mesh m; m.v.resize(nv); m.f.resize(nf);
    char c;
    for(int i=0;i<nv;++i){in>>c>>m.v[i].x>>m.v[i].y>>m.v[i].z; if(c!='v'||!in) throw runtime_error("bad vertex");}
    for(int i=0;i<nf;++i){in>>c>>m.f[i].v[0]>>m.f[i].v[1]>>m.f[i].v[2]; if(c!='f'||!in) throw runtime_error("bad face"); for(int k=0;k<3;++k)--m.f[i].v[k];}
    return m;
}

struct Pixel{
    float n[3];
    float d;
    unsigned char fg;
};
struct Proj{double u=0,v=0,z=0; bool ok=false;};

static const array<Vec3,VIEWS> dirs = {Vec3{1,0,0},Vec3{-1,0,0},Vec3{0,1,0},Vec3{0,-1,0},Vec3{0,0,1},Vec3{0,0,-1}};

static void frame(int view, Vec3& eye, Vec3& right, Vec3& up, Vec3& fwd){
    eye=dirs[view]*CAM_D;
    fwd=unit(eye*(-1.0));
    Vec3 wu=(fabs(dot(fwd,Vec3{0,0,1}))>0.90)?Vec3{0,1,0}:Vec3{0,0,1};
    right=unit(cross(wu,fwd));
    if(norm(right)<=EPS) right={1,0,0};
    up=unit(cross(fwd,right));
}
static Proj project(const Vec3&p,int view,int R){
    Vec3 e,r,u,f; frame(view,e,r,u,f);
    Vec3 q=p-e; double z=dot(q,f);
    if(z<=1e-8) return {};
    double scale=double(R)/1024.0;
    return {FOCAL_1024*scale*dot(q,r)/z+0.5*R,
            FOCAL_1024*scale*dot(q,u)/z+0.5*R,z,true};
}

static vector<Pixel> render(const Mesh&m,int view,int R){
    Pixel bg{{(float)BG_NORMAL,(float)BG_NORMAL,(float)BG_NORMAL},(float)BG_DEPTH,0};
    vector<Pixel> buf((size_t)R*R,bg);
    vector<float> zbuf((size_t)R*R,numeric_limits<float>::infinity());
    for(const Face&f:m.f){
        if(f.v[0]<0||f.v[1]<0||f.v[2]<0||f.v[0]>=(int)m.v.size()||f.v[1]>=(int)m.v.size()||f.v[2]>=(int)m.v.size()) continue;
        const Vec3&a3=m.v[f.v[0]],&b3=m.v[f.v[1]],&c3=m.v[f.v[2]];
        Proj a=project(a3,view,R),b=project(b3,view,R),c=project(c3,view,R);
        if(!a.ok||!b.ok||!c.ok) continue;
        Vec3 nr=cross(b3-a3,c3-a3); double nl=norm(nr); if(nl<1e-12) continue; nr=nr/nl;
        double den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);
        if(fabs(den)<1e-18) continue;
        int x0=max(0,(int)floor(min({a.u,b.u,c.u}))), x1=min(R-1,(int)ceil(max({a.u,b.u,c.u})));
        int y0=max(0,(int)floor(min({a.v,b.v,c.v}))), y1=min(R-1,(int)ceil(max({a.v,b.v,c.v})));
        if(x0>x1||y0>y1) continue;
        for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x){
            double X=x+0.5,Y=y+0.5;
            double w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den;
            double w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den;
            double w2=1.0-w0-w1;
            if(w0<-1e-9||w1<-1e-9||w2<-1e-9) continue;
            double iz=w0/a.z+w1/b.z+w2/c.z; if(iz<=0) continue;
            float zz=(float)(1.0/iz); size_t q=(size_t)y*R+x;
            if(zz<zbuf[q]){
                zbuf[q]=zz;
                buf[q].n[0]=(float)((nr.x+1.0)*127.5);
                buf[q].n[1]=(float)((nr.y+1.0)*127.5);
                buf[q].n[2]=(float)((nr.z+1.0)*127.5);
                buf[q].d=zz; buf[q].fg=1;
            }
        }
    }
    return buf;
}

static double channel(const Pixel&p,int ch){return ch<3?p.n[ch]:p.d;}
static double windowSsim(const vector<Pixel>&a,const vector<Pixel>&b,int ch,int R){
    int S=R+1; size_t N=(size_t)S*S;
    vector<double> ix(N),iy(N),ix2(N),iy2(N),ixy(N);
    for(int y=0;y<R;++y){
        double sx=0,sy=0,sxx=0,syy=0,sxy=0;
        size_t prev=(size_t)y*S,row=(size_t)(y+1)*S;
        for(int x=0;x<R;++x){
            size_t p=(size_t)y*R+x; double X=channel(a[p],ch),Y=channel(b[p],ch);
            sx+=X;sy+=Y;sxx+=X*X;syy+=Y*Y;sxy+=X*Y;
            size_t q=row+x+1,u=prev+x+1;
            ix[q]=ix[u]+sx; iy[q]=iy[u]+sy; ix2[q]=ix2[u]+sxx; iy2[q]=iy2[u]+syy; ixy[q]=ixy[u]+sxy;
        }
    }
    auto rect=[&](const vector<double>&I,int x0,int y0,int x1,int y1){
        size_t A=(size_t)y0*S+x0,B=(size_t)y0*S+x1,C=(size_t)y1*S+x0,D=(size_t)y1*S+x1;
        return I[D]-I[B]-I[C]+I[A];
    };
    double total=0; long long count=0; const double area=SSIM_WIN*SSIM_WIN;
    for(int y=SSIM_RAD;y<R-SSIM_RAD;++y) for(int x=SSIM_RAD;x<R-SSIM_RAD;++x){
        size_t p=(size_t)y*R+x; if(!a[p].fg&&!b[p].fg) continue;
        int x0=x-SSIM_RAD,y0=y-SSIM_RAD,x1=x+SSIM_RAD+1,y1=y+SSIM_RAD+1;
        double sx=rect(ix,x0,y0,x1,y1),sy=rect(iy,x0,y0,x1,y1);
        double sxx=rect(ix2,x0,y0,x1,y1),syy=rect(iy2,x0,y0,x1,y1),sxy=rect(ixy,x0,y0,x1,y1);
        double mx=sx/area,my=sy/area;
        double vx=max(0.0,sxx/area-mx*mx),vy=max(0.0,syy/area-my*my),cov=sxy/area-mx*my;
        double num=(2*mx*my+C1)*(2*cov+C2);
        double den=(mx*mx+my*my+C1)*(vx+vy+C2);
        total+=den!=0?num/den:1.0; ++count;
    }
    return count?total/count:1.0;
}

static unsigned char toByte(double x){return (unsigned char)max(0,min(255,(int)lround(x)));}
static void writePPM(const string&path,const vector<Pixel>&b,int R){
    ofstream o(path,ios::binary); o<<"P6\n"<<R<<" "<<R<<"\n255\n";
    for(const Pixel&p:b){unsigned char rgb[3]={toByte(p.n[0]),toByte(p.n[1]),toByte(p.n[2])};o.write((char*)rgb,3);} }
static void writeDepthPGM(const string&path,const vector<Pixel>&b,int R){
    double mn=numeric_limits<double>::infinity(),mx=-numeric_limits<double>::infinity();
    for(const Pixel&p:b)if(p.fg){mn=min(mn,(double)p.d);mx=max(mx,(double)p.d);} if(!isfinite(mn)||mx<=mn){mn=0;mx=1;}
    ofstream o(path,ios::binary);o<<"P5\n"<<R<<" "<<R<<"\n255\n";
    for(const Pixel&p:b){unsigned char v=255;if(p.fg)v=toByte(255.0*(p.d-mn)/(mx-mn));o.write((char*)&v,1);} }

int main(int argc,char**argv){
    if(argc<5){cerr<<"usage: official_like_renderer ORIGINAL CANDIDATE OUT_DIR RESOLUTION\n";return 2;}
    try{
        string origPath=argv[1],candPath=argv[2],outDir=argv[3]; int R=atoi(argv[4]); if(R<32) throw runtime_error("resolution too small");
        Mesh orig=readMesh(origPath),cand=readMesh(candPath);
        string mkdirCmd="mkdir -p \""+outDir+"\""; if(system(mkdirCmd.c_str())!=0) throw runtime_error("mkdir failed");
        double mean=0,minView=1,minNormal=1,minDepth=1;
        array<double,VIEWS> vscore{},nscore{},dscore{};
        for(int view=0;view<VIEWS;++view){
            auto a=render(orig,view,R),b=render(cand,view,R);
            double sn=(windowSsim(a,b,0,R)+windowSsim(a,b,1,R)+windowSsim(a,b,2,R))/3.0;
            double sd=windowSsim(a,b,3,R),sv=0.5*(sn+sd);
            nscore[view]=sn;dscore[view]=sd;vscore[view]=sv;mean+=sv;minView=min(minView,sv);minNormal=min(minNormal,sn);minDepth=min(minDepth,sd);
            writePPM(outDir+"/original_view"+to_string(view)+"_normal.ppm",a,R);
            writePPM(outDir+"/candidate_view"+to_string(view)+"_normal.ppm",b,R);
            writeDepthPGM(outDir+"/original_view"+to_string(view)+"_depth.pgm",a,R);
            writeDepthPGM(outDir+"/candidate_view"+to_string(view)+"_depth.pgm",b,R);
        }
        mean/=VIEWS;
        ofstream j(outDir+"/render_metrics.json"); j<<fixed<<setprecision(12);
        j<<"{\n  \"resolution\": "<<R<<",\n  \"mean_ssim\": "<<mean<<",\n  \"min_view_ssim\": "<<minView<<",\n  \"min_normal_ssim\": "<<minNormal<<",\n  \"min_depth_ssim\": "<<minDepth<<",\n  \"views\": [\n";
        for(int i=0;i<VIEWS;++i){j<<"    {\"view\": "<<i<<", \"normal_ssim\": "<<nscore[i]<<", \"depth_ssim\": "<<dscore[i]<<", \"mean_ssim\": "<<vscore[i]<<"}"<<(i+1<VIEWS?",":"")<<"\n";}
        j<<"  ]\n}\n";
        cout<<fixed<<setprecision(9)<<"mean="<<mean<<" minView="<<minView<<" minNormal="<<minNormal<<" minDepth="<<minDepth<<"\n";
        return 0;
    }catch(const exception&e){cerr<<"renderer error: "<<e.what()<<"\n";return 1;}
}
