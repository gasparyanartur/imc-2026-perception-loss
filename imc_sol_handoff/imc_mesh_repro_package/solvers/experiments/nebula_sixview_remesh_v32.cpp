#include <bits/stdc++.h>

using namespace std;

static constexpr double CParam_HausdorffDiagFraction = 0.055;

static constexpr double CParam_MinNormalNorm = 1e-12;

static constexpr double CParam_QemSolveDeterminantEps = 1e-12;

static constexpr double CParam_Inf = 1e100;

static constexpr double HParam_TimeBudgetSeconds = 20.2;

static constexpr int HParam_OutputPrecisionSignificantDigits = 10;

static constexpr double HParam_KeepRatio_UpTo5k = 0.00;

static constexpr double HParam_KeepRatio_UpTo25k = 0.32;

static constexpr double HParam_KeepRatio_UpTo45k = 0.16;

static constexpr double HParam_KeepRatio_UpTo50k = 0.1;

static constexpr double HParam_KeepRatio_UpTo400k = 0.025;

static constexpr double HParam_KeepRatio_Huge = 0.032;

static constexpr double HParam_QemCostCapCoeff = 0.0375;

static constexpr int HParam_TailOriginalVertexThreshold = 400000;

static constexpr double HParam_TailBatchElapsedStart = 11.8;

static constexpr double HParam_TailBatchStopElapsed = 19.4;

static constexpr int HParam_TailBatchScanEdges = 65536;

static constexpr int HParam_TailBatchTargetAccepts = 2048;

static constexpr bool HParam_EnableRootNudge = true;

static constexpr int HParam_RootNudgeProfile = 1;

static constexpr double CParam_ViewWeightK = 3.0;

static constexpr double CParam_MaxFaceWeight = 3.0;

static constexpr bool HParam_EnableVegaSsimPass = true;

static constexpr int HParam_VegaPatchResolution = 512;

static constexpr int HParam_VegaPatchPaddingPixels = 4;

static constexpr int HParam_VegaPatchMaxPixels = 52000;

static constexpr int HParam_VegaCandidatePoolCap = 28000;

static constexpr double HParam_VegaNormalDepthWeight = 0.55;

static constexpr double HParam_VegaScoreGeomWeight = 0.0018;

static constexpr double HParam_VegaC1 = (0.01 * 255.0) * (0.01 * 255.0);

static constexpr double HParam_VegaC2 = (0.03 * 255.0) * (0.03 * 255.0);

struct Vec3 {

double x=0,y=0,z=0;

constexpr Vec3()=default;

constexpr Vec3(double x_,double y_,double z_):x(x_),y(y_),z(z_){}

Vec3 operator+(const Vec3& o)const{return{x+o.x,y+o.y,z+o.z};}

Vec3 operator-(const Vec3& o)const{return{x-o.x,y-o.y,z-o.z};}

Vec3 operator*(double s)const{return{x*s,y*s,z*s};}

Vec3 operator/(double s)const{return{x/s,y/s,z/s};}

};

static inline double dot(const Vec3& a,const Vec3& b){return a.x*b.x+a.y*b.y+a.z*b.z;}

static inline Vec3 cross(const Vec3& a,const Vec3& b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}

static inline double norm2(const Vec3& v){return dot(v,v);}

static inline double norm(const Vec3& v){return sqrt(norm2(v));}

static inline bool finiteVec(const Vec3& v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}

struct Face{int v[3];};

struct Quadric {

double a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;

Quadric& operator+=(const Quadric& o){a+=o.a;b+=o.b;c+=o.c;d+=o.d;e+=o.e;f+=o.f;g+=o.g;h+=o.h;i+=o.i;j+=o.j;return *this;}

void scale(double s){a*=s;b*=s;c*=s;d*=s;e*=s;f*=s;g*=s;h*=s;i*=s;j*=s;}

static Quadric fromTriangle(const Vec3& p0,const Vec3& p1,const Vec3& p2){

Vec3 n=cross(p1-p0,p2-p0);double ta=norm(n);

if(ta<CParam_MinNormalNorm)return Quadric();

n=n/ta;double pd=-dot(n,p0);

Quadric q;q.a=n.x*n.x;q.b=n.x*n.y;q.c=n.x*n.z;q.d=n.x*pd;

q.e=n.y*n.y;q.f=n.y*n.z;q.g=n.y*pd;q.h=n.z*n.z;q.i=n.z*pd;q.j=pd*pd;

q.scale(sqrt(0.5*ta));return q;

}

double evaluate(const Vec3& p)const{

return a*p.x*p.x+2*b*p.x*p.y+2*c*p.x*p.z+2*d*p.x+e*p.y*p.y+2*f*p.y*p.z+2*g*p.y+h*p.z*p.z+2*i*p.z+j;

}

};

struct SmallSet {

vector<int> data;

void insert(int v){

auto it=lower_bound(data.begin(),data.end(),v);

if(it==data.end()||*it!=v) data.insert(it,v);

}

void erase(int v){

auto it=lower_bound(data.begin(),data.end(),v);

if(it!=data.end()&&*it==v) data.erase(it);

}

bool contains(int v)const{ return binary_search(data.begin(),data.end(),v); }

int size()const{return(int)data.size();}

auto begin()const{return data.begin();}

auto end()const{return data.end();}

void clear(){data.clear();}

void reserve(int n){data.reserve(n);}

};

static double det3(double a00,double a01,double a02,double a10,double a11,double a12,double a20,double a21,double a22){

return a00*(a11*a22-a12*a21)-a01*(a10*a22-a12*a20)+a02*(a10*a21-a11*a20);

}

static bool solveQem3x3(const Quadric& q,Vec3& out){

double D=det3(q.a,q.b,q.c,q.b,q.e,q.f,q.c,q.f,q.h);

if(fabs(D)<CParam_QemSolveDeterminantEps)return false;

out=Vec3(det3(-q.d,q.b,q.c,-q.g,q.e,q.f,-q.i,q.f,q.h)/D,

det3(q.a,-q.d,q.c,q.b,-q.g,q.f,q.c,-q.i,q.h)/D,

det3(q.a,q.b,-q.d,q.b,q.e,-q.g,q.c,q.f,-q.i)/D);

return finiteVec(out);

}

struct CollapseCandidate {

int absorbed=-1,kept=-1,versionAbsorbed=-1,versionKept=-1;

double cost=CParam_Inf,mergedRadius=0.0;

Vec3 position;

bool operator<(const CollapseCandidate& o)const{return cost>o.cost;}

bool valid()const{return absorbed>=0&&kept>=0&&cost<CParam_Inf;}

};

class QemSimplifier {

public:

static bool MEMLESS;

public:

void run(){

readMesh();

if(nV<=10){writeMesh();return;}

startTime=chrono::steady_clock::now();

if(nV>5000&&nV<=50000){
if(nV<=25000){runDepthHullT2();writeMesh();return;}
if(nV>25000&&nV<=45000){runScreenCoreMid();compact();writeMesh();return;}
runTransactionalScreenMid();writeMesh();return;
}

initScale();

buildConnectivity();

initFaceWeights();

initQueue();

collapseLoop();

if (elapsed() < HParam_TimeBudgetSeconds - 0.95) {

valenceWeldPass();

}

if (elapsed() < HParam_TimeBudgetSeconds - 1.10) {

pairDiskPass();

}

// Star-delete is disabled on the huge tier (T7): on very large, irregular

// meshes the time-gated retriangulation is nondeterministic and can cross a

// validity/Hausdorff edge on some judge runs. T7 gains little from it anyway

// (it is Hausdorff-capped, not flat-vertex-capped). Keep it for T2-T6.

if (nV <= HParam_TailOriginalVertexThreshold &&

elapsed() < HParam_TimeBudgetSeconds - 1.0) {

collapseInvisibleEdges();

}

if (elapsed() < HParam_TimeBudgetSeconds - 0.88) {

valenceWeldPass();

}

if (elapsed() < HParam_TimeBudgetSeconds - 0.78) {

pairDiskPass();

}

if (HParam_EnableVegaSsimPass &&

elapsed() < HParam_TimeBudgetSeconds - 0.65) {

vegaSsimStarPass();

}

// Sharon v13: 2nd Vega pass (Cherry v24 style)

if (HParam_EnableVegaSsimPass &&

elapsed() < HParam_TimeBudgetSeconds - 0.30) {

vegaSsimStarPass();

}

if (HParam_EnableVegaSsimPass &&

elapsed() < HParam_TimeBudgetSeconds - 0.12) {

vegaSsimStarPass();

}

// Use the proven v17 base ceiling on the 400k case; keep the aggressive large continuation only for the huge case.
if(nV>400000 && elapsed()<HParam_TimeBudgetSeconds-0.35) runLargeCameraTx(nV);
else compact();

writeMesh();

}

private:

int nV=0,nF=0;

vector<Vec3> verts;

vector<Face> faces;

vector<char> vdead,fdead;

vector<int> vver;

vector<Quadric> vquad,vmoment;

vector<double> crad;

vector<vector<int>> vfaces;

vector<SmallSet> vneigh;

vector<int> tailLocks;

vector<int> facePix,faceSil,faceWin;
vector<double> vertAnchor;

int screenTier=0;
double faceImpMean=0.0;

priority_queue<CollapseCandidate> pq;

int targetV=0,collapseLimit=0,accepted=0;

int tailCursor=0,tailStamp=1,lastFailBatch=-1,starCursor=0,vegaCursor=0;

double diag=0,hausd=0,costCap=CParam_Inf;

double invDiag2 = 0.0;

chrono::steady_clock::time_point startTime;

static constexpr Vec3 cameraDirs[6] = {

{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}

};

void readMesh(){

vector<char> buf;buf.reserve(1<<27);

char chunk[1<<16];size_t n;

while((n=fread(chunk,1,sizeof(chunk),stdin))>0)buf.insert(buf.end(),chunk,chunk+n);

buf.push_back('\0');char*p=buf.data();

nV=(int)strtol(p,&p,10);nF=(int)strtol(p,&p,10);

verts.resize(nV);faces.resize(nF);

for(int i=0;i<nV;++i){while(*p&&*p<=' ')++p;++p;verts[i].x=strtod(p,&p);verts[i].y=strtod(p,&p);verts[i].z=strtod(p,&p);}

for(int i=0;i<nF;++i){while(*p&&*p<=' ')++p;++p;faces[i].v[0]=(int)strtol(p,&p,10)-1;faces[i].v[1]=(int)strtol(p,&p,10)-1;faces[i].v[2]=(int)strtol(p,&p,10)-1;}

}

void writeMesh(){

string out;out.reserve(nV*42+nF*26+64);char line[128];

snprintf(line,sizeof(line),"%d %d\n",nV,nF);out+=line;

for(int i=0;i<nV;++i){snprintf(line,sizeof(line),"v %.*g %.*g %.*g\n",HParam_OutputPrecisionSignificantDigits,verts[i].x,HParam_OutputPrecisionSignificantDigits,verts[i].y,HParam_OutputPrecisionSignificantDigits,verts[i].z);out+=line;}

for(int i=0;i<nF;++i){snprintf(line,sizeof(line),"f %d %d %d\n",faces[i].v[0]+1,faces[i].v[1]+1,faces[i].v[2]+1);out+=line;}

fwrite(out.data(),1,out.size(),stdout);

}


void cameraBasisImp(int view,Vec3&eye,Vec3&right,Vec3&up,Vec3&fwd)const{
Vec3 dirs[6]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
eye=dirs[view]*2.5;fwd=eye*(-1.0);double fl=norm(fwd);fwd=fwd/fl;
Vec3 wu=(fabs(dot(fwd,Vec3(0,0,1)))>0.9)?Vec3(0,1,0):Vec3(0,0,1);
right=cross(wu,fwd);double rl=norm(right);right=rl>1e-15?right/rl:Vec3(1,0,0);
up=cross(fwd,right);double ul=norm(up);up=ul>1e-15?up/ul:Vec3(0,1,0);
}
struct ImpProj{double u=0,v=0,z=0;bool ok=false;};
ImpProj projectImp(const Vec3&p,int view,int R)const{
Vec3 e,r,u,f;cameraBasisImp(view,e,r,u,f);Vec3 q=p-e;
double x=dot(q,r),y=dot(q,u),z=dot(q,f);if(z<=1e-8)return {};
double sc=double(R)/1024.0;return {800.0*sc*x/z+0.5*R,800.0*sc*y/z+0.5*R,z,true};
}
void buildRasterImportance(int R){
facePix.assign(nF,0);faceSil.assign(nF,0);faceWin.assign(nF,0);vector<Vec3>fn(nF);
for(int fi=0;fi<nF;++fi){if(fi<(int)fdead.size()&&fdead[fi])continue;auto&f=faces[fi];Vec3 n=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);double l=norm(n);if(l>1e-15)fn[fi]=n/l;}
int rad=max(1,(int)ceil(5.0*R/1024.0));
for(int view=0;view<6;++view){vector<float>z((size_t)R*R,1e30f);vector<int>id((size_t)R*R,-1);
for(int fi=0;fi<nF;++fi){if(fi<(int)fdead.size()&&fdead[fi])continue;auto&f=faces[fi];auto a=projectImp(verts[f.v[0]],view,R),b=projectImp(verts[f.v[1]],view,R),c=projectImp(verts[f.v[2]],view,R);if(!a.ok||!b.ok||!c.ok)continue;
double den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);if(fabs(den)<1e-18)continue;
int x0=max(0,(int)floor(min({a.u,b.u,c.u}))),x1=min(R-1,(int)ceil(max({a.u,b.u,c.u}))),y0=max(0,(int)floor(min({a.v,b.v,c.v}))),y1=min(R-1,(int)ceil(max({a.v,b.v,c.v})));
for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){double X=x+.5,Y=y+.5,w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den,w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den,w2=1-w0-w1;if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;double iz=w0/a.z+w1/b.z+w2/c.z;if(iz<=0)continue;float zz=1.0/iz;int q=y*R+x;if(zz<z[q]){z[q]=zz;id[q]=fi;}}}
for(int q=0;q<R*R;++q)if(id[q]>=0)++facePix[id[q]];
vector<unsigned short>d((size_t)R*R,60000);
for(int y=0;y<R;++y)for(int x=0;x<R;++x){int q=y*R+x,f=id[q];if(f<0)continue;bool ft=false;for(int dy=-1;dy<=1&&!ft;++dy)for(int dx=-1;dx<=1;++dx){if(!dx&&!dy)continue;int xx=x+dx,yy=y+dy;if(xx<0||yy<0||xx>=R||yy>=R){ft=true;break;}int g=id[yy*R+xx];if(g<0||(g!=f&&(fabs(z[q]-z[yy*R+xx])>0.0035||dot(fn[f],fn[g])<0.992))){ft=true;break;}}if(ft){d[q]=0;++faceSil[f];}}
for(int y=0;y<R;++y)for(int x=0;x<R;++x){int q=y*R+x,v=d[q];if(x)v=min<int>(v,d[q-1]+1);if(y)v=min<int>(v,d[q-R]+1);if(x&&y)v=min<int>(v,d[q-R-1]+1);if(x+1<R&&y)v=min<int>(v,d[q-R+1]+1);d[q]=v;}
for(int y=R-1;y>=0;--y)for(int x=R-1;x>=0;--x){int q=y*R+x,v=d[q];if(x+1<R)v=min<int>(v,d[q+1]+1);if(y+1<R)v=min<int>(v,d[q+R]+1);if(x+1<R&&y+1<R)v=min<int>(v,d[q+R+1]+1);if(x&&y+1<R)v=min<int>(v,d[q+R-1]+1);d[q]=v;if(v<=rad&&id[q]>=0)++faceWin[id[q]];}
}}
void buildRasterImportanceTier3(int R){
facePix.assign(nF,0);faceSil.assign(nF,0);faceWin.assign(nF,0);vector<Vec3>fn(nF);
for(int fi=0;fi<nF;++fi){if(fi<(int)fdead.size()&&fdead[fi])continue;auto&f=faces[fi];Vec3 n=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);double l=norm(n);if(l>1e-15)fn[fi]=n/l;}
for(int view=0;view<6;++view){vector<float>z((size_t)R*R,1e30f);vector<int>id((size_t)R*R,-1);
for(int fi=0;fi<nF;++fi){if(fi<(int)fdead.size()&&fdead[fi])continue;auto&f=faces[fi];auto a=projectImp(verts[f.v[0]],view,R),b=projectImp(verts[f.v[1]],view,R),c=projectImp(verts[f.v[2]],view,R);if(!a.ok||!b.ok||!c.ok)continue;
double den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);if(fabs(den)<1e-18)continue;
int x0=max(0,(int)floor(min({a.u,b.u,c.u}))),x1=min(R-1,(int)ceil(max({a.u,b.u,c.u}))),y0=max(0,(int)floor(min({a.v,b.v,c.v}))),y1=min(R-1,(int)ceil(max({a.v,b.v,c.v})));
for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){double X=x+.5,Y=y+.5,w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den,w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den,w2=1-w0-w1;if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;double iz=w0/a.z+w1/b.z+w2/c.z;if(iz<=0)continue;float zz=1.0/iz;int q=y*R+x;if(zz<z[q]){z[q]=zz;id[q]=fi;}}}
for(int q=0;q<R*R;++q)if(id[q]>=0)++facePix[id[q]];
for(int y=1;y<R-1;++y)for(int x=1;x<R-1;++x){int q=y*R+x,f=id[q];if(f<0)continue;bool sil=false;for(int d:{-1,1,-R,R}){int g=id[q+d];if(g<0||(g!=f&&(fabs(z[q]-z[q+d])>0.006||dot(fn[f],fn[g])<0.985))){sil=true;break;}}if(sil)++faceSil[f];}
}}
void runScreenCoreMid(){
screenTier=nV<=25000?2:(nV<=45000?3:4);initScale();buildConnectivity();vmoment.assign(nV,Quadric());if(screenTier==4)for(int i=0;i<nV;++i)vmoment[i]=anchorPointQuadric(verts[i],1e-6);
if(screenTier==3){int finalTarget=(int)floor(nV*0.145),safeTarget=(int)floor(nV*0.16);buildRasterImportanceTier3(384);initFaceWeights();targetV=safeTarget;collapseLimit=nV-targetV;initQueue();collapseLoop();if(accepted<nV-finalTarget&&elapsed()<HParam_TimeBudgetSeconds-0.4){buildRasterImportanceTier3(384);initFaceWeights();priority_queue<CollapseCandidate>e;pq.swap(e);targetV=finalTarget;collapseLimit=nV-targetV;initQueue();collapseLoop();}return;}
vector<double>st=screenTier==2?vector<double>{0.36,0.33,0.30}:vector<double>{0.14,0.10,0.08};
for(double kr:st){if(elapsed()>HParam_TimeBudgetSeconds-0.5)break;buildRasterImportance(screenTier==2?1024:768);initFaceWeights();priority_queue<CollapseCandidate>e;pq.swap(e);targetV=(int)floor(nV*kr);collapseLimit=nV-targetV;initQueue();collapseLoop();}
}


struct RefImage{vector<float>nx,ny,nz,d,z;vector<unsigned char>fg;RefImage(){}RefImage(int R):nx((size_t)R*R,127.5f),ny((size_t)R*R,127.5f),nz((size_t)R*R,127.5f),d((size_t)R*R,255.0f),z((size_t)R*R,1e30f),fg((size_t)R*R,0){}};
RefImage renderReference(const vector<Vec3>&vv,const vector<Face>&ff,int view,int R)const{RefImage im(R);for(const Face&fc:ff){if(fc.v[0]<0||fc.v[1]<0||fc.v[2]<0||fc.v[0]>=(int)vv.size()||fc.v[1]>=(int)vv.size()||fc.v[2]>=(int)vv.size())continue;auto p0=projectImp(vv[fc.v[0]],view,R),p1=projectImp(vv[fc.v[1]],view,R),p2=projectImp(vv[fc.v[2]],view,R);if(!p0.ok||!p1.ok||!p2.ok)continue;double den=(p1.v-p2.v)*(p0.u-p2.u)+(p2.u-p1.u)*(p0.v-p2.v);if(fabs(den)<1e-18)continue;Vec3 nr=cross(vv[fc.v[1]]-vv[fc.v[0]],vv[fc.v[2]]-vv[fc.v[0]]);double nl=norm(nr);if(nl<1e-15)continue;nr=nr/nl;int x0=max(0,(int)floor(min({p0.u,p1.u,p2.u}))),x1=min(R-1,(int)ceil(max({p0.u,p1.u,p2.u}))),y0=max(0,(int)floor(min({p0.v,p1.v,p2.v}))),y1=min(R-1,(int)ceil(max({p0.v,p1.v,p2.v})));for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){double X=x+.5,Y=y+.5,w0=((p1.v-p2.v)*(X-p2.u)+(p2.u-p1.u)*(Y-p2.v))/den,w1=((p2.v-p0.v)*(X-p2.u)+(p0.u-p2.u)*(Y-p2.v))/den,w2=1-w0-w1;if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;double iz=w0/p0.z+w1/p1.z+w2/p2.z;if(iz<=0)continue;float zz=(float)(1.0/iz);int q=y*R+x;if(zz>=im.z[q])continue;im.z[q]=zz;im.nx[q]=(float)((nr.x+1.0)*127.5);im.ny[q]=(float)((nr.y+1.0)*127.5);im.nz[q]=(float)((nr.z+1.0)*127.5);im.d[q]=zz;im.fg[q]=1;}}return im;}
double refSsim(const vector<float>&a,const vector<float>&b,const vector<unsigned char>&fa,const vector<unsigned char>&fb,int R)const{int S=R+1;size_t N=(size_t)S*S;vector<double>ix(N),iy(N),ix2(N),iy2(N),ixy(N);for(int y=0;y<R;++y){double sx=0,sy=0,sx2=0,sy2=0,sxy=0;size_t pr=(size_t)y*S,ro=(size_t)(y+1)*S;for(int x=0;x<R;++x){double X=a[y*R+x],Y=b[y*R+x];sx+=X;sy+=Y;sx2+=X*X;sy2+=Y*Y;sxy+=X*Y;size_t q=ro+x+1,u=pr+x+1;ix[q]=ix[u]+sx;iy[q]=iy[u]+sy;ix2[q]=ix2[u]+sx2;iy2[q]=iy2[u]+sy2;ixy[q]=ixy[u]+sxy;}}auto rect=[&](const vector<double>&I,int x0,int y0,int x1,int y1){size_t A=(size_t)y0*S+x0,B=(size_t)y0*S+x1,C=(size_t)y1*S+x0,D=(size_t)y1*S+x1;return I[D]-I[B]-I[C]+I[A];};double inv=1.0/121.0,tot=0;long long cnt=0;double C1=6.5025,C2=58.5225;for(int y=5;y<R-5;++y)for(int x=5;x<R-5;++x){int q=y*R+x;if(!fa[q]&&!fb[q])continue;int x0=x-5,y0=y-5,x1=x+6,y1=y+6;double sx=rect(ix,x0,y0,x1,y1),sy=rect(iy,x0,y0,x1,y1),sxx=rect(ix2,x0,y0,x1,y1),syy=rect(iy2,x0,y0,x1,y1),sxy=rect(ixy,x0,y0,x1,y1),mx=sx*inv,my=sy*inv,vx=max(0.0,sxx*inv-mx*mx),vy=max(0.0,syy*inv-my*my),cov=sxy*inv-mx*my,num=(2*mx*my+C1)*(2*cov+C2),den=(mx*mx+my*my+C1)*(vx+vy+C2);tot+=den>0?max(-1.0,min(1.0,num/den)):1.0;++cnt;}return cnt?tot/cnt:1.0;}
struct RefScore{double finalScore=0,minView=1,minNormal=1,minDepth=1;};
RefScore compareToReference(const vector<RefImage>&ref,const vector<Vec3>&vv,const vector<Face>&ff,int R)const{RefScore o;for(int v=0;v<6;++v){RefImage b=renderReference(vv,ff,v,R);double sn=(refSsim(ref[v].nx,b.nx,ref[v].fg,b.fg,R)+refSsim(ref[v].ny,b.ny,ref[v].fg,b.fg,R)+refSsim(ref[v].nz,b.nz,ref[v].fg,b.fg,R))/3.0,sd=refSsim(ref[v].d,b.d,ref[v].fg,b.fg,R),cv=.5*(sn+sd);o.finalScore+=cv;o.minView=min(o.minView,cv);o.minNormal=min(o.minNormal,sn);o.minDepth=min(o.minDepth,sd);}o.finalScore/=6;return o;}
void extractActiveMesh(vector<Vec3>&vv,vector<Face>&ff)const{vector<int>m(verts.size(),-1);vv.clear();ff.clear();vv.reserve(verts.size());for(int i=0;i<(int)verts.size();++i)if(i>=(int)vdead.size()||!vdead[i]){m[i]=(int)vv.size();vv.push_back(verts[i]);}for(int fi=0;fi<(int)faces.size();++fi){if(fi<(int)fdead.size()&&fdead[fi])continue;const Face&f=faces[fi];if(f.v[0]<0||f.v[1]<0||f.v[2]<0||f.v[0]>=(int)m.size()||f.v[1]>=(int)m.size()||f.v[2]>=(int)m.size())continue;int a=m[f.v[0]],b=m[f.v[1]],c=m[f.v[2]];if(a<0||b<0||c<0||a==b||b==c||a==c)continue;Face q;q.v[0]=a;q.v[1]=b;q.v[2]=c;ff.push_back(q);}}
struct MidSnap{vector<Vec3>verts;vector<Face>faces;vector<char>vdead,fdead;vector<int>vver;vector<Quadric>vquad,vmoment;vector<double>crad;vector<vector<int>>vfaces;vector<SmallSet>vneigh;vector<int>tailLocks,facePix,faceSil,faceWin;vector<double>vertAnchor;int targetV,collapseLimit,accepted,tailCursor,tailStamp,lastFailBatch,starCursor,vegaCursor,screenTier;double faceImpMean;};
MidSnap midSnapshot()const{return {verts,faces,vdead,fdead,vver,vquad,vmoment,crad,vfaces,vneigh,tailLocks,facePix,faceSil,faceWin,vertAnchor,targetV,collapseLimit,accepted,tailCursor,tailStamp,lastFailBatch,starCursor,vegaCursor,screenTier,faceImpMean};}
void restoreMid(const MidSnap&s){verts=s.verts;faces=s.faces;vdead=s.vdead;fdead=s.fdead;vver=s.vver;vquad=s.vquad;vmoment=s.vmoment;crad=s.crad;vfaces=s.vfaces;vneigh=s.vneigh;tailLocks=s.tailLocks;facePix=s.facePix;faceSil=s.faceSil;faceWin=s.faceWin;vertAnchor=s.vertAnchor;targetV=s.targetV;collapseLimit=s.collapseLimit;accepted=s.accepted;tailCursor=s.tailCursor;tailStamp=s.tailStamp;lastFailBatch=s.lastFailBatch;starCursor=s.starCursor;vegaCursor=s.vegaCursor;screenTier=s.screenTier;faceImpMean=s.faceImpMean;priority_queue<CollapseCandidate>e;pq.swap(e);}
void continueScreenTo(int tier,int originalN,double ratio,int freeRemoved){if(tier==3)buildRasterImportanceTier3(512);else buildRasterImportance(tier==2?1024:768);initFaceWeights();priority_queue<CollapseCandidate>e;pq.swap(e);targetV=max(10,(int)floor(originalN*ratio)-freeRemoved);collapseLimit=originalN-targetV;initQueue();collapseLoop();}
bool txGuard(const RefScore&s,int tier)const{if(tier==2)return s.finalScore>=.9965&&s.minView>=.9955&&s.minNormal>=.9930&&s.minDepth>=.9988;if(tier==3)return s.finalScore>=.9950&&s.minView>=.9942&&s.minNormal>=.9892&&s.minDepth>=.9985;return s.finalScore>=.9950&&s.minView>=.9942&&s.minNormal>=.9892&&s.minDepth>=.9985;}
void runTransactionalScreenMid(){
int originalN=nV,tier=nV<=25000?2:(nV<=45000?3:4);
if(tier==2){
runScreenCoreMid();
MidSnap safe=midSnapshot();
vector<Vec3>safeV,candV;vector<Face>safeF,candF;
extractActiveMesh(safeV,safeF);
int R=1024;
if(elapsed()>HParam_TimeBudgetSeconds-4.0){compact();return;}
vector<RefImage>ref(6);for(int v=0;v<6;++v)ref[v]=renderReference(safeV,safeF,v,R);
vector<double>tries={.27,.28,.285,.29,.295};bool kept=false;
for(double ratio:tries){
if(elapsed()>HParam_TimeBudgetSeconds-1.8)break;
restoreMid(safe);continueScreenTo(tier,originalN,ratio,0);
extractActiveMesh(candV,candF);RefScore sc=compareToReference(ref,candV,candF,R);
if(txGuard(sc,tier)){verts=candV;faces=candF;nV=(int)verts.size();nF=(int)faces.size();kept=true;break;}
}
if(!kept){restoreMid(safe);compact();}
return;
}
runScreenCoreMid();
MidSnap preHidden=midSnapshot();
vector<Vec3>baseV,safeV,candV;vector<Face>baseF,safeF,candF;
extractActiveMesh(baseV,baseF);
int freeRemoved=0,R=1024;
if(elapsed()<HParam_TimeBudgetSeconds-4.8){
freeRemoved=occludedEdgeCollapsePass();
if(freeRemoved>0){
vector<RefImage>br(6);for(int v=0;v<6;++v)br[v]=renderReference(baseV,baseF,v,R);
extractActiveMesh(safeV,safeF);RefScore hs=compareToReference(br,safeV,safeF,R);
if(!(hs.finalScore>=.9995&&hs.minView>=.9990&&hs.minNormal>=.9985&&hs.minDepth>=.9995)){restoreMid(preHidden);freeRemoved=0;}
}
}
MidSnap safe=midSnapshot();extractActiveMesh(safeV,safeF);
if(elapsed()>HParam_TimeBudgetSeconds-4.0){compact();return;}
vector<RefImage>ref(6);for(int v=0;v<6;++v)ref[v]=renderReference(safeV,safeF,v,R);
vector<double>tries=tier==3?vector<double>{.125,.13,.135,.1375,.14,.1425}:vector<double>{.065,.07,.075,.0775,.079};
bool kept=false;
for(double ratio:tries){
if(elapsed()>HParam_TimeBudgetSeconds-1.8)break;
restoreMid(safe);continueScreenTo(tier,originalN,ratio,freeRemoved);
extractActiveMesh(candV,candF);RefScore sc=compareToReference(ref,candV,candF,R);
if(txGuard(sc,tier)){verts=candV;faces=candF;nV=(int)verts.size();nF=(int)faces.size();kept=true;break;}
}
if(!kept){restoreMid(safe);compact();}
}


// ---------- v32: six-view depth-hull surface-net alternate core (T2 only) ----------
// This is intentionally independent of the QEM connectivity.  It reconstructs a
// low-resolution watertight surface from the six judge-aligned front/back depth
// intervals, validates it against the ORIGINAL mesh, and otherwise returns v23.
struct HullF3{float x=0,y=0,z=0;};
struct HullAtlas{
    int R=0;
    array<vector<float>,6> front,back;
    array<vector<HullF3>,6> frontN,backN;
};
struct HullCellVert{int id=-1;};

static HullF3 hullF3(const Vec3&v){return {(float)v.x,(float)v.y,(float)v.z};}
static Vec3 hullVec3(const HullF3&v){return Vec3(v.x,v.y,v.z);}

void buildHullAtlas(const vector<Vec3>&vv,const vector<Face>&ff,int R,HullAtlas&h)const{
    h.R=R;
    const size_t P=(size_t)R*R;
    for(int view=0;view<6;++view){
        h.front[view].assign(P,1e30f);h.back[view].assign(P,-1e30f);
        h.frontN[view].assign(P,{});h.backN[view].assign(P,{});
        for(const Face&fc:ff){
            if(fc.v[0]<0||fc.v[1]<0||fc.v[2]<0||fc.v[0]>=(int)vv.size()||fc.v[1]>=(int)vv.size()||fc.v[2]>=(int)vv.size())continue;
            auto p0=projectImp(vv[fc.v[0]],view,R),p1=projectImp(vv[fc.v[1]],view,R),p2=projectImp(vv[fc.v[2]],view,R);
            if(!p0.ok||!p1.ok||!p2.ok)continue;
            double den=(p1.v-p2.v)*(p0.u-p2.u)+(p2.u-p1.u)*(p0.v-p2.v);
            if(fabs(den)<1e-18)continue;
            Vec3 nr=cross(vv[fc.v[1]]-vv[fc.v[0]],vv[fc.v[2]]-vv[fc.v[0]]);double nl=norm(nr);if(nl<CParam_MinNormalNorm)continue;nr=nr/nl;
            int x0=max(0,(int)floor(min({p0.u,p1.u,p2.u}))),x1=min(R-1,(int)ceil(max({p0.u,p1.u,p2.u})));
            int y0=max(0,(int)floor(min({p0.v,p1.v,p2.v}))),y1=min(R-1,(int)ceil(max({p0.v,p1.v,p2.v})));
            for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
                double X=x+.5,Y=y+.5;
                double w0=((p1.v-p2.v)*(X-p2.u)+(p2.u-p1.u)*(Y-p2.v))/den;
                double w1=((p2.v-p0.v)*(X-p2.u)+(p0.u-p2.u)*(Y-p2.v))/den;
                double w2=1.0-w0-w1;
                if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
                double iz=w0/p0.z+w1/p1.z+w2/p2.z;if(iz<=0)continue;
                float z=(float)(1.0/iz);int q=y*R+x;
                if(z<h.front[view][q]){h.front[view][q]=z;h.frontN[view][q]=hullF3(nr);}
                if(z>h.back[view][q]){h.back[view][q]=z;h.backN[view][q]=hullF3(nr);}
            }
        }
    }
}

bool hullInterval(const HullAtlas&h,int view,double u,double v,float&fr,float&bk,int&fq,int&bq)const{
    int cx=(int)floor(u),cy=(int)floor(v);fr=1e30f;bk=-1e30f;fq=bq=-1;
    for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int x=cx+dx,y=cy+dy;if(x<0||y<0||x>=h.R||y>=h.R)continue;int q=y*h.R+x;
        if(h.front[view][q]<fr){fr=h.front[view][q];fq=q;}
        if(h.back[view][q]>bk){bk=h.back[view][q];bq=q;}
    }
    return fq>=0&&bq>=0&&fr<1e20f&&bk>-1e20f&&fr<=bk;
}

bool insideDepthHull(const Vec3&p,const HullAtlas&h,double eps)const{
    for(int view=0;view<6;++view){auto pr=projectImp(p,view,h.R);if(!pr.ok||pr.u<0||pr.v<0||pr.u>=h.R||pr.v>=h.R)return false;float fr,bk;int fq,bq;if(!hullInterval(h,view,pr.u,pr.v,fr,bk,fq,bq))return false;if(pr.z<(double)fr-eps||pr.z>(double)bk+eps)return false;}
    return true;
}

Vec3 hullConstraintNormal(const Vec3&p,const HullAtlas&h)const{
    double best=1e100;Vec3 out;
    for(int view=0;view<6;++view){auto pr=projectImp(p,view,h.R);if(!pr.ok)continue;float fr,bk;int fq,bq;if(!hullInterval(h,view,pr.u,pr.v,fr,bk,fq,bq))continue;
        double df=fabs(pr.z-fr),db=fabs(pr.z-bk);
        if(df<best){Vec3 n=hullVec3(h.frontN[view][fq]);if(norm2(n)>1e-12){best=df;out=n;}}
        if(db<best){Vec3 n=hullVec3(h.backN[view][bq]);if(norm2(n)>1e-12){best=db;out=n;}}
    }
    double l=norm(out);return l>1e-12?out/l:Vec3();
}

struct HullQef{
    double a00=0,a01=0,a02=0,a11=0,a12=0,a22=0,b0=0,b1=0,b2=0;
    void add(const Vec3&n,const Vec3&p,double w=1.0){double d=dot(n,p);a00+=w*n.x*n.x;a01+=w*n.x*n.y;a02+=w*n.x*n.z;a11+=w*n.y*n.y;a12+=w*n.y*n.z;a22+=w*n.z*n.z;b0+=w*n.x*d;b1+=w*n.y*d;b2+=w*n.z*d;}
    void anchor(const Vec3&p,double w){a00+=w;a11+=w;a22+=w;b0+=w*p.x;b1+=w*p.y;b2+=w*p.z;}
    bool solve(Vec3&x)const{double D=det3(a00,a01,a02,a01,a11,a12,a02,a12,a22);if(fabs(D)<1e-14)return false;x.x=det3(b0,a01,a02,b1,a11,a12,b2,a12,a22)/D;x.y=det3(a00,b0,a02,a01,b1,a12,a02,b2,a22)/D;x.z=det3(a00,a01,b0,a01,a11,b1,a02,a12,b2)/D;return finiteVec(x);}
};

bool validateClosedOrientedHull(const vector<Vec3>&vv,const vector<Face>&ff)const{
    if(vv.size()<4||ff.size()<4)return false;
    struct EC{int count=0,sum=0;};unordered_map<unsigned long long,EC> em;em.reserve(ff.size()*4);
    set<array<int,3>> uniq;
    for(const Face&f:ff){int a=f.v[0],b=f.v[1],c=f.v[2];if(a<0||b<0||c<0||a>=(int)vv.size()||b>=(int)vv.size()||c>=(int)vv.size()||a==b||b==c||a==c)return false;if(norm(cross(vv[b]-vv[a],vv[c]-vv[a]))<1e-12*max(1.0,diag*diag))return false;array<int,3>k={a,b,c};sort(k.begin(),k.end());if(!uniq.insert(k).second)return false;int e[3][2]={{a,b},{b,c},{c,a}};for(auto&z:e){int x=z[0],y=z[1];int lo=min(x,y),hi=max(x,y);unsigned long long key=((unsigned long long)(unsigned int)lo<<32)|(unsigned int)hi;auto&t=em[key];++t.count;t.sum+=(x==lo?1:-1);}}
    for(auto&kv:em)if(kv.second.count!=2||kv.second.sum!=0)return false;
    return true;
}


bool buildDepthHullMarchingTets(const vector<Vec3>&ov,const vector<Face>&of,int N,int R,vector<Vec3>&outV,vector<Face>&outF)const{
    HullAtlas at;buildHullAtlas(ov,of,R,at);
    Vec3 mn=ov[0],mx=ov[0];for(const Vec3&p:ov){mn.x=min(mn.x,p.x);mn.y=min(mn.y,p.y);mn.z=min(mn.z,p.z);mx.x=max(mx.x,p.x);mx.y=max(mx.y,p.y);mx.z=max(mx.z,p.z);}double pad=max(1e-6,0.025*diag);mn=mn-Vec3(pad,pad,pad);mx=mx+Vec3(pad,pad,pad);
    Vec3 step={(mx.x-mn.x)/N,(mx.y-mn.y)/N,(mx.z-mn.z)/N};double eps=0.55*max({step.x,step.y,step.z});int G=N+1;
    auto ni=[&](int x,int y,int z){return (int)(((size_t)z*G+y)*G+x);};auto gp=[&](int x,int y,int z){return Vec3(mn.x+x*step.x,mn.y+y*step.y,mn.z+z*step.z);};
    vector<unsigned char>inside((size_t)G*G*G,0);vector<Vec3>npos((size_t)G*G*G);
    for(int z=0;z<G;++z)for(int y=0;y<G;++y)for(int x=0;x<G;++x){int q=ni(x,y,z);npos[q]=gp(x,y,z);if(x==0||y==0||z==0||x==N||y==N||z==N)inside[q]=0;else inside[q]=insideDepthHull(npos[q],at,eps)?1:0;}
    outV.clear();outF.clear();unordered_map<unsigned long long,int> ev;ev.reserve((size_t)12*N*N*N/5);
    auto edgeVert=[&](int ia,int ib)->int{int lo=min(ia,ib),hi=max(ia,ib);unsigned long long key=((unsigned long long)(unsigned int)lo<<32)|(unsigned int)hi;auto it=ev.find(key);if(it!=ev.end())return it->second;Vec3 a=npos[ia],b=npos[ib];bool sa=inside[ia];for(int k=0;k<5;++k){Vec3 m=(a+b)*.5;bool sm=insideDepthHull(m,at,eps);if(sm==sa)a=m;else b=m;}int id=(int)outV.size();outV.push_back((a+b)*.5);ev.emplace(key,id);return id;};
    auto addTri=[&](int a,int b,int c,const Vec3&outward){if(a<0||b<0||c<0||a==b||b==c||a==c)return;Vec3 nn=cross(outV[b]-outV[a],outV[c]-outV[a]);if(dot(nn,outward)<0)swap(b,c);outF.push_back(Face{{a,b,c}});};
    auto addQuad=[&](int a,int b,int c,int d,const Vec3&outward){if(a==b||b==c||c==d||d==a||a==c||b==d)return;int q[4]={a,b,c,d};Vec3 nn=cross(outV[q[1]]-outV[q[0]],outV[q[2]]-outV[q[0]]);if(dot(nn,outward)<0)swap(q[1],q[3]);double d02=norm2(outV[q[0]]-outV[q[2]]),d13=norm2(outV[q[1]]-outV[q[3]]);if(d02<=d13){outF.push_back(Face{{q[0],q[1],q[2]}});outF.push_back(Face{{q[0],q[2],q[3]}});}else{outF.push_back(Face{{q[0],q[1],q[3]}});outF.push_back(Face{{q[1],q[2],q[3]}});}};
    static const int C[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    static const int T[6][4]={{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6},{0,5,1,6}};
    for(int z=0;z<N;++z)for(int y=0;y<N;++y)for(int x=0;x<N;++x){int cn[8];for(int i=0;i<8;++i)cn[i]=ni(x+C[i][0],y+C[i][1],z+C[i][2]);for(auto&tt:T){int in[4],ou[4],ic=0,oc=0;for(int j=0;j<4;++j){int id=cn[tt[j]];(inside[id]?in[ic++]:ou[oc++])=id;}if(ic==0||ic==4)continue;if(ic==1){int p0=edgeVert(in[0],ou[0]),p1=edgeVert(in[0],ou[1]),p2=edgeVert(in[0],ou[2]);Vec3 od=(npos[ou[0]]+npos[ou[1]]+npos[ou[2]])/3.0-npos[in[0]];addTri(p0,p1,p2,od);}else if(ic==3){int p0=edgeVert(ou[0],in[0]),p1=edgeVert(ou[0],in[1]),p2=edgeVert(ou[0],in[2]);Vec3 od=npos[ou[0]]-(npos[in[0]]+npos[in[1]]+npos[in[2]])/3.0;addTri(p0,p1,p2,od);}else{int a=edgeVert(in[0],ou[0]),b=edgeVert(in[0],ou[1]),c=edgeVert(in[1],ou[1]),d=edgeVert(in[1],ou[0]);Vec3 od=(npos[ou[0]]+npos[ou[1]])*.5-(npos[in[0]]+npos[in[1]])*.5;addQuad(a,b,c,d,od);}}}
    auto signedVol=[&](const vector<Vec3>&vv,const vector<Face>&ff){long double s=0;for(const Face&f:ff){const Vec3&a=vv[f.v[0]],&b=vv[f.v[1]],&c=vv[f.v[2]];s+=(long double)a.x*(b.y*c.z-b.z*c.y)+(long double)a.y*(b.z*c.x-b.x*c.z)+(long double)a.z*(b.x*c.y-b.y*c.x);}return (double)(s/6.0L);};double vo=signedVol(ov,of),vc=signedVol(outV,outF);if(vo*vc<0)for(Face&f:outF)swap(f.v[1],f.v[2]);return validateClosedOrientedHull(outV,outF);
}

bool buildDepthHullSurfaceNet(const vector<Vec3>&ov,const vector<Face>&of,int N,int R,vector<Vec3>&outV,vector<Face>&outF)const{
    HullAtlas at;buildHullAtlas(ov,of,R,at);
    Vec3 mn=ov[0],mx=ov[0];for(const Vec3&p:ov){mn.x=min(mn.x,p.x);mn.y=min(mn.y,p.y);mn.z=min(mn.z,p.z);mx.x=max(mx.x,p.x);mx.y=max(mx.y,p.y);mx.z=max(mx.z,p.z);}double pad=max(1e-6,0.025*diag);mn=mn-Vec3(pad,pad,pad);mx=mx+Vec3(pad,pad,pad);
    Vec3 step={(mx.x-mn.x)/N,(mx.y-mn.y)/N,(mx.z-mn.z)/N};double eps=0.55*max({step.x,step.y,step.z});
    int G=N+1;size_t nodes=(size_t)G*G*G;vector<unsigned char>inside(nodes,0);auto ni=[&](int x,int y,int z){return ((size_t)z*G+y)*G+x;};auto gp=[&](int x,int y,int z){return Vec3(mn.x+x*step.x,mn.y+y*step.y,mn.z+z*step.z);};
    for(int z=0;z<G;++z)for(int y=0;y<G;++y)for(int x=0;x<G;++x){if(x==0||y==0||z==0||x==N||y==N||z==N){inside[ni(x,y,z)]=0;continue;}inside[ni(x,y,z)]=insideDepthHull(gp(x,y,z),at,eps)?1:0;}
    vector<int>cell((size_t)N*N*N,-1);auto ci=[&](int x,int y,int z){return ((size_t)z*N+y)*N+x;};
    static const int C[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    static const int E[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    outV.clear();outF.clear();outV.reserve((size_t)4*N*N);
    for(int z=0;z<N;++z)for(int y=0;y<N;++y)for(int x=0;x<N;++x){unsigned char s[8];int sm=0;for(int q=0;q<8;++q){s[q]=inside[ni(x+C[q][0],y+C[q][1],z+C[q][2])];sm+=s[q];}if(sm==0||sm==8)continue;HullQef qef;Vec3 avg;int cnt=0;
        for(auto&e:E)if(s[e[0]]!=s[e[1]]){Vec3 a=gp(x+C[e[0]][0],y+C[e[0]][1],z+C[e[0]][2]),b=gp(x+C[e[1]][0],y+C[e[1]][1],z+C[e[1]][2]);bool ia=s[e[0]];for(int it=0;it<5;++it){Vec3 m=(a+b)*.5;bool im=insideDepthHull(m,at,eps);if(im==ia)a=m;else b=m;}Vec3 p=(a+b)*.5,n=hullConstraintNormal(p,at);avg=avg+p;++cnt;if(norm2(n)>1e-12)qef.add(n,p);}
        if(!cnt)continue;avg=avg/double(cnt);qef.anchor(avg,0.08*cnt);Vec3 p;if(!qef.solve(p))p=avg;Vec3 lo=gp(x,y,z),hi=gp(x+1,y+1,z+1);p.x=min(hi.x,max(lo.x,p.x));p.y=min(hi.y,max(lo.y,p.y));p.z=min(hi.z,max(lo.z,p.z));cell[ci(x,y,z)]=(int)outV.size();outV.push_back(p);
    }
    auto addQuad=[&](int a,int b,int c,int d,const Vec3&outward){if(a<0||b<0||c<0||d<0||a==b||b==c||c==d||d==a)return;int q[4]={a,b,c,d};Vec3 nn=cross(outV[q[1]]-outV[q[0]],outV[q[2]]-outV[q[0]]);if(dot(nn,outward)<0)swap(q[1],q[3]);double d02=norm2(outV[q[0]]-outV[q[2]]),d13=norm2(outV[q[1]]-outV[q[3]]);if(d02<=d13){outF.push_back(Face{{q[0],q[1],q[2]}});outF.push_back(Face{{q[0],q[2],q[3]}});}else{outF.push_back(Face{{q[0],q[1],q[3]}});outF.push_back(Face{{q[1],q[2],q[3]}});}};
    // x-directed grid edges; cyclic incident cells in yz plane.
    for(int z=1;z<N;++z)for(int y=1;y<N;++y)for(int x=0;x<N;++x){bool s0=inside[ni(x,y,z)],s1=inside[ni(x+1,y,z)];if(s0==s1)continue;int a=cell[ci(x,y-1,z-1)],b=cell[ci(x,y,z-1)],c=cell[ci(x,y,z)],d=cell[ci(x,y-1,z)];addQuad(a,b,c,d,Vec3(s0?1:-1,0,0));}
    // y-directed edges; cyclic incident cells in zx plane.
    for(int z=1;z<N;++z)for(int y=0;y<N;++y)for(int x=1;x<N;++x){bool s0=inside[ni(x,y,z)],s1=inside[ni(x,y+1,z)];if(s0==s1)continue;int a=cell[ci(x-1,y,z-1)],b=cell[ci(x-1,y,z)],c=cell[ci(x,y,z)],d=cell[ci(x,y,z-1)];addQuad(a,b,c,d,Vec3(0,s0?1:-1,0));}
    // z-directed edges; cyclic incident cells in xy plane.
    for(int z=0;z<N;++z)for(int y=1;y<N;++y)for(int x=1;x<N;++x){bool s0=inside[ni(x,y,z)],s1=inside[ni(x,y,z+1)];if(s0==s1)continue;int a=cell[ci(x-1,y-1,z)],b=cell[ci(x,y-1,z)],c=cell[ci(x,y,z)],d=cell[ci(x-1,y,z)];addQuad(a,b,c,d,Vec3(0,0,s0?1:-1));}
    auto signedVol=[&](const vector<Vec3>&vv,const vector<Face>&ff){long double s=0;for(const Face&f:ff){const Vec3&a=vv[f.v[0]],&b=vv[f.v[1]],&c=vv[f.v[2]];s+=(long double)a.x*(b.y*c.z-b.z*c.y)+(long double)a.y*(b.z*c.x-b.x*c.z)+(long double)a.z*(b.x*c.y-b.y*c.x);}return (double)(s/6.0L);};
    double vo=signedVol(ov,of),vc=signedVol(outV,outF);if(vo*vc<0)for(Face&f:outF)swap(f.v[1],f.v[2]);
    return validateClosedOrientedHull(outV,outF);
}

struct HullTri{Vec3 a,b,c,mn,mx,cent;};
struct HullBvhNode{Vec3 mn,mx;int l=-1,r=-1,b=0,e=0;};

static Vec3 hullClosestPointTri(const Vec3&p,const Vec3&a,const Vec3&b,const Vec3&c){
    Vec3 ab=b-a,ac=c-a,ap=p-a;double d1=dot(ab,ap),d2=dot(ac,ap);if(d1<=0&&d2<=0)return a;
    Vec3 bp=p-b;double d3=dot(ab,bp),d4=dot(ac,bp);if(d3>=0&&d4<=d3)return b;
    double vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0){double v=d1/(d1-d3);return a+ab*v;}
    Vec3 cp=p-c;double d5=dot(ab,cp),d6=dot(ac,cp);if(d6>=0&&d5<=d6)return c;
    double vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0){double w=d2/(d2-d6);return a+ac*w;}
    double va=d3*d6-d5*d4;if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0){double w=(d4-d3)/((d4-d3)+(d5-d6));return b+(c-b)*w;}
    double den=1.0/(va+vb+vc);double v=vb*den,w=vc*den;return a+ab*v+ac*w;
}
class HullBvh{
public:
    vector<HullTri>t;vector<int>id;vector<HullBvhNode>n;
    explicit HullBvh(const vector<Vec3>&v,const vector<Face>&f){t.reserve(f.size());for(auto&x:f){HullTri q{v[x.v[0]],v[x.v[1]],v[x.v[2]]};q.mn={min({q.a.x,q.b.x,q.c.x}),min({q.a.y,q.b.y,q.c.y}),min({q.a.z,q.b.z,q.c.z})};q.mx={max({q.a.x,q.b.x,q.c.x}),max({q.a.y,q.b.y,q.c.y}),max({q.a.z,q.b.z,q.c.z})};q.cent=(q.a+q.b+q.c)/3.0;t.push_back(q);}id.resize(t.size());iota(id.begin(),id.end(),0);if(!id.empty())build(0,id.size());}
    int build(int b,int e){HullBvhNode q;q.b=b;q.e=e;q.mn=Vec3(CParam_Inf,CParam_Inf,CParam_Inf);q.mx=Vec3(-CParam_Inf,-CParam_Inf,-CParam_Inf);for(int i=b;i<e;++i){auto&x=t[id[i]];q.mn.x=min(q.mn.x,x.mn.x);q.mn.y=min(q.mn.y,x.mn.y);q.mn.z=min(q.mn.z,x.mn.z);q.mx.x=max(q.mx.x,x.mx.x);q.mx.y=max(q.mx.y,x.mx.y);q.mx.z=max(q.mx.z,x.mx.z);}int me=n.size();n.push_back(q);if(e-b>8){Vec3 d=q.mx-q.mn;int ax=d.y>d.x?1:0;if((ax==0?d.x:d.y)<d.z)ax=2;int m=(b+e)/2;nth_element(id.begin()+b,id.begin()+m,id.begin()+e,[&](int i,int j){return ax==0?t[i].cent.x<t[j].cent.x:ax==1?t[i].cent.y<t[j].cent.y:t[i].cent.z<t[j].cent.z;});n[me].l=build(b,m);n[me].r=build(m,e);}return me;}
    static double boxD2(const Vec3&p,const Vec3&a,const Vec3&b){double s=0,z;if(p.x<a.x){z=a.x-p.x;s+=z*z;}else if(p.x>b.x){z=p.x-b.x;s+=z*z;}if(p.y<a.y){z=a.y-p.y;s+=z*z;}else if(p.y>b.y){z=p.y-b.y;s+=z*z;}if(p.z<a.z){z=a.z-p.z;s+=z*z;}else if(p.z>b.z){z=p.z-b.z;s+=z*z;}return s;}
    double nearest2(const Vec3&p,double best=CParam_Inf)const{if(n.empty())return best;vector<int>st(1,0);while(!st.empty()){int u=st.back();st.pop_back();auto&q=n[u];if(boxD2(p,q.mn,q.mx)>=best)continue;if(q.l<0){for(int i=q.b;i<q.e;++i){auto&x=t[id[i]];best=min(best,pointTriangleDistance2(p,x.a,x.b,x.c));}}else{double dl=boxD2(p,n[q.l].mn,n[q.l].mx),dr=boxD2(p,n[q.r].mn,n[q.r].mx);if(dl<dr){if(dr<best)st.push_back(q.r);if(dl<best)st.push_back(q.l);}else{if(dl<best)st.push_back(q.l);if(dr<best)st.push_back(q.r);}}}return best;}
    Vec3 nearestPoint(const Vec3&p,double*dist2=nullptr)const{double best=CParam_Inf;Vec3 bp=p;if(n.empty()){if(dist2)*dist2=best;return bp;}vector<int>st(1,0);while(!st.empty()){int u=st.back();st.pop_back();auto&q=n[u];if(boxD2(p,q.mn,q.mx)>=best)continue;if(q.l<0){for(int i=q.b;i<q.e;++i){auto&x=t[id[i]];Vec3 z=hullClosestPointTri(p,x.a,x.b,x.c);double d=norm2(p-z);if(d<best){best=d;bp=z;}}}else{double dl=boxD2(p,n[q.l].mn,n[q.l].mx),dr=boxD2(p,n[q.r].mn,n[q.r].mx);if(dl<dr){if(dr<best)st.push_back(q.r);if(dl<best)st.push_back(q.l);}else{if(dl<best)st.push_back(q.l);if(dr<best)st.push_back(q.r);}}}if(dist2)*dist2=best;return bp;}
};

bool sampledHullHausdorff(const vector<Vec3>&aV,const vector<Face>&aF,const vector<Vec3>&bV,const vector<Face>&bF,double limit)const{
    HullBvh A(aV,aF),B(bV,bF);double l2=limit*limit;
    auto check=[&](const vector<Vec3>&v,const vector<Face>&f,const HullBvh&dst){for(const Vec3&p:v)if(dst.nearest2(p,l2)>=l2)return false;for(const Face&x:f){Vec3 a=v[x.v[0]],b=v[x.v[1]],c=v[x.v[2]];Vec3 s[4]={(a+b+c)/3.0,(a+b)*.5,(b+c)*.5,(c+a)*.5};for(Vec3&p:s)if(dst.nearest2(p,l2)>=l2)return false;}return true;};
    return check(aV,aF,B)&&check(bV,bF,A);
}

bool depthHullRenderGuard(const vector<Vec3>&ov,const vector<Face>&of,const vector<Vec3>&cv,const vector<Face>&cf)const{
    int R=384;vector<RefImage>ref(6);for(int v=0;v<6;++v)ref[v]=renderReference(ov,of,v,R);RefScore s=compareToReference(ref,cv,cf,R);
    return s.finalScore>=0.945&&s.minView>=0.915&&s.minNormal>=0.885&&s.minDepth>=0.975;
}

void runDepthHullT2(){
    // Preserve the original mesh for direct image-space and Hausdorff validation.
    vector<Vec3> originalV=verts;
    vector<Face> originalF=faces;

    // Produce the exact trusted v23 T2 result first. This is always the fallback.
    runTransactionalScreenMid();
    vector<Vec3> safeV=verts;
    vector<Face> safeF=faces;
    const double originalDiag=diag;

    if(elapsed()>HParam_TimeBudgetSeconds-7.0||safeV.size()<80)return;

    // Independent topology: six-view front/back depth hull -> marching tetrahedra.
    int gridN=(int)llround(sqrt(max(1.0,double(originalV.size())/4.0)));
    gridN=max(28,min(48,gridN));
    vector<Vec3> candidateV;
    vector<Face> candidateF;
    if(!buildDepthHullMarchingTets(originalV,originalF,gridN,256,candidateV,candidateF))return;
    if(elapsed()>HParam_TimeBudgetSeconds-5.0)return;

    // Pull the new connectivity close to the true input surface without allowing
    // the dense pre-remesh to collapse onto identical original samples.
    HullBvh originalBvh(originalV,originalF);
    for(Vec3&p:candidateV){
        Vec3 q=originalBvh.nearestPoint(p);
        p=p*0.10+q*0.90;
    }
    if(!validateClosedOrientedHull(candidateV,candidateF))return;

    // Run a fresh screen-weighted QEM trajectory on the new connectivity.
    verts=candidateV;faces=candidateF;nV=(int)verts.size();nF=(int)faces.size();
    vdead.clear();fdead.clear();vver.clear();vquad.clear();vmoment.clear();
    crad.clear();vfaces.clear();vneigh.clear();tailLocks.clear();
    facePix.clear();faceSil.clear();faceWin.clear();vertAnchor.clear();
    accepted=0;tailCursor=0;tailStamp=1;lastFailBatch=-1;starCursor=vegaCursor=0;
    screenTier=2;priority_queue<CollapseCandidate> empty;pq.swap(empty);
    initScale();buildConnectivity();vmoment.assign(nV,Quadric());
    buildRasterImportance(384);initFaceWeights();costCap*=12.0;
    targetV=max(10,(int)floor(safeV.size()*0.88));
    targetV=min(targetV,nV-1);collapseLimit=max(0,nV-targetV);
    initQueue();collapseLoop();compact();

    candidateV=verts;candidateF=faces;

    // Reproject the coarse vertices, then perform one projected tangential
    // redistribution step to improve triangle quality and normal fidelity.
    for(Vec3&p:candidateV)p=originalBvh.nearestPoint(p);
    vector<vector<int>> adj(candidateV.size()),vf(candidateV.size());
    for(int fi=0;fi<(int)candidateF.size();++fi){
        const Face&f=candidateF[fi];
        for(int k=0;k<3;++k){
            vf[f.v[k]].push_back(fi);
            int a=f.v[k],b=f.v[(k+1)%3];
            adj[a].push_back(b);adj[b].push_back(a);
        }
    }
    for(auto&a:adj){sort(a.begin(),a.end());a.erase(unique(a.begin(),a.end()),a.end());}
    vector<Vec3> proposed=candidateV;
    for(int i=0;i<(int)candidateV.size();++i){
        if(adj[i].empty())continue;
        Vec3 avg;
        for(int j:adj[i])avg=avg+candidateV[j];
        avg=avg/double(adj[i].size());
        Vec3 q=originalBvh.nearestPoint(candidateV[i]*0.55+avg*0.45);
        bool good=true;
        for(int fi:vf[i]){
            Face f=candidateF[fi];
            Vec3 a=candidateV[f.v[0]],b=candidateV[f.v[1]],c=candidateV[f.v[2]];
            Vec3 oldN=cross(b-a,c-a);
            if(f.v[0]==i)a=q;if(f.v[1]==i)b=q;if(f.v[2]==i)c=q;
            Vec3 newN=cross(b-a,c-a);
            if(norm2(newN)<1e-20||dot(oldN,newN)<=0){good=false;break;}
        }
        if(good)proposed[i]=q;
    }
    candidateV.swap(proposed);

    bool accept=candidateV.size()<safeV.size()&&
                validateClosedOrientedHull(candidateV,candidateF);
    if(accept&&elapsed()<HParam_TimeBudgetSeconds-2.0)
        accept=sampledHullHausdorff(originalV,originalF,candidateV,candidateF,
                                    0.038*originalDiag);
    else accept=false;

    if(accept&&elapsed()<HParam_TimeBudgetSeconds-0.8){
        constexpr int R=512;
        vector<RefImage> reference(6);
        for(int view=0;view<6;++view)
            reference[view]=renderReference(originalV,originalF,view,R);
        RefScore s=compareToReference(reference,candidateV,candidateF,R);
        accept=s.finalScore>=0.935&&s.minView>=0.920&&
               s.minNormal>=0.870&&s.minDepth>=0.980;
    }else accept=false;

    if(accept){
        verts.swap(candidateV);faces.swap(candidateF);
    }else{
        verts.swap(safeV);faces.swap(safeF);
    }
    nV=(int)verts.size();nF=(int)faces.size();
    vdead.clear();fdead.clear();
}

void compactRebuildPreserve(){
vector<int>mapv(verts.size(),-1);vector<Vec3>nv;vector<double>nr;nv.reserve(verts.size());nr.reserve(verts.size());
for(int i=0;i<(int)verts.size();++i)if(i>=(int)vdead.size()||!vdead[i]){mapv[i]=(int)nv.size();nv.push_back(verts[i]);nr.push_back(i<(int)crad.size()?crad[i]:0.0);}
vector<Face>nf;nf.reserve(faces.size());
for(int fi=0;fi<(int)faces.size();++fi){if(fi<(int)fdead.size()&&fdead[fi])continue;const Face&f=faces[fi];if(f.v[0]<0||f.v[1]<0||f.v[2]<0||f.v[0]>=(int)mapv.size()||f.v[1]>=(int)mapv.size()||f.v[2]>=(int)mapv.size())continue;int a=mapv[f.v[0]],b=mapv[f.v[1]],c=mapv[f.v[2]];if(a<0||b<0||c<0||a==b||b==c||a==c)continue;Face q;q.v[0]=a;q.v[1]=b;q.v[2]=c;nf.push_back(q);}
verts.swap(nv);faces.swap(nf);crad.swap(nr);nV=(int)verts.size();nF=(int)faces.size();
vdead.assign(nV,0);fdead.assign(nF,0);vver.assign(nV,0);vquad.assign(nV,Quadric());vmoment.assign(nV,Quadric());vfaces.assign(nV,{});vneigh.assign(nV,SmallSet());tailLocks.assign(nV,0);facePix.clear();faceSil.clear();faceWin.clear();vertAnchor.clear();screenTier=0;faceImpMean=0;
for(int fi=0;fi<nF;++fi){Face&f=faces[fi];Quadric q=Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);for(int k=0;k<3;++k){vfaces[f.v[k]].push_back(fi);vquad[f.v[k]]+=q;}for(int k=0;k<3;++k){int a=f.v[k],b=f.v[(k+1)%3];if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);}}}
priority_queue<CollapseCandidate>empty;pq.swap(empty);accepted=0;targetV=nV;collapseLimit=0;tailCursor=0;tailStamp=1;lastFailBatch=-1;starCursor=vegaCursor=0;
}
void continueLargeQemTo(int originalN,double ratio){
initFaceWeights();priority_queue<CollapseCandidate>empty;pq.swap(empty);accepted=0;targetV=max(10,(int)floor(originalN*ratio));targetV=min(targetV,nV);collapseLimit=max(0,nV-targetV);initQueue();collapseLoop();
}
bool largeDeltaGuard(const RefScore&s,int tier)const{
if(tier==5)return s.finalScore>=.9970&&s.minView>=.9955&&s.minNormal>=.9930&&s.minDepth>=.9990;
return s.finalScore>=.9975&&s.minView>=.9960&&s.minNormal>=.9940&&s.minDepth>=.9990;
}
int largeStarPass(int originalN,int tier,double scale=1.0){
StarParams sp=tier==5?StarParams{5,0.006,0.009,0.46,0.0022,1300,115000,1,0.27,0.95}:StarParams{6,0.008,0.012,0.52,0.0030,1800,140000,2,0.32,1.20};
sp.maxOldDev*=scale;sp.maxNewDev*=scale;sp.distFrac*=min(1.12,scale);double timeLeft=HParam_TimeBudgetSeconds-elapsed();if(timeLeft<0.25)return 0;double stopTime=elapsed()+min(sp.maxSeconds,timeLeft*sp.timeFrac);int maxExtra=min(sp.hardCap,max(0,(int)floor(originalN*sp.extraFrac)));int extra=0;
for(int round=0;round<sp.rounds&&extra<maxExtra&&elapsed()<stopTime;++round){vector<StarCandidate>cands;cands.reserve(8192);int scanned=0,visited=0,total=(int)verts.size(),start=total?starCursor%total:0;for(;visited<total&&scanned<sp.scanVertices&&elapsed()<stopTime;++visited){int v=(start+visited)%total;if(vdead[v])continue;++scanned;StarCandidate c=computeStarCandidateWithParams(v,sp);if(c.valid())cands.push_back(c);}if(total)starCursor=(start+max(1,visited))%total;if(cands.empty())break;sort(cands.begin(),cands.end());bool progress=false;for(const StarCandidate&c:cands){if(extra>=maxExtra||elapsed()>=stopTime)break;if(vdead[c.v])continue;if(applyStarDeleteWithParams(c.v,c.root,sp)){++accepted;++extra;progress=true;}}if(!progress)break;}return extra;
}
void runLargeCameraTx(int originalN){
int tier=originalN<=400000?5:6;compactRebuildPreserve();
if(nV<20||elapsed()>HParam_TimeBudgetSeconds-1.0){compact();return;}
vector<Vec3>safeV,candV;vector<Face>safeF,candF;extractActiveMesh(safeV,safeF);int R=tier==5?256:192;vector<RefImage>ref(6);for(int v=0;v<6;++v)ref[v]=renderReference(safeV,safeF,v,R);MidSnap base=midSnapshot();
if(elapsed()<HParam_TimeBudgetSeconds-0.80){int hidden=occludedEdgeCollapsePass(tier);if(hidden>0){extractActiveMesh(candV,candF);RefScore hs=compareToReference(ref,candV,candF,R);if(!largeDeltaGuard(hs,tier))restoreMid(base);}}
MidSnap hiddenBase=midSnapshot();vector<double>tries=tier==5?vector<double>{.018,.020,.0215,.0225}:vector<double>{.026,.028,.030,.031};bool qemKept=false;
for(double ratio:tries){if(elapsed()>HParam_TimeBudgetSeconds-0.60)break;restoreMid(hiddenBase);continueLargeQemTo(originalN,ratio);extractActiveMesh(candV,candF);RefScore sc=compareToReference(ref,candV,candF,R);if(largeDeltaGuard(sc,tier)){qemKept=true;break;}}
if(!qemKept)restoreMid(hiddenBase);
MidSnap preStar=midSnapshot();if(elapsed()<HParam_TimeBudgetSeconds-0.32){int sd=largeStarPass(originalN,tier,1.0);if(sd>0){extractActiveMesh(candV,candF);RefScore ss=compareToReference(ref,candV,candF,R);if(!largeDeltaGuard(ss,tier))restoreMid(preStar);}}
compact();
}

void initScale(){

// Per-tier algorithm dispatch: memoryless QEM ONLY for medium tiers.

// T7 (and T6, T2) get MEMLESS=false → EXACTLY the 88.83 champion path.

MEMLESS = (nV>5000);  // advisor T4: also enable memoryless on T7

Vec3 mn=verts[0],mx=verts[0];

for(auto&p:verts){mn.x=min(mn.x,p.x);mn.y=min(mn.y,p.y);mn.z=min(mn.z,p.z);mx.x=max(mx.x,p.x);mx.y=max(mx.y,p.y);mx.z=max(mx.z,p.z);}

diag=norm(mx-mn);hausd=CParam_HausdorffDiagFraction*diag;

costCap=HParam_QemCostCapCoeff*diag*diag;

invDiag2 = (diag > CParam_MinNormalNorm) ? (1.0 / (diag*diag)) : 0.0;

double kr;

if(nV<=5000)kr=HParam_KeepRatio_UpTo5k;

else if(nV<=25000)kr=HParam_KeepRatio_UpTo25k;

else if(nV<=45000)kr=HParam_KeepRatio_UpTo45k;

else if(nV<=50000)kr=HParam_KeepRatio_UpTo50k;

else if(nV<=400000)kr=HParam_KeepRatio_UpTo400k;

else kr=HParam_KeepRatio_Huge;

targetV=max(10,(int)floor(nV*kr));

targetV=min(targetV,nV-1);

collapseLimit=nV-targetV;

}

void buildConnectivity(){

vdead.assign(nV,0);fdead.assign(nF,0);vver.assign(nV,0);

vquad.assign(nV,Quadric());crad.assign(nV,0.0);

vfaces.assign(nV,{});vneigh.resize(nV);tailLocks.assign(nV,0);

for(int fi=0;fi<nF;++fi){

auto&f=faces[fi];

Quadric q=Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);

for(int k=0;k<3;++k){vfaces[f.v[k]].push_back(fi);vquad[f.v[k]]+=q;}

for(int k=0;k<3;++k){int a=f.v[k],b=f.v[(k+1)%3];if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);}}

}

}

double faceWeightFor(const Vec3& unitNormal, double area) const {

double absSum = fabs(unitNormal.x) + fabs(unitNormal.y) + fabs(unitNormal.z);

double normalizedArea = area * invDiag2;

double w = 1.0 + CParam_ViewWeightK * normalizedArea * absSum;

return min(w, CParam_MaxFaceWeight);

}

Quadric anchorPointQuadric(const Vec3&p,double w)const{Quadric q;q.a=q.e=q.h=w;q.d=-w*p.x;q.g=-w*p.y;q.i=-w*p.z;q.j=w*norm2(p);return q;}

void initFaceWeights(){

vquad.assign(nV, Quadric());
faceImpMean=0.0;int activeF=0;
if(screenTier)for(int fi=0;fi<nF;++fi)if(!(fi<(int)fdead.size()&&fdead[fi])){faceImpMean+=screenTier==3?(facePix[fi]+10.0*faceSil[fi]):(facePix[fi]+12.0*faceSil[fi]+2.0*faceWin[fi]);++activeF;}
if(screenTier)faceImpMean/=max(1,activeF);
vertAnchor.assign(nV,0.0);if(screenTier==2||screenTier==4)for(int fi=0;fi<nF;++fi)if(!(fi<(int)fdead.size()&&fdead[fi]))for(int k=0;k<3;++k)vertAnchor[faces[fi].v[k]]+=faceSil[fi]+0.05*facePix[fi]+0.10*faceWin[fi];

for(int fi=0;fi<nF;++fi){
if(fi<(int)fdead.size()&&fdead[fi])continue;
const Face& f=faces[fi];
Vec3 n=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);
double area=0.5*norm(n);
Quadric q = Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);
if(screenTier){
double imp=screenTier==3?(facePix[fi]+10.0*faceSil[fi]):(facePix[fi]+12.0*faceSil[fi]+2.0*faceWin[fi]);double r=screenTier==3?(imp+0.20*faceImpMean)/(faceImpMean+1e-12):(imp+0.12*faceImpMean)/(faceImpMean+1e-12);
double w;if(screenTier==2)w=min(18.0,max(0.06,0.08+0.92*pow(max(0.0,r),0.90)));else if(screenTier==3)w=min(7.0,max(0.12,0.15+0.85*sqrt(max(0.0,r))));else w=min(16.0,max(0.06,0.08+0.92*pow(max(0.0,r),0.72)));
if(w!=1.0)q.scale(w);
}else if(area >= 1e-30){n=n/(2.0*area);double w=faceWeightFor(n,area);if(w!=1.0)q.scale(w);}
for(int k=0;k<3;++k)vquad[f.v[k]]+=q;
}
if(screenTier==2||screenTier==4){double am=0;int ac=0;for(int i=0;i<nV;++i)if(!vdead[i]){am+=vertAnchor[i];++ac;}am/=max(1,ac);double bw=(screenTier==2?0.0000035:0.0000050)*diag*diag/(am+1.0);for(int i=0;i<nV;++i)if(!vdead[i]&&vertAnchor[i]>0)vquad[i]+=anchorPointQuadric(verts[i],bw*min(18.0,vertAnchor[i]/(am+1e-12)));if(screenTier==4)for(int i=0;i<nV;++i)if(!vdead[i])vquad[i]+=vmoment[i];}
}

Quadric weightedFaceQuadric(int fi)const{
const Face&f=faces[fi];Vec3 n=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);double area=0.5*norm(n);Quadric q=Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);
if(screenTier&&fi<(int)facePix.size()){double imp=facePix[fi]+12.0*faceSil[fi]+2.0*faceWin[fi];double r=(imp+0.12*faceImpMean)/(faceImpMean+1e-12);double w=screenTier==2?min(18.0,max(0.06,0.08+0.92*pow(max(0.0,r),0.90))):min(16.0,max(0.06,0.08+0.92*pow(max(0.0,r),0.72)));if(w!=1.0)q.scale(w);}else if(area>=1e-30){n=n/(2.0*area);double w=faceWeightFor(n,area);if(w!=1.0)q.scale(w);}return q;
}

bool isFaceInvisible(int fi) const {

const Face& f = faces[fi];

Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);

double len = norm(n);

if (len < 1e-30) return false;

n = n / len;

for (int k = 0; k < 6; ++k) {

if (dot(n, cameraDirs[k]) > 0) return false;

}

return true;

}

void getCandidatePositions(int a,int b,const Quadric&q,Vec3 pos[4],int&np)const{

np=0;

Vec3 qp;

if(solveQem3x3(q,qp))pos[np++]=qp;

pos[np++]=(verts[a]+verts[b])*0.5;

pos[np++]=verts[a];

pos[np++]=verts[b];

int wp=0;

for(int i=0;i<np;++i){

if(!finiteVec(pos[i]))continue;

bool dup=false;

for(int j=0;j<wp;++j)if(norm2(pos[i]-pos[j])<1e-30){dup=true;break;}

if(!dup)pos[wp++]=pos[i];

}

np=wp;

}

CollapseCandidate makeCandidate(int ab,int kp,const Vec3&p,const Quadric&q)const{

CollapseCandidate c;c.absorbed=ab;c.kept=kp;

c.versionAbsorbed=vver[ab];c.versionKept=vver[kp];

c.position=p;c.cost=q.evaluate(p);return c;

}

double cheapEdgeCost(int a,int b)const{

// Fast lower-cost proxy: quadric error at the edge midpoint (no 3x3 solve,

// no multi-position search). Used only to PRE-RANK/FILTER tail candidates;

// the full 8-placement solve still runs in computeBestValid for accepted ones.

Quadric q=vquad[a];q+=vquad[b];

Vec3 mid=(verts[a]+verts[b])*0.5;

return q.evaluate(mid);

}

CollapseCandidate computeQueueCandidate(int a,int b)const{

Quadric q=vquad[a];q+=vquad[b];

Vec3 pos[4];int np;getCandidatePositions(a,b,q,pos,np);

CollapseCandidate best;

for(int i=0;i<np;++i){

CollapseCandidate c1=makeCandidate(a,b,pos[i],q);if(c1.cost<best.cost)best=c1;

CollapseCandidate c2=makeCandidate(b,a,pos[i],q);if(c2.cost<best.cost)best=c2;

}

return best;

}

bool passesEnvelope(int a,int b,const Vec3&p,double&mr)const{

mr=max(crad[a]+norm(verts[a]-p),crad[b]+norm(verts[b]-p));

return mr<=hausd;

}

CollapseCandidate computeBestValid(int a,int b)const{

Quadric q=vquad[a];q+=vquad[b];

Vec3 pos[4];int np;getCandidatePositions(a,b,q,pos,np);

CollapseCandidate best;

for(int i=0;i<np;++i){

for(int dir=0;dir<2;++dir){

int ab=dir?b:a,kp=dir?a:b;

double mr;

if(!passesEnvelope(ab,kp,pos[i],mr))continue;

CollapseCandidate c=makeCandidate(ab,kp,pos[i],q);

c.mergedRadius=mr;

if(c.cost<best.cost)best=c;

}

}

return best;

}

void initQueue(){

for(int a=0;a<nV;++a)

for(int b:vneigh[a])if(a<b){auto c=computeQueueCandidate(a,b);if(c.valid())pq.push(c);}

}

double elapsed()const{return chrono::duration<double>(chrono::steady_clock::now()-startTime).count();}

bool tailMode()const{if(nV<=HParam_TailOriginalVertexThreshold)return false;double e=elapsed();return e>HParam_TailBatchElapsedStart&&e<HParam_TailBatchStopElapsed;}

bool edgeExists(int a,int b)const{return a>=0&&b>=0&&a<(int)verts.size()&&b<(int)verts.size()&&!vdead[a]&&!vdead[b]&&vneigh[a].contains(b);}

int countCommonFaces(int a,int b)const{

int cnt=0;

const auto&fa=vfaces[a];const auto&fb=vfaces[b];

if(fa.size()<fb.size()){for(int f:fa){if(!fdead[f])for(int f2:fb)if(f==f2){++cnt;break;}}}

else{for(int f:fb){if(!fdead[f])for(int f2:fa)if(f==f2){++cnt;break;}}}

return cnt;

}

int countCommonNeighbors(int a,int b)const{

int cnt=0;

const auto&na=vneigh[a];const auto&nb=vneigh[b];

if(na.size()<nb.size()){for(int x:na){if(x!=a&&x!=b&&!vdead[x]&&nb.contains(x))++cnt;}}

else{for(int x:nb){if(x!=a&&x!=b&&!vdead[x]&&na.contains(x))++cnt;}}

return cnt;

}

static void eraseVal(vector<int>&v,int x){for(int i=(int)v.size()-1;i>=0;--i)if(v[i]==x){v[i]=v.back();v.pop_back();return;}}

bool batchFree(int a,int b)const{

if(tailLocks[a]==tailStamp||tailLocks[b]==tailStamp)return false;

for(int nb:vneigh[a])if(!vdead[nb]&&tailLocks[nb]==tailStamp)return false;

for(int nb:vneigh[b])if(!vdead[nb]&&tailLocks[nb]==tailStamp)return false;

return true;

}

void lockBatch(int a,int b){

tailLocks[a]=tailLocks[b]=tailStamp;

for(int nb:vneigh[a])if(!vdead[nb])tailLocks[nb]=tailStamp;

for(int nb:vneigh[b])if(!vdead[nb])tailLocks[nb]=tailStamp;

}

struct EBC{int a,b;double cost;bool operator<(const EBC&o)const{return cost<o.cost;}};

int runTailBatch(){

if(!tailMode())return 0;

if(++tailStamp==INT_MAX){fill(tailLocks.begin(),tailLocks.end(),0);tailStamp=1;}

vector<EBC> cands;cands.reserve(HParam_TailBatchScanEdges/4);

int scanned=0,visited=0;

for(;visited<(int)verts.size()&&scanned<HParam_TailBatchScanEdges;++visited){

int a=(tailCursor+visited)%(int)verts.size();if(vdead[a])continue;

for(int b:vneigh[a]){if(scanned>=HParam_TailBatchScanEdges)break;if(b<=a||vdead[b])continue;++scanned;

double cc=cheapEdgeCost(a,b);if(!(cc<=costCap))continue;cands.push_back({a,b,cc});}

}

tailCursor=(tailCursor+max(1,visited))%(int)verts.size();

if(cands.empty())return 0;

sort(cands.begin(),cands.end());

vector<EBC> sel;sel.reserve(HParam_TailBatchTargetAccepts);

for(auto&e:cands){if((int)sel.size()>=HParam_TailBatchTargetAccepts)break;if(!edgeExists(e.a,e.b))continue;if(!batchFree(e.a,e.b))continue;lockBatch(e.a,e.b);sel.push_back(e);}

int acc=0;

for(auto&e:sel){

if(accepted>=collapseLimit||elapsed()>HParam_TailBatchStopElapsed)break;

if(!edgeExists(e.a,e.b))continue;

if(countCommonFaces(e.a,e.b)!=2)continue;

if(countCommonNeighbors(e.a,e.b)!=2)continue;

auto best=computeBestValid(e.a,e.b);if(!best.valid()||best.cost>costCap)continue;

applyCollapse(best.absorbed,best.kept,best.position,best.mergedRadius);++accepted;++acc;

}

return acc;

}

void collapseLoop(){

int tick=0;

while(accepted<collapseLimit&&(!pq.empty()||tailMode())){

if((++tick&8191)==0&&elapsed()>HParam_TimeBudgetSeconds)break;

if(tailMode()&&lastFailBatch!=accepted){int ba=runTailBatch();if(ba>0)continue;lastFailBatch=accepted;}

if(pq.empty())break;

auto c=pq.top();pq.pop();

int a=c.absorbed,b=c.kept;

if(!edgeExists(a,b))continue;

if(c.versionAbsorbed!=vver[a]||c.versionKept!=vver[b]){auto fr=computeQueueCandidate(a,b);if(fr.valid())pq.push(fr);continue;}

if(c.cost>costCap)break;

if(countCommonFaces(a,b)!=2)continue;

if(countCommonNeighbors(a,b)!=2)continue;

auto best=computeBestValid(a,b);if(!best.valid()||best.cost>costCap)continue;

applyCollapse(best.absorbed,best.kept,best.position,best.mergedRadius);++accepted;

}

}

void applyCollapse(int ab,int kp,const Vec3&np,double nr){

verts[kp]=np;crad[kp]=nr;crad[ab]=0;vdead[ab]=1;++vver[ab];++vver[kp];

auto abFaces=vfaces[ab];vector<int> dead;dead.reserve(4);

for(int fi:abFaces){if(fdead[fi])continue;bool touched=false;

for(int k=0;k<3;++k)if(faces[fi].v[k]==ab){faces[fi].v[k]=kp;touched=true;}

if(!touched)continue;

if(faces[fi].v[0]==faces[fi].v[1]||faces[fi].v[1]==faces[fi].v[2]||faces[fi].v[0]==faces[fi].v[2]){fdead[fi]=1;dead.push_back(fi);}

else vfaces[kp].push_back(fi);}

for(int fi:dead)for(int k=0;k<3;++k){int v=faces[fi].v[k];if(v>=0&&v<(int)vfaces.size())eraseVal(vfaces[v],fi);}

vfaces[ab].clear();

if(kp<(int)vmoment.size()&&ab<(int)vmoment.size())vmoment[kp]+=vmoment[ab];

if(MEMLESS){

Quadric fresh;

for(int fi:vfaces[kp]){if(fdead[fi])continue;const Face&f=faces[fi];

fresh+=((screenTier==2||screenTier==4)?weightedFaceQuadric(fi):Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]));}

if(screenTier==4&&kp<(int)vmoment.size())fresh+=vmoment[kp];vquad[kp]=fresh;

} else {

vquad[kp]+=vquad[ab];

}

for(int nb:vneigh[ab]){if(nb==kp||vdead[nb])continue;vneigh[nb].erase(ab);vneigh[nb].insert(kp);vneigh[kp].insert(nb);}

vneigh[ab].clear();vneigh[kp].erase(ab);vneigh[kp].erase(kp);

for(int nb:vneigh[kp]){if(nb==kp||vdead[nb])continue;auto c=computeQueueCandidate(kp,nb);if(c.valid())pq.push(c);}

}

// ---------- Star-delete retriangulation post-pass ----------

static double clampDouble(double x,double lo,double hi){ return x<lo?lo:(x>hi?hi:x); }

static double pointTriangleDistance2(const Vec3& p,const Vec3& a,const Vec3& b,const Vec3& c){

Vec3 ab=b-a, ac=c-a, ap=p-a;

double d1=dot(ab,ap), d2=dot(ac,ap);

if(d1<=0.0&&d2<=0.0)return norm2(ap);

Vec3 bp=p-b;

double d3=dot(ab,bp), d4=dot(ac,bp);

if(d3>=0.0&&d4<=d3)return norm2(bp);

double vc=d1*d4-d3*d2;

if(vc<=0.0&&d1>=0.0&&d3<=0.0){ double v=d1/(d1-d3); Vec3 q=a+ab*v; return norm2(p-q); }

Vec3 cp=p-c;

double d5=dot(ab,cp), d6=dot(ac,cp);

if(d6>=0.0&&d5<=d6)return norm2(cp);

double vb=d5*d2-d1*d6;

if(vb<=0.0&&d2>=0.0&&d6<=0.0){ double w=d2/(d2-d6); Vec3 q=a+ac*w; return norm2(p-q); }

double va=d3*d6-d5*d4;

if(va<=0.0&&(d4-d3)>=0.0&&(d5-d6)>=0.0){ double w=(d4-d3)/((d4-d3)+(d5-d6)); Vec3 q=b+(c-b)*w; return norm2(p-q); }

Vec3 n=cross(ab,ac);

double nn=norm2(n);

if(nn<CParam_MinNormalNorm*CParam_MinNormalNorm)return CParam_Inf;

double dist=dot(p-a,n);

return (dist*dist)/nn;

}

struct HiddenAtlas{
int R=0;array<vector<float>,6> z;vector<char> visibleFace,protectedVert;
};

void buildHiddenAtlas(HiddenAtlas& h,int R)const{
h.R=R;h.visibleFace.assign(nF,0);
for(int view=0;view<6;++view){h.z[view].assign((size_t)R*R,1e30f);vector<int>id((size_t)R*R,-1);
for(int fi=0;fi<nF;++fi){if(fdead[fi])continue;const Face&f=faces[fi];auto a=projectImp(verts[f.v[0]],view,R),b=projectImp(verts[f.v[1]],view,R),c=projectImp(verts[f.v[2]],view,R);if(!a.ok||!b.ok||!c.ok)continue;double den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);if(fabs(den)<1e-18)continue;int x0=max(0,(int)floor(min({a.u,b.u,c.u}))),x1=min(R-1,(int)ceil(max({a.u,b.u,c.u}))),y0=max(0,(int)floor(min({a.v,b.v,c.v}))),y1=min(R-1,(int)ceil(max({a.v,b.v,c.v})));for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){double X=x+.5,Y=y+.5,w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den,w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den,w2=1-w0-w1;if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;double iz=w0/a.z+w1/b.z+w2/c.z;if(iz<=0)continue;float zz=(float)(1.0/iz);int q=y*R+x;if(zz<h.z[view][q]){h.z[view][q]=zz;id[q]=fi;}}}
for(int q=0;q<R*R;++q)if(id[q]>=0)h.visibleFace[id[q]]=1;}
h.protectedVert.assign(nV,0);for(int fi=0;fi<nF;++fi)if(!fdead[fi]&&h.visibleFace[fi])for(int k=0;k<3;++k)h.protectedVert[faces[fi].v[k]]=1;
vector<char>p=h.protectedVert;for(int v=0;v<nV;++v)if(p[v]&&!vdead[v])for(int nb:vneigh[v])if(!vdead[nb])h.protectedVert[nb]=1;
}

bool hiddenLocalGeometrySafe(const CollapseCandidate& c,vector<int>&patch,vector<array<Vec3,3>>&oldTris,vector<array<Vec3,3>>&newTris)const{
int a=c.absorbed,b=c.kept;patch.clear();for(int fi:vfaces[a])if(!fdead[fi])patch.push_back(fi);for(int fi:vfaces[b])if(!fdead[fi])patch.push_back(fi);sort(patch.begin(),patch.end());patch.erase(unique(patch.begin(),patch.end()),patch.end());oldTris.clear();newTris.clear();
for(int fi:patch){const Face&f=faces[fi];Vec3 op[3]={verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]};oldTris.push_back({op[0],op[1],op[2]});int id[3]={f.v[0],f.v[1],f.v[2]};for(int k=0;k<3;++k)if(id[k]==a)id[k]=b;if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;Vec3 np[3]={id[0]==b?c.position:verts[id[0]],id[1]==b?c.position:verts[id[1]],id[2]==b?c.position:verts[id[2]]};Vec3 on=cross(op[1]-op[0],op[2]-op[0]),nn=cross(np[1]-np[0],np[2]-np[0]);double ol=norm(on),nl=norm(nn);if(ol<CParam_MinNormalNorm||nl<1e-10*diag*diag)return false;if(dot(on,nn)<=0.02*ol*nl)return false;newTris.push_back({np[0],np[1],np[2]});}
if(newTris.empty())return false;auto worst=[&](const vector<array<Vec3,3>>&A,const vector<array<Vec3,3>>&B){double w=0;for(auto&t:A){Vec3 s[7]={t[0],t[1],t[2],(t[0]+t[1])*.5,(t[1]+t[2])*.5,(t[2]+t[0])*.5,(t[0]+t[1]+t[2])/3.0};for(Vec3&p:s){double d=CParam_Inf;for(auto&q:B)d=min(d,pointTriangleDistance2(p,q[0],q[1],q[2]));w=max(w,d);}}return w;};double w2=max(worst(oldTris,newTris),worst(newTris,oldTris));return isfinite(w2)&&w2<=(0.72*hausd)*(0.72*hausd);
}

bool hiddenPatchBehind(const HiddenAtlas&h,const CollapseCandidate&c,const vector<int>&patch)const{
for(int fi:patch)if(fi>=0&&fi<nF&&!fdead[fi]&&h.visibleFace[fi])return false;int a=c.absorbed,b=c.kept;double margin=max(2e-5,0.0012*diag);
for(int view=0;view<6;++view)for(int fi:patch){if(fdead[fi])continue;const Face&f=faces[fi];int id[3]={f.v[0],f.v[1],f.v[2]};for(int k=0;k<3;++k)if(id[k]==a)id[k]=b;if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;Vec3 pp[3]={id[0]==b?c.position:verts[id[0]],id[1]==b?c.position:verts[id[1]],id[2]==b?c.position:verts[id[2]]};auto p0=projectImp(pp[0],view,h.R),p1=projectImp(pp[1],view,h.R),p2=projectImp(pp[2],view,h.R);if(!p0.ok||!p1.ok||!p2.ok)return false;double den=(p1.v-p2.v)*(p0.u-p2.u)+(p2.u-p1.u)*(p0.v-p2.v);if(fabs(den)<1e-18)return false;int x0=max(0,(int)floor(min({p0.u,p1.u,p2.u}))),x1=min(h.R-1,(int)ceil(max({p0.u,p1.u,p2.u}))),y0=max(0,(int)floor(min({p0.v,p1.v,p2.v}))),y1=min(h.R-1,(int)ceil(max({p0.v,p1.v,p2.v})));for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){double X=x+.5,Y=y+.5,w0=((p1.v-p2.v)*(X-p2.u)+(p2.u-p1.u)*(Y-p2.v))/den,w1=((p2.v-p0.v)*(X-p2.u)+(p0.u-p2.u)*(Y-p2.v))/den,w2=1-w0-w1;if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;double iz=w0/p0.z+w1/p1.z+w2/p2.z;if(iz<=0)return false;double zz=1.0/iz;float front=h.z[view][y*h.R+x];if(!isfinite(front)||front>1e20f||zz<=front+margin)return false;}}
return true;
}

struct HiddenEdge{int a,b;double score;bool operator<(const HiddenEdge&o)const{return score<o.score;}};
int occludedEdgeCollapsePass(int forcedTier=0){
int active=0;for(int v=0;v<nV;++v)if(!vdead[v])++active;if(active<20)return 0;int R=nV<=50000?512:(nV<=400000?256:160);HiddenAtlas h;buildHiddenAtlas(h,R);vector<HiddenEdge>cand;cand.reserve(active*2);for(int a=0;a<nV;++a)if(!vdead[a]&&!h.protectedVert[a])for(int b:vneigh[a])if(a<b&&!vdead[b]&&!h.protectedVert[b]){bool near=false;for(int x:vneigh[a])if(!vdead[x]&&h.protectedVert[x]){near=true;break;}if(!near)for(int x:vneigh[b])if(!vdead[x]&&h.protectedVert[x]){near=true;break;}if(near)continue;double s=cheapEdgeCost(a,b)+1e-5*norm2(verts[a]-verts[b]);cand.push_back({a,b,s});}sort(cand.begin(),cand.end());int tier=forcedTier?forcedTier:(nV<=5000?1:(nV<=25000?2:(nV<=45000?3:(nV<=50000?4:(nV<=400000?5:6)))));double frac=tier<=4?0.12:(tier==5?0.035:0.018);int hard=tier==1?350:(tier==2?1600:(tier==3?2200:(tier==4?2800:(tier==5?3500:4500))));int cap=min(hard,(int)floor(active*frac)),done=0;vector<int>patch;vector<array<Vec3,3>>ot,nt;
for(auto&e:cand){if(done>=cap||elapsed()>HParam_TimeBudgetSeconds-0.18)break;if(!edgeExists(e.a,e.b)||h.protectedVert[e.a]||h.protectedVert[e.b])continue;if(countCommonFaces(e.a,e.b)!=2||countCommonNeighbors(e.a,e.b)!=2)continue;auto c=computeBestValid(e.a,e.b);if(!c.valid())continue;if(h.protectedVert[c.absorbed]||h.protectedVert[c.kept])continue;if(!hiddenLocalGeometrySafe(c,patch,ot,nt))continue;if(!hiddenPatchBehind(h,c,patch))continue;applyCollapse(c.absorbed,c.kept,c.position,c.mergedRadius);++accepted;++done;}return done;
}

Vec3 faceNormalRaw(int fi)const{

const Face& f=faces[fi];

return cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);

}

bool faceHasVertex(int fi,int v)const{

const Face& f=faces[fi];

return f.v[0]==v||f.v[1]==v||f.v[2]==v;

}

struct StarParams{

int maxValence; double maxOldDev; double maxNewDev; double distFrac;

double extraFrac; int hardCap; int scanVertices; int rounds; double timeFrac; double maxSeconds;

};

int originalTier() const{

if(nV>=1000000)return 6;

if(nV>=350000)return 5;

if(nV>=45000)return 4;

if(nV>=35000)return 3;

if(nV>=20000)return 2;

return 1;

}

StarParams starParams() const{

StarParams p1={12,0.160,0.220,1.18,0.0400,30000,820000,8,0.90,6.20};

StarParams p2={5,0.004,0.006,0.40,0.0015,900,90000,1,0.22,0.75};

StarParams p3={6,0.008,0.012,0.52,0.0030,1800,140000,2,0.32,1.20};

StarParams p4={6,0.010,0.014,0.58,0.0035,2200,150000,2,0.35,1.35};

StarParams p5={5,0.006,0.009,0.46,0.0022,1300,115000,1,0.27,0.95};

StarParams p6={6,0.008,0.012,0.52,0.0030,1800,140000,2,0.32,1.20};

int tier=originalTier();

if(tier==1)return p1;

if(tier==2)return p2;

if(tier==3)return p3;

if(tier==4)return p4;

if(tier==5)return p5;

return p6;

}

StarParams vegaSsimParams() const{

StarParams p0={0,0,0,0,0,0,0,0,0,0};

StarParams p2={8,0.024,0.040,0.90,0.0200,1250,175000,1,0.44,1.95};

StarParams p3={8,0.026,0.042,0.92,0.0180,1250,170000,1,0.45,1.95};

StarParams p4={8,0.030,0.048,1.00,0.0220,1600,190000,1,0.50,2.20};

int tier=originalTier();

if(tier==2)return p2;

if(tier==3)return p3;

if(tier==4)return p0;

return p0;

}

double vegaSsimMin() const{

int tier=originalTier();

if(tier==2)return 0.95; // Sharon v3: Goji v98 value

if(tier==3)return 0.94; // Sharon v3: Goji v98 value

if(tier==4)return 1.01;

return 1.01;

}

double vegaMaxDamage() const{

int tier=originalTier();

if(tier==2)return 0.052; // Sharon v3

if(tier==3)return 0.057; // Sharon v3

if(tier==4)return 0.0;

return 0.0;

}

bool orientedRingForVertex(int v,vector<int>& ring,vector<int>& inc)const{

StarParams sp=starParams();

return orientedRingForVertexWithParams(v,sp,ring,inc);

}

bool orientedRingForVertexWithParams(int v,const StarParams& sp,vector<int>& ring,vector<int>& inc)const{

ring.clear();inc.clear();

if(sp.maxValence<=0)return false;

if(v<0||v>=(int)vfaces.size()||vdead[v])return false;

for(int fi:vfaces[v]){ if(fdead[fi])continue; if(faceHasVertex(fi,v))inc.push_back(fi); }

int m=(int)inc.size();

if(m<3||m>sp.maxValence)return false;

vector<pair<int,int>> dir; dir.reserve(m);

for(int fi:inc){

const Face& f=faces[fi];

int pos=-1;

for(int k=0;k<3;++k)if(f.v[k]==v)pos=k;

if(pos<0)return false;

int a=f.v[(pos+1)%3],b=f.v[(pos+2)%3];

if(a==b||a==v||b==v||vdead[a]||vdead[b])return false;

dir.push_back({a,b});

}

for(int i=0;i<m;++i)for(int j=i+1;j<m;++j){

if(dir[i].first==dir[j].first)return false;

if(dir[i].second==dir[j].second)return false;

}

int start=dir[0].first,cur=start;

ring.push_back(start);

for(int step=0;step<m;++step){

int nxt=-1;

for(auto&e:dir)if(e.first==cur){nxt=e.second;break;}

if(nxt<0)return false;

if(step==m-1){ if(nxt!=start)return false; }

else{ for(int x:ring)if(x==nxt)return false; ring.push_back(nxt); cur=nxt; }

}

return (int)ring.size()==m;

}

bool activeFaceWithSameKey(int a,int b,int c,const vector<int>& skip)const{

array<int,3> key={a,b,c}; sort(key.begin(),key.end());

for(int fi:vfaces[a]){

if(fdead[fi])continue;

bool skipFace=false;

for(int s:skip)if(s==fi){skipFace=true;break;}

if(skipFace)continue;

const Face& f=faces[fi];

array<int,3> k2={f.v[0],f.v[1],f.v[2]}; sort(k2.begin(),k2.end());

if(k2==key)return true;

}

return false;

}

struct StarCandidate{

int v=-1,root=0; double score=CParam_Inf;

bool valid()const{return v>=0&&score<CParam_Inf;}

bool operator<(const StarCandidate& o)const{return score<o.score;}

};

bool evaluateStarRoot(int v,const vector<int>& ring,const vector<int>& inc,int root,double oldDev,const Vec3& avgN,StarCandidate& out)const{

StarParams sp=starParams();

return evaluateStarRootWithParams(v,ring,inc,root,oldDev,avgN,sp,out);

}

bool evaluateStarRootWithParams(int v,const vector<int>& ring,const vector<int>& inc,int root,double oldDev,const Vec3& avgN,const StarParams& sp,StarCandidate& out)const{

int m=(int)ring.size();

vector<int> rr; rr.reserve(m);

for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);

int r0=rr[0];

for(int i=2;i<=m-2;++i){ if(vneigh[r0].contains(rr[i]))return false; }

double maxNewDev=0.0; double minDist2=CParam_Inf;

for(int i=1;i<m-1;++i){

int a=rr[0],b=rr[i],c=rr[i+1];

if(a==b||b==c||a==c)return false;

if(activeFaceWithSameKey(a,b,c,inc))return false;

Vec3 n=cross(verts[b]-verts[a],verts[c]-verts[a]);

double nl=norm(n);

if(nl<CParam_MinNormalNorm)return false;

Vec3 un=n/nl;

double d=clampDouble(dot(un,avgN),-1.0,1.0);

if(d<=0.0)return false;

maxNewDev=max(maxNewDev,1.0-d);

if(maxNewDev>sp.maxNewDev)return false;

minDist2=min(minDist2,pointTriangleDistance2(verts[v],verts[a],verts[b],verts[c]));

}

double dist=sqrt(max(0.0,minDist2));

if(crad[v]+dist>hausd*sp.distFrac)return false;

out.v=v; out.root=root;

out.score=(crad[v]+dist)/(hausd+CParam_MinNormalNorm)+0.35*oldDev+0.25*maxNewDev+1e-4*m;

return true;

}

StarCandidate computeStarCandidate(int v)const{

StarParams sp=starParams();

return computeStarCandidateWithParams(v,sp);

}

StarCandidate computeStarCandidateWithParams(int v,const StarParams& sp)const{

StarCandidate best;

vector<int> ring,inc;

if(!orientedRingForVertexWithParams(v,sp,ring,inc))return best;

Vec3 avg; double areaSum=0.0,oldDev=0.0;

for(int fi:inc){

Vec3 n=faceNormalRaw(fi);

double nl=norm(n);

if(nl<CParam_MinNormalNorm)return best;

avg=avg+n; areaSum+=0.5*nl;

}

double al=norm(avg);

if(al<CParam_MinNormalNorm||areaSum<=0.0)return best;

avg=avg/al;

for(int fi:inc){

Vec3 n=faceNormalRaw(fi); double nl=norm(n); Vec3 un=n/nl;

double d=clampDouble(dot(un,avg),-1.0,1.0);

if(d<=0.0)return best;

oldDev=max(oldDev,1.0-d);

}

if(oldDev>sp.maxOldDev)return best;

for(int root=0;root<(int)ring.size();++root){

StarCandidate c;

if(evaluateStarRootWithParams(v,ring,inc,root,oldDev,avg,sp,c)&&c.score<best.score)best=c;

}

return best;

}

struct RootNudgeParams{

double frac=0.0,maxMoveDiag=0.0,radiusFrac=0.0,minDot=1.01;

int maxIncident=0;

};

RootNudgeParams rootNudgeParams()const{

RootNudgeParams z;

if(!HParam_EnableRootNudge)return z;

int tier=originalTier();

if(HParam_RootNudgeProfile==1){

if(tier==4)return {0.055,0.0012,0.96,0.992,34};

}else if(HParam_RootNudgeProfile==2){

if(tier==2)return {0.035,0.0008,0.92,0.995,28};

if(tier==4)return {0.050,0.0011,0.94,0.993,34};

}else if(HParam_RootNudgeProfile==3){

if(tier==2)return {0.045,0.0010,0.94,0.993,30};

if(tier==3)return {0.030,0.0007,0.90,0.996,30};

if(tier==4)return {0.065,0.0015,0.98,0.990,36};

}

return z;

}

bool tryRootNudgeToward(int root,const Vec3& target,const RootNudgeParams& rp){

if(rp.frac<=0.0||root<0||root>=(int)vdead.size()||vdead[root])return false;

if((int)vfaces[root].size()>rp.maxIncident)return false;

Vec3 cur=verts[root];

Vec3 delta=(target-cur)*rp.frac;

double dl=norm(delta);

if(dl<CParam_MinNormalNorm)return false;

double cap=diag*rp.maxMoveDiag;

if(cap<=0.0)return false;

if(dl>cap){ delta=delta*(cap/dl); dl=cap; }

if(crad[root]+dl>hausd*rp.radiusFrac)return false;

Vec3 np=cur+delta;

for(int fi:vfaces[root]){

if(fdead[fi])continue;

const Face& f=faces[fi];

Vec3 oldN=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);

double oldLen=norm(oldN);

if(oldLen<CParam_MinNormalNorm)return false;

Vec3 p[3]={verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]};

for(int k=0;k<3;++k)if(f.v[k]==root)p[k]=np;

Vec3 newN=cross(p[1]-p[0],p[2]-p[0]);

double newLen=norm(newN);

if(newLen<CParam_MinNormalNorm)return false;

double d=clampDouble(dot(oldN/oldLen,newN/newLen),-1.0,1.0);

if(d<rp.minDot)return false;

}

verts[root]=np;

crad[root]+=dl;

++vver[root];

return true;

}

bool applyStarDelete(int v,int root){

StarCandidate best=computeStarCandidate(v);

if(!best.valid())return false;

return applyStarDeleteWithParams(v,best.root,starParams());

}

bool applyStarDeleteWithParams(int v,int root,const StarParams& sp){

StarCandidate best=computeStarCandidateWithParams(v,sp);

if(!best.valid())return false;

root=best.root;

vector<int> ring,inc;

if(!orientedRingForVertexWithParams(v,sp,ring,inc))return false;

int m=(int)ring.size();

vector<int> rr; rr.reserve(m);

for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);

Vec3 removedPos=verts[v];

for(int fi:inc){

if(fdead[fi])continue;

Face old=faces[fi]; fdead[fi]=1;

for(int k=0;k<3;++k){ int u=old.v[k]; if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi); }

}

for(int nb:ring)if(nb>=0&&nb<(int)vneigh.size())vneigh[nb].erase(v);

vneigh[v].clear(); vfaces[v].clear(); vdead[v]=1; crad[v]=0.0; ++vver[v];

for(int i=1;i<m-1;++i){

Face nf; nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];

int fi=(int)faces.size();

faces.push_back(nf); fdead.push_back(0);

for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);

for(int k=0;k<3;++k){ int a=nf.v[k],b=nf.v[(k+1)%3]; if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);} }

}

RootNudgeParams rp=rootNudgeParams();

if(rp.frac>0.0&&!rr.empty())tryRootNudgeToward(rr[0],removedPos,rp);

return true;

}

bool weldTierEnabled()const{

return originalTier()==4;

}

StarParams weldParams()const{

if(originalTier()==4)return {6,0.015,0.024,0.66,0.0110,760,280000,1,0.52,2.30};

return {0,0,0,0,0,0,0,0,0,0};

}

bool valenceWeldRingForParams(int v,const StarParams& sp)const{

vector<int> ring,inc;

return orientedRingForVertexWithParams(v,sp,ring,inc)&&(int)ring.size()>=4&&(int)ring.size()<=sp.maxValence;

}

void valenceWeldPass(){

if(!weldTierEnabled())return;

StarParams sp=weldParams();

if(sp.maxValence<=0||sp.hardCap<=0||sp.scanVertices<=0)return;

double timeLeft=HParam_TimeBudgetSeconds-elapsed();

if(timeLeft<0.45)return;

double stopTime=elapsed()+min(sp.maxSeconds,timeLeft*sp.timeFrac);

int maxExtra=min(sp.hardCap,max(0,(int)floor(nV*sp.extraFrac)));

if(maxExtra<=0)return;

vector<StarCandidate> cands;

cands.reserve(min(sp.scanVertices,maxExtra*12+512));

int scanned=0;

for(int v=0;v<(int)verts.size()&&scanned<sp.scanVertices&&elapsed()<stopTime;++v){

if(vdead[v])continue;

++scanned;

if(!valenceWeldRingForParams(v,sp))continue;

StarCandidate c=computeStarCandidateWithParams(v,sp);

if(c.valid())cands.push_back(c);

}

if(cands.empty())return;

sort(cands.begin(),cands.end());

int extra=0;

for(const StarCandidate& c:cands){

if(extra>=maxExtra||elapsed()>=stopTime)break;

if(c.v<0||c.v>=(int)vdead.size()||vdead[c.v])continue;

if(!valenceWeldRingForParams(c.v,sp))continue;

if(applyStarDeleteWithParams(c.v,c.root,sp)){

++accepted;

++extra;

}

}

}

struct PairDiskCandidate{

int a=-1,b=-1,root=0;

double score=CParam_Inf;

vector<int> boundary;

vector<int> patchFaces;

bool valid()const{return a>=0&&b>=0&&score<CParam_Inf&&!boundary.empty()&&!patchFaces.empty();}

bool operator<(const PairDiskCandidate& o)const{return score<o.score;}

};

StarParams pairDiskParams()const{

if(originalTier()==4)return {8,0.018,0.029,0.66,0.0025,90,120000,1,0.28,0.85};

return {0,0,0,0,0,0,0,0,0,0};

}

static long long edgeKeyFor(int a,int b){

if(a>b)swap(a,b);

return ((long long)a<<32) ^ (unsigned int)b;

}

bool orderBoundaryCycle(const vector<pair<int,int>>& edges,vector<int>& ring)const{

ring.clear();

if(edges.size()<4||edges.size()>16)return false;

vector<int> vertsLocal;

vertsLocal.reserve(edges.size()*2);

for(auto&e:edges){vertsLocal.push_back(e.first);vertsLocal.push_back(e.second);}

sort(vertsLocal.begin(),vertsLocal.end());

vertsLocal.erase(unique(vertsLocal.begin(),vertsLocal.end()),vertsLocal.end());

if(vertsLocal.size()!=edges.size())return false;

vector<vector<int>> adj(vertsLocal.size());

for(auto&e:edges){

int ia=(int)(lower_bound(vertsLocal.begin(),vertsLocal.end(),e.first)-vertsLocal.begin());

int ib=(int)(lower_bound(vertsLocal.begin(),vertsLocal.end(),e.second)-vertsLocal.begin());

if(ia<0||ib<0||ia==ib||ia>=(int)vertsLocal.size()||ib>=(int)vertsLocal.size())return false;

adj[ia].push_back(ib);

adj[ib].push_back(ia);

}

for(auto&v:adj)if(v.size()!=2)return false;

int start=0,prev=-1,cur=start;

for(int step=0;step<(int)vertsLocal.size();++step){

ring.push_back(vertsLocal[cur]);

int n0=adj[cur][0],n1=adj[cur][1];

int nxt=(n0==prev)?n1:n0;

prev=cur;

cur=nxt;

if(cur==start)return step+1==(int)vertsLocal.size();

}

return false;

}

double triSetDistanceSamples(const vector<array<Vec3,3>>& src,const vector<array<Vec3,3>>& dst)const{

if(src.empty()||dst.empty())return CParam_Inf;

double worst=0.0;

for(const auto&t:src){

Vec3 samples[7]={

t[0],t[1],t[2],

(t[0]+t[1])*0.5,

(t[1]+t[2])*0.5,

(t[2]+t[0])*0.5,

(t[0]+t[1]+t[2])/3.0

};

for(const Vec3&p:samples){

double best=CParam_Inf;

for(const auto&q:dst)best=min(best,pointTriangleDistance2(p,q[0],q[1],q[2]));

worst=max(worst,best);

}

}

return worst;

}

bool buildPairDiskPatch(int a,int b,const StarParams& sp,PairDiskCandidate& out)const{

out=PairDiskCandidate();

if(!edgeExists(a,b)||sp.maxValence<=0)return false;

vector<int> patch;

patch.reserve(vfaces[a].size()+vfaces[b].size());

for(int fi:vfaces[a])if(!fdead[fi])patch.push_back(fi);

for(int fi:vfaces[b])if(!fdead[fi])patch.push_back(fi);

sort(patch.begin(),patch.end());

patch.erase(unique(patch.begin(),patch.end()),patch.end());

if(patch.size()<4||patch.size()>24)return false;

map<long long,pair<int,int>> edgeEnds;

map<long long,int> edgeCount;

for(int fi:patch){

const Face& f=faces[fi];

bool touches=false;

for(int k=0;k<3;++k)if(f.v[k]==a||f.v[k]==b)touches=true;

if(!touches)return false;

for(int k=0;k<3;++k){

int x=f.v[k],y=f.v[(k+1)%3];

long long key=edgeKeyFor(x,y);

edgeEnds[key]={min(x,y),max(x,y)};

++edgeCount[key];

}

}

vector<pair<int,int>> boundaryEdges;

for(auto&kv:edgeCount){

if(kv.second!=1)continue;

int x=edgeEnds[kv.first].first,y=edgeEnds[kv.first].second;

if(x==a||x==b||y==a||y==b)return false;

if(vdead[x]||vdead[y])return false;

boundaryEdges.push_back({x,y});

}

vector<int> ring;

if(!orderBoundaryCycle(boundaryEdges,ring))return false;

int m=(int)ring.size();

if(m<4||m>sp.maxValence)return false;

Vec3 avg;

double oldDev=0.0;

for(int fi:patch){

Vec3 n=faceNormalRaw(fi);

double nl=norm(n);

if(nl<CParam_MinNormalNorm)return false;

avg=avg+n;

}

double al=norm(avg);

if(al<CParam_MinNormalNorm)return false;

avg=avg/al;

for(int fi:patch){

Vec3 n=faceNormalRaw(fi);

double nl=norm(n);

Vec3 un=n/nl;

double d=clampDouble(dot(un,avg),-1.0,1.0);

if(d<=0.0)return false;

oldDev=max(oldDev,1.0-d);

}

if(oldDev>sp.maxOldDev)return false;

vector<array<Vec3,3>> oldTris;

oldTris.reserve(patch.size());

for(int fi:patch){

const Face& f=faces[fi];

oldTris.push_back({verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]});

}

PairDiskCandidate best;

for(int root=0;root<m;++root){

vector<int> rr;

rr.reserve(m);

for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);

int r0=rr[0];

bool ok=true;

for(int i=2;i<=m-2&&ok;++i)if(vneigh[r0].contains(rr[i]))ok=false;

if(!ok)continue;

double maxNewDev=0.0;

vector<array<Vec3,3>> newTris;

newTris.reserve(max(0,m-2));

for(int i=1;i<m-1&&ok;++i){

int x=rr[0],y=rr[i],z=rr[i+1];

if(x==y||y==z||x==z){ok=false;break;}

if(activeFaceWithSameKey(x,y,z,patch)){ok=false;break;}

Vec3 n=cross(verts[y]-verts[x],verts[z]-verts[x]);

double nl=norm(n);

if(nl<CParam_MinNormalNorm){ok=false;break;}

Vec3 un=n/nl;

double d=clampDouble(dot(un,avg),-1.0,1.0);

if(d<=0.0){ok=false;break;}

maxNewDev=max(maxNewDev,1.0-d);

if(maxNewDev>sp.maxNewDev){ok=false;break;}

newTris.push_back({verts[x],verts[y],verts[z]});

}

if(!ok||newTris.empty())continue;

double worst2=max(triSetDistanceSamples(oldTris,newTris),triSetDistanceSamples(newTris,oldTris));

if(!isfinite(worst2))continue;

double worst=sqrt(max(0.0,worst2));

double removedCover=max(crad[a],crad[b])+worst;

if(removedCover>hausd*sp.distFrac)continue;

PairDiskCandidate cand;

cand.a=a;cand.b=b;cand.root=root;cand.boundary=ring;cand.patchFaces=patch;

cand.score=removedCover/(hausd+CParam_MinNormalNorm)+0.35*oldDev+0.25*maxNewDev+1e-4*m;

if(cand.score<best.score)best=cand;

}

if(!best.valid())return false;

out=best;

return true;

}

bool applyPairDiskDelete(const PairDiskCandidate& c,const StarParams& sp){

if(c.a<0||c.b<0||vdead[c.a]||vdead[c.b]||!edgeExists(c.a,c.b))return false;

PairDiskCandidate fresh;

if(!buildPairDiskPatch(c.a,c.b,sp,fresh))return false;

if(!fresh.valid())return false;

int m=(int)fresh.boundary.size();

vector<int> rr;

rr.reserve(m);

for(int i=0;i<m;++i)rr.push_back(fresh.boundary[(fresh.root+i)%m]);

for(int fi:fresh.patchFaces){

if(fdead[fi])continue;

Face old=faces[fi];

fdead[fi]=1;

for(int k=0;k<3;++k){

int u=old.v[k];

if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi);

}

}

vector<int> na=vneigh[fresh.a].data,nb=vneigh[fresh.b].data;

for(int x:na)if(x>=0&&x<(int)vneigh.size())vneigh[x].erase(fresh.a);

for(int x:nb)if(x>=0&&x<(int)vneigh.size())vneigh[x].erase(fresh.b);

vneigh[fresh.a].clear();vneigh[fresh.b].clear();

vfaces[fresh.a].clear();vfaces[fresh.b].clear();

vdead[fresh.a]=1;vdead[fresh.b]=1;

crad[fresh.a]=0.0;crad[fresh.b]=0.0;

++vver[fresh.a];++vver[fresh.b];

for(int i=1;i<m-1;++i){

Face nf;nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];

int fi=(int)faces.size();

faces.push_back(nf);fdead.push_back(0);

for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);

for(int k=0;k<3;++k){

int x=nf.v[k],y=nf.v[(k+1)%3];

if(x!=y){vneigh[x].insert(y);vneigh[y].insert(x);}

}

}

return true;

}

void pairDiskPass(){

StarParams sp=pairDiskParams();

if(sp.maxValence<=0||sp.hardCap<=0||sp.scanVertices<=0)return;

double timeLeft=HParam_TimeBudgetSeconds-elapsed();

if(timeLeft<0.55)return;

double stopTime=elapsed()+min(sp.maxSeconds,timeLeft*sp.timeFrac);

int maxExtra=min(sp.hardCap,max(0,(int)floor(nV*sp.extraFrac)));

if(maxExtra<=0)return;

vector<PairDiskCandidate> cands;

cands.reserve(min(sp.scanVertices,maxExtra*8+256));

int scanned=0;

for(int a=0;a<(int)verts.size()&&scanned<sp.scanVertices&&elapsed()<stopTime;++a){

if(vdead[a])continue;

for(int b:vneigh[a]){

if(scanned>=sp.scanVertices||elapsed()>=stopTime)break;

if(b<=a||vdead[b])continue;

++scanned;

PairDiskCandidate c;

if(buildPairDiskPatch(a,b,sp,c))cands.push_back(c);

}

}

if(cands.empty())return;

sort(cands.begin(),cands.end());

int extra=0;

for(const PairDiskCandidate& c:cands){

if(extra>=maxExtra||elapsed()>=stopTime)break;

if(c.a<0||c.b<0||c.a>=(int)vdead.size()||c.b>=(int)vdead.size())continue;

if(vdead[c.a]||vdead[c.b])continue;

if(applyPairDiskDelete(c,sp)){

accepted+=2;

extra+=2;

}

}

}

struct VegaProj{double u=0.0,v=0.0,z=0.0;bool ok=false;};

struct VegaTri{Vec3 p[3];};

struct VegaPixel{float n[3];float d;unsigned char fg;};

struct VegaCandidate{

int v=-1,root=0; double ssim=1.0,score=CParam_Inf;

bool operator<(const VegaCandidate& o)const{return score<o.score;}

};

static VegaPixel vegaBackgroundPixel(){

VegaPixel p;

p.n[0]=127.5f;p.n[1]=127.5f;p.n[2]=127.5f;

p.d=255.0f;p.fg=0;

return p;

}

void vegaCameraBasis(int view,Vec3& eye,Vec3& right,Vec3& up,Vec3& fwd)const{

switch(view){

case 0: eye=Vec3( 2.5,0,0);break;

case 1: eye=Vec3(-2.5,0,0);break;

case 2: eye=Vec3(0, 2.5,0);break;

case 3: eye=Vec3(0,-2.5,0);break;

case 4: eye=Vec3(0,0, 2.5);break;

default:eye=Vec3(0,0,-2.5);break;

}

fwd=eye*(-1.0);

double fl=norm(fwd);

if(fl<CParam_MinNormalNorm)fwd=Vec3(0,0,-1);else fwd=fwd/fl;

Vec3 worldUp=(fabs(dot(fwd,Vec3(0,0,1)))>0.9)?Vec3(0,1,0):Vec3(0,0,1);

right=cross(worldUp,fwd);

double rl=norm(right);

if(rl<CParam_MinNormalNorm)right=Vec3(1,0,0);else right=right/rl;

up=cross(fwd,right);

double ul=norm(up);

if(ul<CParam_MinNormalNorm)up=Vec3(0,1,0);else up=up/ul;

}

VegaProj vegaProjectPoint(const Vec3& p,int view,int R)const{

Vec3 eye,right,up,fwd;

vegaCameraBasis(view,eye,right,up,fwd);

Vec3 rel=p-eye;

double x=dot(rel,right),y=dot(rel,up),z=dot(rel,fwd);

if(z<=1e-8)return {};

double scale=double(R)/1024.0;

double f=800.0*scale;

double c=0.5*double(R);

return {f*x/z+c,f*y/z+c,z,true};

}

bool buildVegaStarPatchTris(int v,int root,const StarParams& sp,vector<VegaTri>& oldTris,vector<VegaTri>& newTris)const{

oldTris.clear();newTris.clear();

vector<int> ring,inc;

if(!orientedRingForVertexWithParams(v,sp,ring,inc))return false;

StarCandidate check=computeStarCandidateWithParams(v,sp);

if(!check.valid())return false;

root=check.root;

for(int fi:inc){

const Face& f=faces[fi];

VegaTri t;

t.p[0]=verts[f.v[0]];t.p[1]=verts[f.v[1]];t.p[2]=verts[f.v[2]];

oldTris.push_back(t);

}

int m=(int)ring.size();

vector<int> rr;rr.reserve(m);

for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);

for(int i=1;i<m-1;++i){

VegaTri t;

t.p[0]=verts[rr[0]];t.p[1]=verts[rr[i]];t.p[2]=verts[rr[i+1]];

newTris.push_back(t);

}

return !oldTris.empty()&&!newTris.empty();

}

double vegaPatchGeomFrac()const{

int tier=originalTier();

if(tier==2)return 0.80;

if(tier==3)return 0.42;

if(tier==4)return 0.0;

return 1e100;

}

double vegaSampleToTrisMaxDistance2(const vector<VegaTri>& src,const vector<VegaTri>& dst)const{

if(src.empty()||dst.empty())return CParam_Inf;

double worst=0.0;

for(const VegaTri& t:src){

Vec3 samples[7]={

t.p[0],t.p[1],t.p[2],

(t.p[0]+t.p[1])*0.5,

(t.p[1]+t.p[2])*0.5,

(t.p[2]+t.p[0])*0.5,

(t.p[0]+t.p[1]+t.p[2])/3.0

};

for(const Vec3& p:samples){

double best=CParam_Inf;

for(const VegaTri& q:dst)best=min(best,pointTriangleDistance2(p,q.p[0],q.p[1],q.p[2]));

worst=max(worst,best);

}

}

return worst;

}

double vegaPatchDeviation(const vector<VegaTri>& oldTris,const vector<VegaTri>& newTris)const{

double d2=max(vegaSampleToTrisMaxDistance2(oldTris,newTris),

vegaSampleToTrisMaxDistance2(newTris,oldTris));

if(!isfinite(d2))return CParam_Inf;

return sqrt(max(0.0,d2));

}

static double vegaPixelChannel(const VegaPixel& p,int ch){

if(ch<3)return p.n[ch];

return p.d;

}

double vegaScalarSsim(const vector<VegaPixel>& a,const vector<VegaPixel>& b,int ch)const{

double sx=0,sy=0,sxx=0,syy=0,sxy=0;int cnt=0;

int n=(int)a.size();

for(int i=0;i<n;++i){

if(!a[i].fg&&!b[i].fg)continue;

double x=vegaPixelChannel(a[i],ch),y=vegaPixelChannel(b[i],ch);

sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;++cnt;

}

if(cnt<4)return 1.0;

double inv=1.0/double(cnt);

double mx=sx*inv,my=sy*inv;

double vx=max(0.0,sxx*inv-mx*mx),vy=max(0.0,syy*inv-my*my);

double cov=sxy*inv-mx*my;

double num=(2.0*mx*my+HParam_VegaC1)*(2.0*cov+HParam_VegaC2);

double den=(mx*mx+my*my+HParam_VegaC1)*(vx+vy+HParam_VegaC2);

if(den<=0.0)return 1.0;

return clampDouble(num/den,-1.0,1.0);

}

double vegaBuffersSsim(const vector<VegaPixel>& a,const vector<VegaPixel>& b)const{

int cnt=0;

for(int i=0;i<(int)a.size();++i)if(a[i].fg||b[i].fg)++cnt;

if(cnt==0)return 1.0;

double sn=(vegaScalarSsim(a,b,0)+vegaScalarSsim(a,b,1)+vegaScalarSsim(a,b,2))/3.0;

double sd=vegaScalarSsim(a,b,3);

return HParam_VegaNormalDepthWeight*sn+(1.0-HParam_VegaNormalDepthWeight)*sd;

}

bool renderVegaPatch(const vector<VegaTri>& tris,int view,int x0,int y0,int w,int h,vector<VegaPixel>& buf)const{

VegaPixel bg=vegaBackgroundPixel();

buf.assign(w*h,bg);

vector<float> zbuf(w*h,numeric_limits<float>::infinity());

const int R=HParam_VegaPatchResolution;

for(const VegaTri& tri:tris){

VegaProj p0=vegaProjectPoint(tri.p[0],view,R);

VegaProj p1=vegaProjectPoint(tri.p[1],view,R);

VegaProj p2=vegaProjectPoint(tri.p[2],view,R);

if(!p0.ok||!p1.ok||!p2.ok)continue;

double area2=(p1.u-p0.u)*(p2.v-p0.v)-(p1.v-p0.v)*(p2.u-p0.u);

if(fabs(area2)<1e-12)continue;

Vec3 nr=cross(tri.p[1]-tri.p[0],tri.p[2]-tri.p[0]);

double nl=norm(nr);

if(nl<CParam_MinNormalNorm)continue;

nr=nr/nl;

int bx0=max(x0,(int)floor(min({p0.u,p1.u,p2.u})));

int bx1=min(x0+w-1,(int)ceil(max({p0.u,p1.u,p2.u})));

int by0=max(y0,(int)floor(min({p0.v,p1.v,p2.v})));

int by1=min(y0+h-1,(int)ceil(max({p0.v,p1.v,p2.v})));

if(bx0>bx1||by0>by1)continue;

double den=(p1.v-p2.v)*(p0.u-p2.u)+(p2.u-p1.u)*(p0.v-p2.v);

if(fabs(den)<1e-18)continue;

for(int py=by0;py<=by1;++py){

for(int px=bx0;px<=bx1;++px){

double sx=px+0.5,sy=py+0.5;

double w0=((p1.v-p2.v)*(sx-p2.u)+(p2.u-p1.u)*(sy-p2.v))/den;

double w1=((p2.v-p0.v)*(sx-p2.u)+(p0.u-p2.u)*(sy-p2.v))/den;

double w2=1.0-w0-w1;

if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;

double iz=w0/p0.z+w1/p1.z+w2/p2.z;

if(iz<=0.0)continue;

double z=1.0/iz;

int idx=(py-y0)*w+(px-x0);

if(z<zbuf[idx]){

zbuf[idx]=(float)z;

buf[idx].n[0]=(float)((nr.x+1.0)*127.5);

buf[idx].n[1]=(float)((nr.y+1.0)*127.5);

buf[idx].n[2]=(float)((nr.z+1.0)*127.5);

buf[idx].d=(float)z;

buf[idx].fg=1;

}

}

}

}

return true;

}

double localVegaSsimForStarCandidate(int v,int root,const StarParams& sp)const{

vector<VegaTri> oldTris,newTris;

if(!buildVegaStarPatchTris(v,root,sp,oldTris,newTris))return -1.0;

double dev=vegaPatchDeviation(oldTris,newTris);

if(!(dev<=hausd*vegaPatchGeomFrac()))return -1.0;

const int R=HParam_VegaPatchResolution;

double total=0.0;int usedViews=0;

vector<VegaPixel> a,b;

for(int view=0;view<6;++view){

double mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;

bool any=false;

auto includeTri=[&](const VegaTri& t){

for(int k=0;k<3;++k){

VegaProj p=vegaProjectPoint(t.p[k],view,R);

if(!p.ok)continue;

any=true;

mnU=min(mnU,p.u);mxU=max(mxU,p.u);

mnV=min(mnV,p.v);mxV=max(mxV,p.v);

}

};

for(const VegaTri& t:oldTris)includeTri(t);

for(const VegaTri& t:newTris)includeTri(t);

if(!any)continue;

int pad=HParam_VegaPatchPaddingPixels;

int x0=max(0,(int)floor(mnU)-pad);

int y0=max(0,(int)floor(mnV)-pad);

int x1=min(R-1,(int)ceil(mxU)+pad);

int y1=min(R-1,(int)ceil(mxV)+pad);

if(x0>x1||y0>y1)continue;

int w=x1-x0+1,h=y1-y0+1;

if(w*h>HParam_VegaPatchMaxPixels)return -1.0;

renderVegaPatch(oldTris,view,x0,y0,w,h,a);

renderVegaPatch(newTris,view,x0,y0,w,h,b);

total+=vegaBuffersSsim(a,b);

++usedViews;

}

if(usedViews==0)return 1.0;

return total/double(usedViews);

}

void vegaSsimStarPass(){

StarParams sp=vegaSsimParams();

if(sp.maxValence<=0)return;

double timeLeft=HParam_TimeBudgetSeconds-elapsed();

if(timeLeft<0.45)return;

double stopTime=elapsed()+min(sp.maxSeconds,timeLeft*sp.timeFrac);

int maxExtra=min(sp.hardCap,max(0,(int)floor(nV*sp.extraFrac)));

if(maxExtra<=0)return;

double minS=vegaSsimMin();

double maxDamage=vegaMaxDamage();

vector<VegaCandidate> cands;

cands.reserve(min(HParam_VegaCandidatePoolCap,maxExtra*10+256));

int scanned=0,visited=0,total=(int)verts.size();

int start=(total>0)?(vegaCursor%total):0;

for(;visited<total&&scanned<sp.scanVertices&&elapsed()<stopTime;++visited){

int v=(start+visited)%total;

if(vdead[v])continue;

++scanned;

StarCandidate geom=computeStarCandidateWithParams(v,sp);

if(!geom.valid())continue;

double s=localVegaSsimForStarCandidate(geom.v,geom.root,sp);

if(s<0.0)continue;

double damage=1.0-s;

if(s<minS||damage>maxDamage)continue;

VegaCandidate vc;

vc.v=geom.v;vc.root=geom.root;vc.ssim=s;

vc.score=damage+HParam_VegaScoreGeomWeight*geom.score;

cands.push_back(vc);

if((int)cands.size()>=HParam_VegaCandidatePoolCap)break;

}

if(total>0)vegaCursor=(start+max(1,visited))%total;

if(cands.empty())return;

sort(cands.begin(),cands.end());

int extra=0;

for(const VegaCandidate& c:cands){

if(extra>=maxExtra||elapsed()>=stopTime)break;

if(c.v<0||c.v>=(int)vdead.size()||vdead[c.v])continue;

double s=localVegaSsimForStarCandidate(c.v,c.root,sp);

if(s<minS||1.0-s>maxDamage)continue;

if(applyStarDeleteWithParams(c.v,c.root,sp)){

++accepted;

++extra;

}

}

}

void collapseInvisibleEdges(){

StarParams sp=starParams();

double timeLeft=HParam_TimeBudgetSeconds-elapsed();

if(timeLeft<0.35)return;

double stopTime=elapsed()+min(sp.maxSeconds,timeLeft*sp.timeFrac);

int maxExtra=min(sp.hardCap,max(0,(int)floor(nV*sp.extraFrac)));

if(maxExtra<=0)return;

int extra=0;

for(int round=0;round<sp.rounds&&extra<maxExtra&&elapsed()<stopTime;++round){

vector<StarCandidate> cands; cands.reserve(4096);

int scanned=0,visited=0,total=(int)verts.size();

int start=(total>0)?(starCursor%total):0;

for(;visited<total&&scanned<sp.scanVertices&&elapsed()<stopTime;++visited){

int v=(start+visited)%total;

if(vdead[v])continue;

++scanned;

StarCandidate c=computeStarCandidate(v);

if(c.valid())cands.push_back(c);

}

if(total>0)starCursor=(start+max(1,visited))%total;

if(cands.empty())break;

sort(cands.begin(),cands.end());

bool progress=false;

for(const StarCandidate& c:cands){

if(extra>=maxExtra||elapsed()>=stopTime)break;

if(vdead[c.v])continue;

if(applyStarDelete(c.v,c.root)){ ++accepted; ++extra; progress=true; }

}

if(!progress)break;

}

}

void compact(){

vector<int> o2n(verts.size(),-1);

vector<Vec3> nv;nv.reserve(verts.size()-accepted);

for(int i=0;i<(int)verts.size();++i)if(!vdead[i]){o2n[i]=(int)nv.size();nv.push_back(verts[i]);}

struct FK{array<int,3>key;Face face;bool operator<(const FK&o)const{return key<o.key;}};

vector<FK> fc;fc.reserve(faces.size());

for(int fi=0;fi<(int)faces.size();++fi){

if(fdead[fi])continue;int a=faces[fi].v[0],b=faces[fi].v[1],c=faces[fi].v[2];

if(a<0||b<0||c<0||a>=(int)verts.size()||b>=(int)verts.size()||c>=(int)verts.size())continue;

if(vdead[a]||vdead[b]||vdead[c]||a==b||b==c||a==c)continue;

int na=o2n[a],nb=o2n[b],nc=o2n[c];

if(na<0||nb<0||nc<0||na==nb||nb==nc||na==nc)continue;

Face nf;nf.v[0]=na;nf.v[1]=nb;nf.v[2]=nc;

array<int,3>key={na,nb,nc};sort(key.begin(),key.end());

fc.push_back({key,nf});}

sort(fc.begin(),fc.end());

vector<Face> nf;nf.reserve(fc.size());

array<int,3>prev={-1,-1,-1};

for(auto&item:fc){if(item.key==prev)continue;prev=item.key;nf.push_back(item.face);}

verts.swap(nv);faces.swap(nf);nV=(int)verts.size();nF=(int)faces.size();

}

};

bool QemSimplifier::MEMLESS=false;

int main(){QemSimplifier s;s.run();return 0;}
