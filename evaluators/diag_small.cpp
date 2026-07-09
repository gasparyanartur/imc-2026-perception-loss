/* Fixed SSAA validator with sign-aware barycentric */
#include <bits/stdc++.h>
using namespace std;

struct Vec3 { double x=0, y=0, z=0; Vec3()=default; Vec3(double x_,double y_,double z_):x(x_),y(y_),z(z_){}
    Vec3 operator+(const Vec3& o)const{return{x+o.x,y+o.y,z+o.z};}
    Vec3 operator-(const Vec3& o)const{return{x-o.x,y-o.y,z-o.z};}
    Vec3 operator*(double s)const{return{x*s,y*s,z*s};}};
static inline double dot(const Vec3& a, const Vec3& b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static inline Vec3 cross(const Vec3& a, const Vec3& b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static inline double norm(const Vec3& v){return sqrt(max(0.0,dot(v,v)));}

struct Mesh{ vector<Vec3> verts; vector<array<int,3>> faces; };
static Mesh read_mesh(const char* fn){
    FILE* f=fopen(fn,"r");
    if(!f){fprintf(stderr,"Cannot open %s\n",fn);exit(1);}
    vector<char> buf; buf.reserve(1u<<24);
    char chunk[1<<16]; size_t n;
    while((n=fread(chunk,1,sizeof(chunk),f))>0) buf.insert(buf.end(),chunk,chunk+n);
    fclose(f);
    buf.push_back('\0');
    char* p=buf.data();
    int V=(int)strtol(p,&p,10);
    int F=(int)strtol(p,&p,10);
    Mesh m; m.verts.resize(V); m.faces.resize(F);
    for(int i=0;i<V;++i){while(*p&&*p<=' ')++p;++p;m.verts[i].x=strtod(p,&p);m.verts[i].y=strtod(p,&p);m.verts[i].z=strtod(p,&p);}
    for(int i=0;i<F;++i){while(*p&&*p<=' ')++p;++p;m.faces[i]={(int)strtol(p,&p,10)-1,(int)strtol(p,&p,10)-1,(int)strtol(p,&p,10)-1};}
    return m;
}

static void render_ssaa(const Mesh& m, int view, int IMG, int SS, 
                        vector<unsigned char>& nrm, vector<unsigned char>& dep){
    int SUP=IMG*SS;
    nrm.assign(IMG*IMG*3, 127);
    dep.assign(IMG*IMG, 255);
    if (m.faces.empty()) return;
    const double FOCAL=800, D=2.5;
    Vec3 eye, right, up, fwd;
    switch(view){
        case 0: eye=Vec3(D,0,0); break;
        case 1: eye=Vec3(-D,0,0); break;
        case 2: eye=Vec3(0,D,0); break;
        case 3: eye=Vec3(0,-D,0); break;
        case 4: eye=Vec3(0,0,D); break;
        case 5: eye=Vec3(0,0,-D); break;
    }
    fwd = eye * -1.0; double fl = norm(fwd); if (fl<1e-12) fwd=Vec3(0,0,-1); else fwd=fwd*(1.0/fl);
    Vec3 wu = fabs(fwd.z)>0.9?Vec3(0,1,0):Vec3(0,0,1);
    right = cross(wu, fwd); double rl=norm(right); if(rl<1e-12) right=Vec3(1,0,0); else right=right*(1.0/rl);
    up = cross(fwd, right); double ul=norm(up); if(ul<1e-12) up=Vec3(0,1,0); else up=up*(1.0/ul);
    vector<double> u(m.verts.size()), v(m.verts.size()), z(m.verts.size());
    for(size_t i=0;i<m.verts.size();++i){
        Vec3 r=m.verts[i]-eye;
        z[i]=dot(r,fwd);
        if (z[i]<1e-8) continue;
        u[i]=FOCAL*dot(r,right)/z[i]+IMG*0.5;
        v[i]=FOCAL*dot(r,up)/z[i]+IMG*0.5;
    }
    vector<float> zb(SUP*SUP, 1e20f);
    vector<double> nxa(SUP*SUP, 0), nya(SUP*SUP, 0), nza(SUP*SUP, 0);
    vector<float> invzb(SUP*SUP, 0);
    vector<int> cnt(SUP*SUP, 0);
    for(auto& fa : m.faces){
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
        nr=nr*(1.0/nl);
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
                    cnt[idx]=1;
                }
            }
        }
    }
    // Downsample
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
                nrm[didx*3]=(unsigned char)max(0,min(255,(int)round((rx+1.0)*127.5)));
                nrm[didx*3+1]=(unsigned char)max(0,min(255,(int)round((ry+1.0)*127.5)));
                nrm[didx*3+2]=(unsigned char)max(0,min(255,(int)round((rz+1.0)*127.5)));
                double dv = rd/total * 255;  // 1/z * 255
                if (dv<0) dv=0; if (dv>255) dv=255;
                dep[didx]=(unsigned char)dv;
            }
        }
    }
}

constexpr int W=11, WH=5;
constexpr double C1=(0.01*255)*(0.01*255);
constexpr double C2=(0.03*255)*(0.03*255);
double gw[W*W];
void init_gw(){
    constexpr double sigma=1.5;
    double sum=0;
    for(int dy=-WH;dy<=WH;++dy) for(int dx=-WH;dx<=WH;++dx){
        double r2=dx*dx+dy*dy;
        int idx=(dy+WH)*W+(dx+WH);
        gw[idx]=exp(-r2/(2*sigma*sigma));
        sum+=gw[idx];
    }
    for(int i=0;i<W*W;++i) gw[i]/=sum;
}
double ssim_ch(const vector<unsigned char>& a, const vector<unsigned char>& b, unsigned char bg, int IMG){
    double tot=0; int cnt=0;
    for(int j=WH;j<IMG-WH;++j) for(int i=WH;i<IMG-WH;++i){
        int ctr=j*IMG+i;
        if(a[ctr]==bg && b[ctr]==bg) continue;
        double mx=0,my=0;
        for(int dy=-WH;dy<=WH;++dy) for(int dx=-WH;dx<=WH;++dx){
            int p=(j+dy)*IMG+(i+dx);
            double w=gw[(dy+WH)*W+(dx+WH)];
            mx+=w*a[p]; my+=w*b[p];
        }
        double vx=0,vy=0,cov=0;
        for(int dy=-WH;dy<=WH;++dy) for(int dx=-WH;dx<=WH;++dx){
            int p=(j+dy)*IMG+(i+dx);
            double w=gw[(dy+WH)*W+(dx+WH)];
            double da=a[p]-mx, db=b[p]-my;
            vx+=w*da*da; vy+=w*db*db; cov+=w*da*db;
        }
        double num=(2*mx*my+C1)*(2*cov+C2);
        double den=(mx*mx+my*my+C1)*(vx+vy+C2);
        if(den<=0) continue;
        tot += num/den;
        cnt++;
    }
    return cnt>0 ? tot/cnt : 1.0;
}

int main(int argc, char**argv){
    if(argc<3)return 1;
    init_gw();
    int IMG=256, SS=2;
    if (argc>3) SS = atoi(argv[3]);
    Mesh A=read_mesh(argv[1]);
    Mesh B=read_mesh(argv[2]);
    
    vector<unsigned char> na, da, nb, db;
    double ssim_n=0, ssim_d=0;
    vector<unsigned char> oR(IMG*IMG), oG(IMG*IMG), oB(IMG*IMG);
    vector<unsigned char> sR(IMG*IMG), sG(IMG*IMG), sB(IMG*IMG);
    for(int v=0;v<6;++v){
        render_ssaa(A, v, IMG, SS, na, da);
        render_ssaa(B, v, IMG, SS, nb, db);
        for(int i=0;i<IMG*IMG;++i){
            oR[i]=na[i*3]; oG[i]=na[i*3+1]; oB[i]=na[i*3+2];
            sR[i]=nb[i*3]; sG[i]=nb[i*3+1]; sB[i]=nb[i*3+2];
        }
        ssim_n += ssim_ch(oR, sR, 127, IMG);
        ssim_n += ssim_ch(oG, sG, 127, IMG);
        ssim_n += ssim_ch(oB, sB, 127, IMG);
        ssim_d += ssim_ch(da, db, 255, IMG);
    }
    ssim_n/=18.0; ssim_d/=6.0;
    double final_ssim=(ssim_n+ssim_d)*0.5;
    printf("FinalSSIM=%.4f (SS=%d)\n", final_ssim, SS);
    // Diagnostic: per-view breakdown
    printf("\n# Diagnostic Output\n");
    printf("FinalSSIM=%.4f (SS=%d)\n", final_ssim, SS);
    printf("NormalSSIM=%.4f\n", ssim_n);
    printf("DepthSSIM=%.4f\n", ssim_d);
    return 0;
}
