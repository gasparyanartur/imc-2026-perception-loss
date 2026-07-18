#include <bits/stdc++.h>
#define DB double
#define VC vector
#define PQ priority_queue
#define NL numeric_limits
#define UM unordered_map
#define US unordered_set
#define CX constexpr
#define V3 Vec3
#define FC Face
using namespace std;
CX DB _f2=0.055;
CX DB A2=1e-12;
CX DB _X1=1e-12;
CX DB _k=1e100;
CX DB A0=20.2;
CX int B3=10;
CX DB _N2=0.00;
CX DB _F2=0.00;
CX DB _G2=0.00;
CX DB _H2=0.00;
CX DB _B2=0.0237;
CX DB _c3=0.019912109375;
CX DB _J1=0.0330;
CX bool _Y2=true;
CX int _P=1;
CX DB _u3=3.;
CX DB _j3=3.;
CX bool A9=true;
CX int B8=512;
CX int _a2=4;
CX int _C2=52000;
CX int _g1=28000;
CX DB _a1=0.55;
CX DB _w2=0.0018;
CX DB _i2=(0.01*255.)*(0.01*255.);
CX DB _j2=(0.03*255.)*(0.03*255.);
CX bool _L1=true;
CX int B5=512;
CX int _q2=176;
CX int _g2=96;
CX int _t1=112;
CX int B2=320;
CX int _G1=48;
CX int _H1=72;
CX int _I1=76000;
CX DB _Y1=0.00024;
CX DB _Z1=0.00018;
CX DB _q=0.00080;
CX DB hparam_T3EndpointWeldScoreCap=.20;
struct V3{
DB x=0,y=0,z=0;
CX V3()=default;
CX V3(DB x_,DB y_,DB z_):x(x_),y(y_),z(z_){
}
V3 operator+(const V3&o)const{
return{
x+o.x,y+o.y,z+o.z}
;
}
V3 operator-(const V3&o)const{
return{
x-o.x,y-o.y,z-o.z}
;
}
V3 operator*(DB s)const{
return{
x*s,y*s,z*s}
;
}
V3 operator/(DB s)const{
return{
x/s,y/s,z/s}
;
}
}
;
DB dot(const V3&a,const V3&b){
return a.x*b.x+a.y*b.y+a.z*b.z;
}
V3 cross(const V3&a,const V3&b){
return{
a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}
;
}
DB norm2(const V3&v){
return dot(v,v);
}
DB norm(const V3&v){
return sqrt(norm2(v));
}
bool _N4(const V3&v){
return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);
}
struct FC{
int v[3];
}
;
struct Quadric{
DB a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
Quadric&operator+=(const Quadric&o){
a+=o.a;
b+=o.b;
c+=o.c;
d+=o.d;
e+=o.e;
f+=o.f;
g+=o.g;
h+=o.h;
i+=o.i;
j+=o.j;
return*this;
}
void scale(DB s){
a*=s;
b*=s;
c*=s;
d*=s;
e*=s;
f*=s;
g*=s;
h*=s;
i*=s;
j*=s;
}
static Quadric _T1(const V3&p0,const V3&p1,const V3&p2){
V3 n=cross(p1-p0,p2-p0);
DB ta=norm(n);
if(ta<A2)return Quadric();
n=n/ta;
DB pd=-dot(n,p0);
Quadric q;
q.a=n.x*n.x;
q.b=n.x*n.y;
q.c=n.x*n.z;
q.d=n.x*pd;
q.e=n.y*n.y;
q.f=n.y*n.z;
q.g=n.y*pd;
q.h=n.z*n.z;
q.i=n.z*pd;
q.j=pd*pd;
q.scale(sqrt(0.5*ta));
return q;
}
DB _b5(const V3&p)const{
return a*p.x*p.x+2*b*p.x*p.y+2*c*p.x*p.z+2*d*p.x+e*p.y*p.y+2*f*p.y*p.z+2*g*p.y+h*p.z*p.z+2*i*p.z+j;
}
}
;
struct _X3{
VC<int>data;
void insert(int v){
auto it=lower_bound(data.begin(),data.end(),v);
if(it==data.end()||*it!=v)data.insert(it,v);
}
void erase(int v){
auto it=lower_bound(data.begin(),data.end(),v);
if(it!=data.end()&&*it==v)data.erase(it);
}
bool _z3(int v)const{
return binary_search(data.begin(),data.end(),v);
}
int size()const{
return(int)data.size();
}
auto begin()const{
return data.begin();
}
auto end()const{
return data.end();
}
void clear(){
data.clear();
}
}
;
DB det3(DB a00,DB a01,DB a02,DB a10,DB a11,DB a12,DB a20,DB a21,DB a22){
return a00*(a11*a22-a12*a21)-a01*(a10*a22-a12*a20)+a02*(a10*a21-a11*a20);
}
bool _X4(const Quadric&q,V3&out){
DB D=det3(q.a,q.b,q.c,q.b,q.e,q.f,q.c,q.f,q.h);
if(fabs(D)<_X1)return false;
out=V3(det3(-q.d,q.b,q.c,-q.g,q.e,q.f,-q.i,q.f,q.h)/D,det3(q.a,-q.d,q.c,q.b,-q.g,q.f,q.c,-q.i,q.h)/D,det3(q.a,q.b,-q.d,q.b,q.e,-q.g,q.c,q.f,-q.i)/D);
return _N4(out);
}
struct A1{
int _o=-1,kept=-1,_i3=-1,_o4=-1;
DB cost=_k,_E=0.;
V3 _t;
bool operator<(const A1&o)const{
return cost>o.cost;
}
bool valid()const{
return _o>=0&&kept>=0&&cost<_k;
}
}
;
class _v3{
public:static bool MEMLESS;
public:void run(){
_w5();
if(nV<=10){
_v2();
return;
}
_T4=chrono::steady_clock::now();
if(IV>5000&&IV<=50000){
VC<V3>pv=ZV;
VC<FC>pf=ZF;
VC<DB>pr=_e;
_e4();
int pg=_Q2(4,4096,.94);
int gate=(int)floor(IV*(IV<=25000?.29:(IV<=45000?.145:.065)));
if(!(IV<=45000&&pg>0)){
if(pg>0&&nV<=gate){
if(IV>25000&&IV<=45000)_P5(pv);
_v2();
return;
}
ZV.swap(pv);
ZF.swap(pf);
_e.swap(pr);
nV=IV;
nF=inputF;
}
else _oc=true;
}
if(nV>5000&&nV<=50000){
VC<V3>_ov=ZV;
VC<FC>_of=ZF;
if(nV>25000&&nV<=45000){
_S=A0-4.2;
_c2();
_d2(true);
_g();
_g();
_g(0,2,1024);
compact();
}
else _E2();
if(IV<=25000)_w4();
else _R6(IV<=45000?4:5,_ov,_of);
if(IV>25000&&IV<=45000){
_a=0;
_frag=0;
_e4();
_M1();
_U1(2,true);
compact();
_P5(_ov,0,.20);
_P5(_ov,1,.20);
VC<_u1>originalReference(6);
for(int view=0;
view<6;
++view)originalReference[view]=_x(_U,_T,view,1024);
strategicDebtWeight=1e-5;
for(int strike=0;
strike<4;
++strike){
A7(originalReference,ZV,ZF,1024,true);
_P5(_ov,1,.20,true,16);
}
A7(originalReference,ZV,ZF,1024,true);
strategicDebtWeight=1e-4;
_P5(_ov,1,.20,true,8);
}
_v2();
return;
}
_e4();
_M1();
_v0();
if(IV>400000)_S=A0-0.8;
else if(IV>50000)_S=A0-1.90;
else _S=A0;
_s20();
_i1();
if(IV>400000){
if(ET()<A0-1.)_X2(IV);
else compact();
_v2();
return;
}
if(IV>50000){
DB _l4=1.70;
if(ET()<A0-_l4-0.72)_m1();
if(ET()<A0-_l4-0.55)_U1(24);
if(ET()<A0-_l4-0.30)_Z();
if(ET()<A0-0.62)_X2(IV);
else compact();
_v2();
return;
}
if(ET()<A0-0.95)_m1();
if(ET()<A0-1.10)_U1();
if(ET()<A0-1.)_Z();
if(ET()<A0-0.88)_m1();
if(ET()<A0-0.78)_U1();
if(A9&&ET()<A0-0.65)_D();
if(A9&&ET()<A0-0.30)_D();
if(A9&&ET()<A0-0.12)_D();
compact();
_v2();
}
private:int nV=0,nF=0;
DB strategicDebtWeight=1e-5;
int IV=0,inputF=0;
VC<V3>ZV;
VC<FC>ZF;
VC<V3>_U;
VC<FC>_T;
VC<DB>_e;
bool _F1=false,_oc=false;
VC<char>ZD,FD;
VC<int>vver;
VC<Quadric>vquad,vmoment;
VC<DB>crad;
VC<VC<int>>ZI;
VC<_X3>ZN;
VC<int>facePix,faceSil,faceWin;
VC<DB>_b,_fd;
VC<VC<VC<int>>>_d3;
int _b4=0;
static CX int _V=32;
int _a=0,_frag=0;
DB B6=0,_M4=0;
DB _S=A0;
PQ<A1>pq;
int targetV=0,B1=0,_j=0;
int _o1=0,_O1=0;
DB diag=0,hausd=0,costCap=_k;
DB _D4=0.;
chrono::steady_clock::time_point _T4;
void _w5(){
VC<char>buf;
buf.reserve(1<<27);
char chunk[1<<16];
size_t n;
while((n=fread(chunk,1,sizeof(chunk),stdin))>0)buf.insert(buf.end(),chunk,chunk+n);
buf.push_back('\0');
char*p=buf.data();
nV=(int)strtol(p,&p,10);
nF=(int)strtol(p,&p,10);
IV=nV;
inputF=nF;
ZV.resize(nV);
ZF.resize(nF);
for(int i=0;
i<nV;
++i){
while(*p&&*p<=' ')++p;
++p;
ZV[i].x=strtod(p,&p);
ZV[i].y=strtod(p,&p);
ZV[i].z=strtod(p,&p);
}
for(int i=0;
i<nF;
++i){
while(*p&&*p<=' ')++p;
++p;
ZF[i].v[0]=(int)strtol(p,&p,10)-1;
ZF[i].v[1]=(int)strtol(p,&p,10)-1;
ZF[i].v[2]=(int)strtol(p,&p,10)-1;
}
if(nV>5000&&nV<=50000){
_U=ZV;
_T=ZF;
}
}
void _v2(){
string out;
out.reserve(nV*42+nF*26+64);
char line[128];
snprintf(line,sizeof(line),"%d %d\n",nV,nF);
out+=line;
for(int i=0;
i<nV;
++i){
snprintf(line,sizeof(line),"v %.*g %.*g %.*g\n",B3,ZV[i].x,B3,ZV[i].y,B3,ZV[i].z);
out+=line;
}
for(int i=0;
i<nF;
++i){
snprintf(line,sizeof(line),"f %d %d %d\n",ZF[i].v[0]+1,ZF[i].v[1]+1,ZF[i].v[2]+1);
out+=line;
}
fwrite(out.data(),1,out.size(),stdout);
}
void _q4(int view,V3&eye,V3&right,V3&up,V3&fwd)const{
V3 dirs[6]={
{
1,0,0}
,{
-1,0,0}
,{
0,1,0}
,{
0,-1,0}
,{
0,0,1}
,{
0,0,-1}
}
;
eye=dirs[view]*2.5;
fwd=eye*(-1.);
DB fl=norm(fwd);
fwd=fwd/fl;
V3 wu=_oc?((fabs(dot(fwd,V3(0,1,0)))>.9)?V3(0,0,1):V3(0,1,0)):((fabs(dot(fwd,V3(0,0,1)))>.9)?V3(0,1,0):V3(0,0,1));
right=cross(wu,fwd);
DB rl=norm(right);
right=rl>1e-15?right/rl:V3(1,0,0);
up=cross(fwd,right);
DB ul=norm(up);
up=ul>1e-15?up/ul:V3(0,1,0);
}
struct ImpProj{
DB u=0,v=0,z=0;
bool ok=false;
}
;
ImpProj _n(const V3&p,int view,int R)const{
V3 e,r,u,f;
_q4(view,e,r,u,f);
V3 q=p-e;
DB x=dot(q,r),y=dot(q,u),z=dot(q,f);
if(z<=1e-8)return{
}
;
DB sc=DB(R)/1024.;
return{
800.*sc*x/z+0.5*R,800.*sc*y/z+0.5*R,z,true}
;
}
void _h1(int R){
facePix.assign(nF,0);
faceSil.assign(nF,0);
faceWin.assign(nF,0);
VC<V3>fn(nF);
for(int fi=0;
fi<nF;
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
auto&f=ZF[fi];
V3 n=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB l=norm(n);
if(l>1e-15)fn[fi]=n/l;
}
int rad=max(1,(int)ceil(5.*R/1024.));
for(int view=0;
view<6;
++view){
VC<float>z((size_t)R*R,1e30f);
VC<int>id((size_t)R*R,-1);
for(int fi=0;
fi<nF;
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
auto&f=ZF[fi];
auto a=_n(ZV[f.v[0]],view,R),b=_n(ZV[f.v[1]],view,R),c=_n(ZV[f.v[2]],view,R);
if(!a.ok||!b.ok||!c.ok)continue;
DB den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);
if(fabs(den)<1e-18)continue;
int x0=max(0,(int)floor(min({
a.u,b.u,c.u}
))),x1=min(R-1,(int)ceil(max({
a.u,b.u,c.u}
))),y0=max(0,(int)floor(min({
a.v,b.v,c.v}
))),y1=min(R-1,(int)ceil(max({
a.v,b.v,c.v}
)));
for(int y=y0;
y<=y1;
++y)for(int x=x0;
x<=x1;
++x){
DB X=x+.5,Y=y+.5,w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den,w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den,w2=1-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/a.z+w1/b.z+w2/c.z;
if(iz<=0)continue;
float zz=1./iz;
int q=y*R+x;
if(zz<z[q]){
z[q]=zz;
id[q]=fi;
}
}
}
for(int q=0;
q<R*R;
++q)if(id[q]>=0)++facePix[id[q]];
VC<unsigned short>d((size_t)R*R,60000);
for(int y=0;
y<R;
++y)for(int x=0;
x<R;
++x){
int q=y*R+x,f=id[q];
if(f<0)continue;
bool ft=false;
for(int dy=-1;
dy<=1&&!ft;
++dy)for(int dx=-1;
dx<=1;
++dx){
if(!dx&&!dy)continue;
int xx=x+dx,yy=y+dy;
if(xx<0||yy<0||xx>=R||yy>=R){
ft=true;
break;
}
int g=id[yy*R+xx];
if(g<0||(g!=f&&(fabs(z[q]-z[yy*R+xx])>0.0035||dot(fn[f],fn[g])<0.992))){
ft=true;
break;
}
}
if(ft){
d[q]=0;
++faceSil[f];
}
}
for(int y=0;
y<R;
++y)for(int x=0;
x<R;
++x){
int q=y*R+x,v=d[q];
if(x)v=min<int>(v,d[q-1]+1);
if(y)v=min<int>(v,d[q-R]+1);
if(x&&y)v=min<int>(v,d[q-R-1]+1);
if(x+1<R&&y)v=min<int>(v,d[q-R+1]+1);
d[q]=v;
}
for(int y=R-1;
y>=0;
--y)for(int x=R-1;
x>=0;
--x){
int q=y*R+x,v=d[q];
if(x+1<R)v=min<int>(v,d[q+1]+1);
if(y+1<R)v=min<int>(v,d[q+R]+1);
if(x+1<R&&y+1<R)v=min<int>(v,d[q+R+1]+1);
if(x&&y+1<R)v=min<int>(v,d[q+R-1]+1);
d[q]=v;
if(v<=rad&&id[q]>=0)++faceWin[id[q]];
}
}
}
void B9(int R){
facePix.assign(nF,0);
faceSil.assign(nF,0);
faceWin.assign(nF,0);
VC<V3>fn(nF);
for(int fi=0;
fi<nF;
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
auto&f=ZF[fi];
V3 n=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB l=norm(n);
if(l>1e-15)fn[fi]=n/l;
}
for(int view=0;
view<6;
++view){
VC<float>z((size_t)R*R,1e30f);
VC<int>id((size_t)R*R,-1);
for(int fi=0;
fi<nF;
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
auto&f=ZF[fi];
auto a=_n(ZV[f.v[0]],view,R),b=_n(ZV[f.v[1]],view,R),c=_n(ZV[f.v[2]],view,R);
if(!a.ok||!b.ok||!c.ok)continue;
DB den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);
if(fabs(den)<1e-18)continue;
int x0=max(0,(int)floor(min({
a.u,b.u,c.u}
))),x1=min(R-1,(int)ceil(max({
a.u,b.u,c.u}
))),y0=max(0,(int)floor(min({
a.v,b.v,c.v}
))),y1=min(R-1,(int)ceil(max({
a.v,b.v,c.v}
)));
for(int y=y0;
y<=y1;
++y)for(int x=x0;
x<=x1;
++x){
DB X=x+.5,Y=y+.5,w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den,w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den,w2=1-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/a.z+w1/b.z+w2/c.z;
if(iz<=0)continue;
float zz=1./iz;
int q=y*R+x;
if(zz<z[q]){
z[q]=zz;
id[q]=fi;
}
}
}
for(int q=0;
q<R*R;
++q)if(id[q]>=0)++facePix[id[q]];
for(int y=1;
y<R-1;
++y)for(int x=1;
x<R-1;
++x){
int q=y*R+x,f=id[q];
if(f<0)continue;
bool sil=false;
for(int d:{
-1,1,-R,R}
){
int g=id[q+d];
if(g<0||(g!=f&&(fabs(z[q]-z[q+d])>0.006||dot(fn[f],fn[g])<0.985))){
sil=true;
break;
}
}
if(sil)++faceSil[f];
}
}
}
void _c2(){
_a=nV<=25000?2:(nV<=45000?3:4);
_e4();
_M1();
vmoment.assign(nV,Quadric());
if(_a==4)for(int i=0;
i<nV;
++i)vmoment[i]=_x2(ZV[i],1e-6);
if(_a==3){
int _c4=(int)floor(nV*0.145),_l5=(int)floor(nV*0.16);
B9(384);
_v();
targetV=_l5;
B1=nV-targetV;
_s2();
_i1();
if(_j<nV-_c4&&ET()<A0-0.4){
B9(384);
_v();
PQ<A1>e;
pq.swap(e);
targetV=_c4;
B1=nV-targetV;
_s2();
_i1();
}
return;
}
VC<DB>st=_a==2?VC<DB>{
0.36,0.33,0.30}
:VC<DB>{
0.14,0.10,0.08}
;
for(DB kr:st){
if(IV>25000&&ET()>A0-0.5)break;
_h1(_a==2?1024:768);
_v();
PQ<A1>e;
pq.swap(e);
targetV=(int)floor(nV*kr);
B1=nV-targetV;
_s2();
_i1();
}
}
struct _u1{
VC<float>nx,ny,nz,d,z;
VC<unsigned char>fg;
_u1(){
}
_u1(int R):nx((size_t)R*R,127.5f),ny((size_t)R*R,127.5f),nz((size_t)R*R,127.5f),d((size_t)R*R,255.f),z((size_t)R*R,1e30f),fg((size_t)R*R,0){
}
}
;
_u1 _x(const VC<V3>&vv,const VC<FC>&ff,int view,int R,VC<int>*oi=nullptr)const{
_u1 im(R);
if(oi)oi->assign((size_t)R*R,-1);
int fi=-1;
for(const FC&fc:ff){
++fi;
if(fc.v[0]<0||fc.v[1]<0||fc.v[2]<0||fc.v[0]>=(int)vv.size()||fc.v[1]>=(int)vv.size()||fc.v[2]>=(int)vv.size())continue;
auto p0=_n(vv[fc.v[0]],view,R),p1=_n(vv[fc.v[1]],view,R),p2=_n(vv[fc.v[2]],view,R);
if(!p0 .ok||!p1 .ok||!p2 .ok)continue;
DB den=(p1 .v-p2 .v)*(p0 .u-p2 .u)+(p2 .u-p1 .u)*(p0 .v-p2 .v);
if(fabs(den)<1e-18)continue;
V3 nr=cross(vv[fc.v[1]]-vv[fc.v[0]],vv[fc.v[2]]-vv[fc.v[0]]);
DB nl=norm(nr);
if(nl<1e-15)continue;
nr=nr/nl;
int x0=max(0,(int)floor(min({
p0 .u,p1 .u,p2 .u}
))),x1=min(R-1,(int)ceil(max({
p0 .u,p1 .u,p2 .u}
))),y0=max(0,(int)floor(min({
p0 .v,p1 .v,p2 .v}
))),y1=min(R-1,(int)ceil(max({
p0 .v,p1 .v,p2 .v}
)));
for(int y=y0;
y<=y1;
++y)for(int x=x0;
x<=x1;
++x){
DB X=x+.5,Y=y+.5,w0=((p1 .v-p2 .v)*(X-p2 .u)+(p2 .u-p1 .u)*(Y-p2 .v))/den,w1=((p2 .v-p0 .v)*(X-p2 .u)+(p0 .u-p2 .u)*(Y-p2 .v))/den,w2=1-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/p0 .z+w1/p1 .z+w2/p2 .z;
if(iz<=0)continue;
float zz=(float)(1./iz);
int q=y*R+x;
if(zz>=im.z[q])continue;
im.z[q]=zz;
im.nx[q]=(float)((nr.x+1.)*127.5);
im.ny[q]=(float)((nr.y+1.)*127.5);
im.nz[q]=(float)((nr.z+1.)*127.5);
im.d[q]=zz;
im.fg[q]=1;
if(oi)(*oi)[q]=fi;
}
}
return im;
}
DB refSsim(const VC<float>&a,const VC<float>&b,const VC<unsigned char>&fa,const VC<unsigned char>&fb,int R)const{
VC<DB>x(R),y(R),xx(R),yy(R),xy(R);
auto row=[&](int r,DB s){
int q=r*R;
for(int i=0;
i<R;
++i){
DB A=a[q+i],B=b[q+i];
x[i]+=s*A;
y[i]+=s*B;
xx[i]+=s*A*A;
yy[i]+=s*B*B;
xy[i]+=s*A*B;
}
}
;
for(int r=0;
r<11;
++r)row(r,1);
DB T=0,iv=1./121.,C1=6.5025,C2=58.5225;
long long n=0;
for(int v=5;
v<R-5;
++v){
DB X=0,Y=0,XX=0,YY=0,XY=0;
for(int i=0;
i<11;
++i){
X+=x[i];
Y+=y[i];
XX+=xx[i];
YY+=yy[i];
XY+=xy[i];
}
for(int u=5;
u<R-5;
++u){
int q=v*R+u;
if(fa[q]||fb[q]){
DB A=X*iv,B=Y*iv,V=max(0.,XX*iv-A*A),W=max(0.,YY*iv-B*B),K=XY*iv-A*B,D=(A*A+B*B+C1)*(V+W+C2);
T+=D?max(-1.,min(1.,(2*A*B+C1)*(2*K+C2)/D)):1.;
++n;
}
int l=u-5,r=u+6;
if(r<R){
X+=x[r]-x[l];
Y+=y[r]-y[l];
XX+=xx[r]-xx[l];
YY+=yy[r]-yy[l];
XY+=xy[r]-xy[l];
}
}
int a0=v-5,a1=v+6;
if(a1<R){
row(a0,-1);
row(a1,1);
}
}
return n?T/n:1.;
}
struct _l{
DB _f=0,minView=1,_w=1,_R=1;
}
;
_l A7(const VC<_u1>&ref,const VC<V3>&vv,const VC<FC>&ff,int R,bool debt=false){
_l o;
if(debt)_fd.assign(vv.size(),0);
for(int v=0;
v<6;
++v){
VC<int>id;
_u1 b=_x(vv,ff,v,R,debt?&id:nullptr);
DB sn=(refSsim(ref[v].nx,b.nx,ref[v].fg,b.fg,R)+refSsim(ref[v].ny,b.ny,ref[v].fg,b.fg,R)+refSsim(ref[v].nz,b.nz,ref[v].fg,b.fg,R))/3.,sd=refSsim(ref[v].d,b.d,ref[v].fg,b.fg,R),cv=.5*(sn+sd);
o._f+=cv;
o.minView=min(o.minView,cv);
o._w=min(o._w,sn);
o._R=min(o._R,sd);
if(debt){
DB vw=1+10*(1-cv);
for(int q=0;
q<R*R;
q+=4){
int f=id[q];
if(f<0)continue;
DB e;
if(ref[v].fg[q]!=b.fg[q])e=9;
else{
e=(fabs(ref[v].nx[q]-b.nx[q])+fabs(ref[v].ny[q]-b.ny[q])+fabs(ref[v].nz[q]-b.nz[q]))/255.;
e+=min(2.,fabs(ref[v].d[q]-b.d[q])/(.003*diag+1e-12));
}
if(e<.08)continue;
for(int k=0;
k<3;
++k)_fd[ff[f].v[k]]+=e*vw;
}
}
}
o._f/=6;
if(debt){
DB m=0;
for(DB x:_fd)m+=x;
m=m/max(1,(int)_fd.size())+1e-12;
for(DB&x:_fd)x=max(0.,x/m-.65);
}
return o;
}
void A3(VC<V3>&vv,VC<FC>&ff)const{
VC<int>m(ZV.size(),-1);
vv.clear();
ff.clear();
vv.reserve(ZV.size());
for(int i=0;
i<(int)ZV.size();
++i)if(i>=(int)ZD.size()||!ZD[i]){
m[i]=(int)vv.size();
vv.push_back(ZV[i]);
}
for(int fi=0;
fi<(int)ZF.size();
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
const FC&f=ZF[fi];
if(f.v[0]<0||f.v[1]<0||f.v[2]<0||f.v[0]>=(int)m.size()||f.v[1]>=(int)m.size()||f.v[2]>=(int)m.size())continue;
int a=m[f.v[0]],b=m[f.v[1]],c=m[f.v[2]];
if(a<0||b<0||c<0||a==b||b==c||a==c)continue;
FC q;
q.v[0]=a;
q.v[1]=b;
q.v[2]=c;
ff.push_back(q);
}
}
void _A2(VC<DB>&rr)const{
rr.clear();
rr.reserve(ZV.size());
for(int i=0;
i<(int)ZV.size();
++i)if(i>=(int)ZD.size()||!ZD[i])rr.push_back(i<(int)crad.size()?crad[i]:0.);
}
struct MidSnap{
VC<V3>ZV;
VC<FC>ZF;
VC<char>ZD,FD;
VC<int>vver;
VC<Quadric>vquad,vmoment;
VC<DB>crad;
VC<VC<int>>ZI;
VC<_X3>ZN;
VC<int>facePix,faceSil,faceWin;
VC<DB>_b;
int targetV,B1,_j,_o1,_O1,_a;
DB B6;
}
;
MidSnap _K()const{
return{
ZV,ZF,ZD,FD,vver,vquad,vmoment,crad,ZI,ZN,facePix,faceSil,faceWin,_b,targetV,B1,_j,_o1,_O1,_a,B6}
;
}
void _i(const MidSnap&s){
ZV=s.ZV;
ZF=s.ZF;
ZD=s.ZD;
FD=s.FD;
vver=s.vver;
vquad=s.vquad;
vmoment=s.vmoment;
crad=s.crad;
ZI=s.ZI;
ZN=s.ZN;
facePix=s.facePix;
faceSil=s.faceSil;
faceWin=s.faceWin;
_b=s._b;
targetV=s.targetV;
B1=s.B1;
_j=s._j;
_o1=s._o1;
_O1=s._O1;
_a=s._a;
B6=s.B6;
PQ<A1>e;
pq.swap(e);
}
void _U2(int tier,int _r,DB ratio,int _N1){
if(tier==3)B9(512);
else _h1(tier==2?1024:768);
_v();
PQ<A1>e;
pq.swap(e);
targetV=max(10,(int)floor(_r*ratio)-_N1);
B1=_r-targetV;
_s2();
_i1();
}
bool txGuard(const _l&s,int tier)const{
if(tier==2)return s._f>=.9965&&s.minView>=.9955&&s._w>=.9930&&s._R>=.9988;
if(tier==3)return s._f>=.9950&&s.minView>=.9942&&s._w>=.9892&&s._R>=.9985;
return s._f>=.9950&&s.minView>=.9942&&s._w>=.9892&&s._R>=.9985;
}
void _E2(){
int _r=nV,tier=nV<=25000?2:(nV<=45000?3:4);
if(tier==2){
_c2();
_d2(true);
if(IV>25000||nV<IV*.90){
_g();
_g();
}
else _g(1);
MidSnap safe=_K();
VC<V3>safeV,candV;
VC<FC>safeF,candF;
A3(safeV,safeF);
int R=1024;
if(IV>25000&&ET()>A0-4.){
compact();
return;
}
VC<_u1>ref(6);
for(int v=0;
v<6;
++v)ref[v]=_x(safeV,safeF,v,R);
VC<DB>tries={
.27,.28,.285,.29,.295}
;
bool kept=false;
for(DB ratio:tries){
if(IV>25000&&ET()>A0-1.8)break;
_i(safe);
_U2(tier,_r,ratio,0);
A3(candV,candF);
_l sc=A7(ref,candV,candF,R);
if(txGuard(sc,tier)){
_A2(_e);
ZV=candV;
ZF=candF;
nV=(int)ZV.size();
nF=(int)ZF.size();
kept=true;
break;
}
}
if(!kept){
_i(safe);
compact();
}
return;
}
_c2();
_d2(true);
_g();
_g();
MidSnap _s5=_K();
VC<V3>baseV,safeV,candV;
VC<FC>baseF,safeF,candF;
A3(baseV,baseF);
int _N1=0,R=1024;
if(ET()<A0-4.8){
_N1=_O();
if(_N1>0){
VC<_u1>br(6);
for(int v=0;
v<6;
++v)br[v]=_x(baseV,baseF,v,R);
A3(safeV,safeF);
_l hs=A7(br,safeV,safeF,R);
if(!(hs._f>=.9995&&hs.minView>=.9990&&hs._w>=.9985&&hs._R>=.9995)){
_i(_s5);
_N1=0;
}
}
}
MidSnap safe=_K();
A3(safeV,safeF);
if(IV>25000&&ET()>A0-4.){
compact();
return;
}
VC<_u1>ref(6);
for(int v=0;
v<6;
++v)ref[v]=_x(safeV,safeF,v,R);
VC<DB>tries=tier==3?VC<DB>{
.125,.13,.135,.1375,.14,.1425}
:VC<DB>{
.065,.07,.075,.0775,.079}
;
bool kept=false;
for(DB ratio:tries){
if(IV>25000&&ET()>A0-1.8)break;
_i(safe);
_U2(tier,_r,ratio,_N1);
A3(candV,candF);
_l sc=A7(ref,candV,candF,R);
if(txGuard(sc,tier)){
_A2(_e);
ZV=candV;
ZF=candF;
nV=(int)ZV.size();
nF=(int)ZF.size();
kept=true;
break;
}
}
if(!kept){
_i(safe);
compact();
}
}
void _K1(){
VC<int>mapv(ZV.size(),-1);
VC<V3>nv;
VC<DB>nr;
nv.reserve(ZV.size());
nr.reserve(ZV.size());
for(int i=0;
i<(int)ZV.size();
++i)if(i>=(int)ZD.size()||!ZD[i]){
mapv[i]=(int)nv.size();
nv.push_back(ZV[i]);
nr.push_back(i<(int)crad.size()?crad[i]:0.);
}
VC<FC>nf;
nf.reserve(ZF.size());
for(int fi=0;
fi<(int)ZF.size();
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
const FC&f=ZF[fi];
if(f.v[0]<0||f.v[1]<0||f.v[2]<0||f.v[0]>=(int)mapv.size()||f.v[1]>=(int)mapv.size()||f.v[2]>=(int)mapv.size())continue;
int a=mapv[f.v[0]],b=mapv[f.v[1]],c=mapv[f.v[2]];
if(a<0||b<0||c<0||a==b||b==c||a==c)continue;
FC q;
q.v[0]=a;
q.v[1]=b;
q.v[2]=c;
nf.push_back(q);
}
ZV.swap(nv);
ZF.swap(nf);
crad.swap(nr);
nV=(int)ZV.size();
nF=(int)ZF.size();
ZD.assign(nV,0);
FD.assign(nF,0);
vver.assign(nV,0);
vquad.assign(nV,Quadric());
vmoment.assign(nV,Quadric());
ZI.assign(nV,{
}
);
ZN.assign(nV,_X3());
facePix.clear();
faceSil.clear();
faceWin.clear();
_b.clear();
_a=0;
B6=0;
_M4=0;
for(int fi=0;
fi<nF;
++fi){
FC&f=ZF[fi];
Quadric q=Quadric::_T1(ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]);
for(int k=0;
k<3;
++k){
ZI[f.v[k]].push_back(fi);
vquad[f.v[k]]+=q;
}
for(int k=0;
k<3;
++k){
int a=f.v[k],b=f.v[(k+1)%3];
if(a!=b){
ZN[a].insert(b);
ZN[b].insert(a);
}
}
}
PQ<A1>empty;
pq.swap(empty);
_j=0;
targetV=nV;
B1=0;
_o1=_O1=0;
}
void _z2(int _r,DB ratio){
_v();
PQ<A1>empty;
pq.swap(empty);
_j=0;
targetV=max(10,(int)floor(_r*ratio));
targetV=min(targetV,nV);
B1=max(0,nV-targetV);
_s2();
_i1();
}
bool _M(const _l&s,int tier)const{
if(tier==5)return s._f>=.9945&&s.minView>=.9925&&s._w>=.9880&&s._R>=.9970;
return s._f>=.9975&&s.minView>=.9960&&s._w>=.9940&&s._R>=.9990;
}
int _B3(int _r,int tier,DB scale=1.){
_c sp=tier==5?_c{
5,0.006,0.009,0.46,0.0022,1300,115000,1,0.27,0.95}
:_c{
6,0.008,0.012,0.52,0.0030,1800,140000,2,0.32,1.20}
;
sp._f4*=scale;
sp._N*=scale;
sp._z4*=min(1.12,scale);
DB _c1=A0-ET();
if(_c1<0.25)return 0;
DB _G=ET()+min(sp._K2,_c1*sp._F3);
int _y=min(sp.hardCap,max(0,(int)floor(_r*sp._g3)));
int extra=0;
for(int round=0;
round<sp.rounds&&extra<_y&&ET()<_G;
++round){
VC<A4>cands;
cands.reserve(8192);
int scanned=0,visited=0,total=(int)ZV.size(),start=total?_o1%total:0;
for(;
visited<total&&scanned<sp._u&&ET()<_G;
++visited){
int v=(start+visited)%total;
if(ZD[v])continue;
++scanned;
A4 c=A5(v,sp);
if(c.valid())cands.push_back(c);
}
if(total)_o1=(start+max(1,visited))%total;
if(cands.empty())break;
sort(cands.begin(),cands.end());
bool _E3=false;
for(const A4&c:cands){
if(extra>=_y||ET()>=_G)break;
if(ZD[c.v])continue;
if(_p(c.v,c.root,sp)){
++_j;
++extra;
_E3=true;
}
}
if(!_E3)break;
}
return extra;
}
void _X2(int _r){
int tier=_r<=400000?5:6;
if(tier==6){
_K1();
if(nV<20||ET()>A0-1.){
compact();
return;
}
VC<V3>safeV,candV;
VC<FC>safeF,candF;
A3(safeV,safeF);
int R=192;
VC<_u1>ref(6);
for(int v=0;
v<6;
++v)ref[v]=_x(safeV,safeF,v,R);
MidSnap base=_K();
if(ET()<A0-0.80){
int hidden=_O(6);
if(hidden>0){
A3(candV,candF);
_l hs=A7(ref,candV,candF,R);
if(!_M(hs,6))_i(base);
}
}
MidSnap pp6=_K();
if(ET()<A0-.72){
_U1(2);
A3(candV,candF);
_l ps=A7(ref,candV,candF,R);
if(!_M(ps,6))_i(pp6);
}
MidSnap _J2=_K();
VC<DB>tries={
.026,.028,.030,.031}
;
bool qemKept=false;
for(DB ratio:tries){
if(ET()>A0-0.60)break;
_i(_J2);
_z2(_r,ratio);
A3(candV,candF);
_l sc=A7(ref,candV,candF,R);
if(_M(sc,6)){
qemKept=true;
break;
}
}
if(!qemKept)_i(_J2);
if(ET()<A0-.39){
MidSnap pp7=_K();
_U1(2);
A3(candV,candF);
_l ps=A7(ref,candV,candF,R);
if(!_M(ps,6))_i(pp7);
}
MidSnap preStar=_K();
if(ET()<A0-0.32){
int sd=_B3(_r,6,1.);
if(sd>0){
A3(candV,candF);
_l ss=A7(ref,candV,candF,R);
if(!_M(ss,6))_i(preStar);
}
}
compact();
return;
}
_K1();
if(nV<20||ET()>A0-0.48){
compact();
return;
}
VC<V3>safeV,candV;
VC<FC>safeF,candF;
A3(safeV,safeF);
int R=256;
VC<_u1>ref(6);
for(int v=0;
v<6;
++v)ref[v]=_x(safeV,safeF,v,R);
MidSnap base=_K();
if(ET()<A0-0.92){
int hidden=_O(5);
if(hidden>0){
A3(candV,candF);
_l hs=A7(ref,candV,candF,R);
if(!_M(hs,5))_i(base);
}
}
MidSnap pp=_K();
if(ET()<A0-.70){
_U1();
A3(candV,candF);
_l ps=A7(ref,candV,candF,R);
if(!_M(ps,5))_i(pp);
}
MidSnap _J2=_K();
int _J4=0;
for(int i=0;
i<(int)ZD.size();
++i)if(!ZD[i])++_J4;
DB _j4=DB(_J4)/DB(_r);
DB _L3=0.0155,cut=0.18;
VC<DB>tries={
max(_L3,_j4*(1.-cut)),max(_L3,_j4*(1.-0.55*cut)),max(_L3,_j4*0.97)}
;
sort(tries.begin(),tries.end());
tries.erase(unique(tries.begin(),tries.end(),[](DB a,DB b){
return fabs(a-b)<1e-5;
}
),tries.end());
bool qemKept=false;
for(DB ratio:tries){
if(ET()>A0-0.43)break;
_i(_J2);
_S=A0-0.25;
_z2(_r,ratio);
A3(candV,candF);
_l sc=A7(ref,candV,candF,R);
if(_M(sc,5)){
qemKept=true;
break;
}
}
if(!qemKept)_i(_J2);
MidSnap preStar=_K();
if(ET()<A0-0.24){
int sd=_B3(_r,5,1.);
if(sd>0){
A3(candV,candF);
_l ss=A7(ref,candV,candF,R);
if(!_M(ss,5))_i(preStar);
}
}
compact();
}
void _e4(){
MEMLESS=(nV>5000);
V3 mn=ZV[0],mx=ZV[0];
for(auto&p:ZV){
mn.x=min(mn.x,p.x);
mn.y=min(mn.y,p.y);
mn.z=min(mn.z,p.z);
mx.x=max(mx.x,p.x);
mx.y=max(mx.y,p.y);
mx.z=max(mx.z,p.z);
}
diag=norm(mx-mn);
hausd=_f2*diag;
costCap=_J1*diag*diag;
_D4=(diag>A2)?(1./(diag*diag)):0.;
DB kr;
if(nV<=5000)kr=_N2;
else if(nV<=25000)kr=_F2;
else if(nV<=45000)kr=_G2;
else if(nV<=50000)kr=_H2;
else if(nV<=400000)kr=_B2;
else kr=_c3;
targetV=max(10,(int)floor(nV*kr));
targetV=min(targetV,nV-1);
B1=nV-targetV;
}
void _M1(){
ZD.assign(nV,0);
FD.assign(nF,0);
vver.assign(nV,0);
vquad.assign(nV,Quadric());
vmoment.assign(nV,Quadric());
crad.assign(nV,0.);
ZI.assign(nV,{
}
);
ZN.resize(nV);
for(int fi=0;
fi<nF;
++fi){
auto&f=ZF[fi];
Quadric q=Quadric::_T1(ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]);
for(int k=0;
k<3;
++k){
ZI[f.v[k]].push_back(fi);
vquad[f.v[k]]+=q;
}
for(int k=0;
k<3;
++k){
int a=f.v[k],b=f.v[(k+1)%3];
if(a!=b){
ZN[a].insert(b);
ZN[b].insert(a);
}
}
}
}
DB _A3(const V3&_R3,DB area)const{
DB absSum=fabs(_R3 .x)+fabs(_R3 .y)+fabs(_R3 .z);
DB _r4=area*_D4;
DB w=1.+_u3*_r4*absSum;
return min(w,_j3);
}
Quadric _x2(const V3&p,DB w)const{
Quadric q;
q.a=q.e=q.h=w;
q.d=-w*p.x;
q.g=-w*p.y;
q.i=-w*p.z;
q.j=w*norm2(p);
return q;
}
__attribute__((noinline,section(".text.largeqem")))void _v0(){
vquad.assign(nV,Quadric());
for(int fi=0;
fi<nF;
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
const FC&f=ZF[fi];
V3 n=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB area=0.5*norm(n);
Quadric q=Quadric::_T1(ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]);
if(area>=1e-30){
n=n/(2.*area);
DB w=_A3(n,area);
if(w!=1.)q.scale(w);
}
for(int k=0;
k<3;
++k)vquad[f.v[k]]+=q;
}
}
void _v(){
vquad.assign(nV,Quadric());
B6=0.;
int activeF=0;
if(_a)for(int fi=0;
fi<nF;
++fi)if(!(fi<(int)FD.size()&&FD[fi])){
B6+=_a==3?(facePix[fi]+10.*faceSil[fi]):(facePix[fi]+12.*faceSil[fi]+2.*faceWin[fi]);
++activeF;
}
if(_a)B6/=max(1,activeF);
_b.assign(nV,0.);
if(_a==2||_a==4)for(int fi=0;
fi<nF;
++fi)if(!(fi<(int)FD.size()&&FD[fi]))for(int k=0;
k<3;
++k)_b[ZF[fi].v[k]]+=faceSil[fi]+0.05*facePix[fi]+0.10*faceWin[fi];
for(int fi=0;
fi<nF;
++fi){
if(fi<(int)FD.size()&&FD[fi])continue;
const FC&f=ZF[fi];
V3 n=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB area=0.5*norm(n);
Quadric q=Quadric::_T1(ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]);
if(_a){
DB imp=_a==3?(facePix[fi]+10.*faceSil[fi]):(facePix[fi]+12.*faceSil[fi]+2.*faceWin[fi]);
DB r=_a==3?(imp+0.20*B6)/(B6+1e-12):(imp+0.12*B6)/(B6+1e-12);
DB w;
if(_a==2)w=min(18.,max(0.06,0.08+0.92*pow(max(0.,r),0.90)));
else if(_a==3)w=min(7.,max(0.12,0.15+0.85*sqrt(max(0.,r))));
else w=min(16.,max(0.06,0.08+0.92*pow(max(0.,r),0.72)));
if(w!=1.)q.scale(w);
}
else if(area>=1e-30){
n=n/(2.*area);
DB w=_A3(n,area);
if(w!=1.)q.scale(w);
}
for(int k=0;
k<3;
++k)vquad[f.v[k]]+=q;
}
if(_a==2||_a==4){
DB am=0;
int ac=0;
for(int i=0;
i<nV;
++i)if(!ZD[i]){
am+=_b[i];
++ac;
}
am/=max(1,ac);
if(_frag&&_a==2){
DB z=am+1;
for(int i=0;
i<nV;
++i)if(!ZD[i])_b[i]+=z*min(12.,2.4*_fd[i]);
am=0;
ac=0;
for(int i=0;
i<nV;
++i)if(!ZD[i]){
am+=_b[i];
++ac;
}
am/=max(1,ac);
}
_M4=am;
DB bw=(_a==2?0.0000035:0.0000050)*diag*diag/(am+1.);
for(int i=0;
i<nV;
++i)if(!ZD[i]&&_b[i]>0)vquad[i]+=_x2(ZV[i],bw*min(18.,_b[i]/(am+1e-12)));
if(_a==4||(_frag&&_a==2))for(int i=0;
i<nV;
++i)if(!ZD[i])vquad[i]+=vmoment[i];
}
}
Quadric _t3(int fi)const{
const FC&f=ZF[fi];
V3 n=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB area=0.5*norm(n);
Quadric q=Quadric::_T1(ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]);
if(_a&&fi<(int)facePix.size()){
DB imp=facePix[fi]+12.*faceSil[fi]+2.*faceWin[fi];
DB r=(imp+0.12*B6)/(B6+1e-12);
DB w=_a==2?min(18.,max(0.06,0.08+0.92*pow(max(0.,r),0.90))):min(16.,max(0.06,0.08+0.92*pow(max(0.,r),0.72)));
if(w!=1.)q.scale(w);
}
else if(area>=1e-30){
n=n/(2.*area);
DB w=_A3(n,area);
if(w!=1.)q.scale(w);
}
return q;
}
void _k1(int a,int b,const Quadric&q,V3 pos[9],int&np)const{
np=0;
V3 qp;
if(_X4(q,qp))pos[np++]=qp;
if(IV>400000){
V3 ed=ZV[b]-ZV[a];
DB el=norm(ed);
if(el>1e-30){
DB t=max(0.,min(1.,0.5*(1.+(crad[b]-crad[a])/el)));
pos[np++]=ZV[a]+ed*t;
}
}
if(_frag&&_a==2){
Quadric z=vmoment[a];
z+=vmoment[b];
V3 p;
if(_X4(z,p)){
pos[np++]=p;
if(_N4(qp))pos[np++]=(p+qp)*.5;
}
DB x=1+min(12.,_b[a]/(_M4+1e-12)),y=1+min(12.,_b[b]/(_M4+1e-12));
pos[np++]=(ZV[a]*x+ZV[b]*y)/(x+y);
V3 e=ZV[b]-ZV[a];
pos[np++]=ZV[a]+e/3.;
pos[np++]=ZV[a]+e*(2./3.);
}
pos[np++]=(ZV[a]+ZV[b])*0.5;
pos[np++]=ZV[a];
pos[np++]=ZV[b];
int wp=0;
for(int i=0;
i<np;
++i){
if(!_N4(pos[i]))continue;
bool dup=false;
for(int j=0;
j<wp;
++j)if(norm2(pos[i]-pos[j])<1e-30){
dup=true;
break;
}
if(!dup)pos[wp++]=pos[i];
}
np=wp;
}
A1 _n2(int ab,int kp,const V3&p,const Quadric&q)const{
A1 c;
c._o=ab;
c.kept=kp;
c._i3=vver[ab];
c._o4=vver[kp];
c._t=p;
DB r=q._b5(p);
if(!_frag||_a!=2){
c.cost=r;
return c;
}
if(r>costCap)return A1();
DB a=min(12.,_b[ab]/(_M4+1e-12)),b=min(12.,_b[kp]/(_M4+1e-12)),d=max(0.,a-b),mv=norm2(p-ZV[kp])*_D4;
uint32_t h=ab*73856093u^kp*19349663u^targetV*83492791u;
DB j=(h&1023)/1023.-.5;
c.cost=r*(1+.62*d+.035*j)/(1+.16*max(0.,1.-a))+costCap*(.0011*a+.075*b*mv);
return c;
}
DB _O2(int a,int b)const{
Quadric q=vquad[a];
q+=vquad[b];
V3 mid=(ZV[a]+ZV[b])*0.5;
return q._b5(mid);
}
A1 _j1(int a,int b)const{
Quadric q=vquad[a];
q+=vquad[b];
V3 pos[9];
int np;
_k1(a,b,q,pos,np);
A1 best;
for(int i=0;
i<np;
++i){
A1 c1=_n2(a,b,pos[i],q);
if(c1 .cost<best.cost||(c1 .cost==best.cost&&IV<=400000&&((_a&&c1 .kept<(int)_b.size()?_b[c1 .kept]:0.)>(_a&&best.kept<(int)_b.size()?_b[best.kept]:0.)||((_a&&c1 .kept<(int)_b.size()?_b[c1 .kept]:0.)==(_a&&best.kept<(int)_b.size()?_b[best.kept]:0.)&&ZN[c1 .kept].size()>ZN[best.kept].size()))))best=c1;
A1 c2=_n2(b,a,pos[i],q);
if(c2 .cost<best.cost||(c2 .cost==best.cost&&IV<=400000&&((_a&&c2 .kept<(int)_b.size()?_b[c2 .kept]:0.)>(_a&&best.kept<(int)_b.size()?_b[best.kept]:0.)||((_a&&c2 .kept<(int)_b.size()?_b[c2 .kept]:0.)==(_a&&best.kept<(int)_b.size()?_b[best.kept]:0.)&&ZN[c2 .kept].size()>ZN[best.kept].size()))))best=c2;
}
return best;
}
bool _l3(int a,int b,const V3&p,DB&mr)const{
mr=max(crad[a]+norm(ZV[a]-p),crad[b]+norm(ZV[b]-p));
return mr<=hausd;
}
A1 _X(int a,int b)const{
Quadric q=vquad[a];
q+=vquad[b];
V3 pos[9];
int np;
_k1(a,b,q,pos,np);
A1 best;
for(int i=0;
i<np;
++i){
for(int dir=0;
dir<2;
++dir){
int ab=dir?b:a,kp=dir?a:b;
DB mr;
if(!_l3(ab,kp,pos[i],mr))continue;
A1 c=_n2(ab,kp,pos[i],q);
c._E=mr;
if(c.cost<best.cost||(c.cost==best.cost&&IV<=400000&&((_a&&c.kept<(int)_b.size()?_b[c.kept]:0.)>(_a&&best.kept<(int)_b.size()?_b[best.kept]:0.)||((_a&&c.kept<(int)_b.size()?_b[c.kept]:0.)==(_a&&best.kept<(int)_b.size()?_b[best.kept]:0.)&&ZN[c.kept].size()>ZN[best.kept].size()))))best=c;
}
}
return best;
}
__attribute__((noinline,section(".text.largeheap")))void _s20(){
VC<A1>h;
h.reserve(nF*2);
for(int a=0;
a<nV;
++a)for(int b:ZN[a])if(a<b){
auto c=_j1(a,b);
if(c.valid())h.push_back(c);
}
pq=PQ<A1>(less<A1>(),std::move(h));
}
void _s2(){
for(int a=0;
a<nV;
++a)for(int b:ZN[a])if(a<b){
auto c=_j1(a,b);
if(c.valid())pq.push(c);
}
}
DB ET()const{
return chrono::duration<DB>(chrono::steady_clock::now()-_T4).count();
}
bool _n1(int a,int b)const{
return a>=0&&b>=0&&a<(int)ZV.size()&&b<(int)ZV.size()&&!ZD[a]&&!ZD[b]&&ZN[a]._z3(b);
}
int _A(int a,int b)const{
int cnt=0;
const auto&fa=ZI[a];
const auto&fb=ZI[b];
if(fa.size()<fb.size()){
for(int f:fa){
if(!FD[f])for(int f2:fb)if(f==f2){
++cnt;
break;
}
}
}
else{
for(int f:fb){
if(!FD[f])for(int f2:fa)if(f==f2){
++cnt;
break;
}
}
}
return cnt;
}
int B7(int a,int b)const{
int cnt=0;
const auto&na=ZN[a];
const auto&nb=ZN[b];
if(na.size()<nb.size()){
for(int x:na){
if(x!=a&&x!=b&&!ZD[x]&&nb._z3(x))++cnt;
}
}
else{
for(int x:nb){
if(x!=a&&x!=b&&!ZD[x]&&na._z3(x))++cnt;
}
}
return cnt;
}
static void _B4(VC<int>&v,int x){
for(int i=(int)v.size()-1;
i>=0;
--i)if(v[i]==x){
v[i]=v.back();
v.pop_back();
return;
}
}
bool _r1(const A1&c)const{
int a=c._o,b=c.kept;
VC<int>patch;
patch.reserve(ZI[a].size()+ZI[b].size());
for(int fi:ZI[a])if(!FD[fi])patch.push_back(fi);
for(int fi:ZI[b])if(!FD[fi])patch.push_back(fi);
sort(patch.begin(),patch.end());
patch.erase(unique(patch.begin(),patch.end()),patch.end());
VC<array<int,3>>newKeys;
newKeys.reserve(patch.size());
for(int fi:patch){
const FC&f=ZF[fi];
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
V3 op[3]={
ZV[id[0]],ZV[id[1]],ZV[id[2]]}
;
for(int k=0;
k<3;
++k)if(id[k]==a)id[k]=b;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
V3 np[3]={
id[0]==b?c._t:ZV[id[0]],id[1]==b?c._t:ZV[id[1]],id[2]==b?c._t:ZV[id[2]]}
;
V3 on=cross(op[1]-op[0],op[2]-op[0]),nn=cross(np[1]-np[0],np[2]-np[0]);
DB ol=norm(on),nl=norm(nn);
if(ol<A2||nl<max(A2,1e-11*diag*diag))return false;
DB d=dot(on,nn)/(ol*nl);
if(d<(_frag&&_a==2?.008:.10))return false;
DB ar=nl/ol;
if(ar<(_frag&&_a==2?.002:.015)||ar>(_frag&&_a==2?160.:45.))return false;
array<int,3>key={
id[0],id[1],id[2]}
;
sort(key.begin(),key.end());
newKeys.push_back(key);
}
sort(newKeys.begin(),newKeys.end());
for(int i=1;
i<(int)newKeys.size();
++i)if(newKeys[i]==newKeys[i-1])return false;
return true;
}
void _i1(){
int tick=0;
while(_j<B1&&!pq.empty()){
++tick;
int _x5=(IV>400000?8191:4095);
if((tick&_x5)==0&&IV>25000&&ET()>_S)break;
auto c=pq.top();
pq.pop();
int a=c._o,b=c.kept;
if(!_n1(a,b))continue;
if(c._i3!=vver[a]||c._o4!=vver[b]){
auto fr=_j1(a,b);
if(fr.valid())pq.push(fr);
continue;
}
if(!_frag&&c.cost>costCap)break;
if(_A(a,b)!=2)continue;
if(B7(a,b)!=2)continue;
auto best=_X(a,b);
if(!best.valid()||best.cost>costCap)continue;
if(_F1&&!_r1(best))continue;
_D1(best._o,best.kept,best._t,best._E);
++_j;
if(IV>400000&&(_j==IV/5||_j==2*IV/5||_j==3*IV/5||_j==4*IV/5)&&ET()<_S-1.5)_s20();
}
}
void _D1(int ab,int kp,const V3&np,DB nr){
if(_frag&&_a==2){
_b[kp]=max(_b[kp],.94*_b[ab]);
_fd[kp]=max(_fd[kp],.96*_fd[ab]);
}
ZV[kp]=np;
crad[kp]=nr;
crad[ab]=0;
ZD[ab]=1;
++vver[ab];
++vver[kp];
auto abFaces=ZI[ab];
VC<int>dead;
dead.reserve(4);
for(int fi:abFaces){
if(FD[fi])continue;
bool touched=false;
for(int k=0;
k<3;
++k)if(ZF[fi].v[k]==ab){
ZF[fi].v[k]=kp;
touched=true;
}
if(!touched)continue;
if(ZF[fi].v[0]==ZF[fi].v[1]||ZF[fi].v[1]==ZF[fi].v[2]||ZF[fi].v[0]==ZF[fi].v[2]){
FD[fi]=1;
dead.push_back(fi);
}
else ZI[kp].push_back(fi);
}
for(int fi:dead)for(int k=0;
k<3;
++k){
int v=ZF[fi].v[k];
if(v>=0&&v<(int)ZI.size())_B4(ZI[v],fi);
}
ZI[ab].clear();
if(kp<(int)vmoment.size()&&ab<(int)vmoment.size())vmoment[kp]+=vmoment[ab];
if(MEMLESS){
Quadric fresh;
for(int fi:ZI[kp]){
if(FD[fi])continue;
const FC&f=ZF[fi];
fresh+=((_a==2||_a==4)?_t3(fi):Quadric::_T1(ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]));
}
if((_a==4||(_frag&&_a==2))&&kp<(int)vmoment.size())fresh+=vmoment[kp];
vquad[kp]=fresh;
}
else{
vquad[kp]+=vquad[ab];
}
for(int nb:ZN[ab]){
if(nb==kp||ZD[nb])continue;
ZN[nb].erase(ab);
ZN[nb].insert(kp);
ZN[kp].insert(nb);
}
ZN[ab].clear();
ZN[kp].erase(ab);
ZN[kp].erase(kp);
for(int nb:ZN[kp]){
if(nb==kp||ZD[nb])continue;
auto c=_j1(kp,nb);
if(c.valid())pq.push(c);
}
}
static DB _v1(DB x,DB lo,DB hi){
return x<lo?lo:(x>hi?hi:x);
}
static DB _z(const V3&p,const V3&a,const V3&b,const V3&c){
V3 ab=b-a,ac=c-a,ap=p-a;
DB d1=dot(ab,ap),d2=dot(ac,ap);
if(d1<=0.&&d2<=0.)return norm2(ap);
V3 bp=p-b;
DB d3=dot(ab,bp),d4=dot(ac,bp);
if(d3>=0.&&d4<=d3)return norm2(bp);
DB vc=d1*d4-d3*d2;
if(vc<=0.&&d1>=0.&&d3<=0.){
DB v=d1/(d1-d3);
V3 q=a+ab*v;
return norm2(p-q);
}
V3 cp=p-c;
DB d5=dot(ab,cp),d6=dot(ac,cp);
if(d6>=0.&&d5<=d6)return norm2(cp);
DB vb=d5*d2-d1*d6;
if(vb<=0.&&d2>=0.&&d6<=0.){
DB w=d2/(d2-d6);
V3 q=a+ac*w;
return norm2(p-q);
}
DB va=d3*d6-d5*d4;
if(va<=0.&&(d4-d3)>=0.&&(d5-d6)>=0.){
DB w=(d4-d3)/((d4-d3)+(d5-d6));
V3 q=b+(c-b)*w;
return norm2(p-q);
}
V3 n=cross(ab,ac);
DB nn=norm2(n);
if(nn<A2*A2)return _k;
DB dist=dot(p-a,n);
return(dist*dist)/nn;
}
struct _m3{
int R=0;
array<VC<float>,6>z;
VC<char>_T2,B4;
}
;
void _V3(_m3&h,int R)const{
h.R=R;
h._T2 .assign(nF,0);
for(int view=0;
view<6;
++view){
h.z[view].assign((size_t)R*R,1e30f);
VC<int>id((size_t)R*R,-1);
for(int fi=0;
fi<nF;
++fi){
if(FD[fi])continue;
const FC&f=ZF[fi];
auto a=_n(ZV[f.v[0]],view,R),b=_n(ZV[f.v[1]],view,R),c=_n(ZV[f.v[2]],view,R);
if(!a.ok||!b.ok||!c.ok)continue;
DB den=(b.v-c.v)*(a.u-c.u)+(c.u-b.u)*(a.v-c.v);
if(fabs(den)<1e-18)continue;
int x0=max(0,(int)floor(min({
a.u,b.u,c.u}
))),x1=min(R-1,(int)ceil(max({
a.u,b.u,c.u}
))),y0=max(0,(int)floor(min({
a.v,b.v,c.v}
))),y1=min(R-1,(int)ceil(max({
a.v,b.v,c.v}
)));
for(int y=y0;
y<=y1;
++y)for(int x=x0;
x<=x1;
++x){
DB X=x+.5,Y=y+.5,w0=((b.v-c.v)*(X-c.u)+(c.u-b.u)*(Y-c.v))/den,w1=((c.v-a.v)*(X-c.u)+(a.u-c.u)*(Y-c.v))/den,w2=1-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/a.z+w1/b.z+w2/c.z;
if(iz<=0)continue;
float zz=(float)(1./iz);
int q=y*R+x;
if(zz<h.z[view][q]){
h.z[view][q]=zz;
id[q]=fi;
}
}
}
for(int q=0;
q<R*R;
++q)if(id[q]>=0)h._T2[id[q]]=1;
}
h.B4 .assign(nV,0);
for(int fi=0;
fi<nF;
++fi)if(!FD[fi]&&h._T2[fi])for(int k=0;
k<3;
++k)h.B4[ZF[fi].v[k]]=1;
VC<char>p=h.B4;
for(int v=0;
v<nV;
++v)if(p[v]&&!ZD[v])for(int nb:ZN[v])if(!ZD[nb])h.B4[nb]=1;
}
bool _R2(const A1&c,VC<int>&patch,VC<array<V3,3>>&oldTris,VC<array<V3,3>>&newTris)const{
int a=c._o,b=c.kept;
patch.clear();
for(int fi:ZI[a])if(!FD[fi])patch.push_back(fi);
for(int fi:ZI[b])if(!FD[fi])patch.push_back(fi);
sort(patch.begin(),patch.end());
patch.erase(unique(patch.begin(),patch.end()),patch.end());
oldTris.clear();
newTris.clear();
for(int fi:patch){
const FC&f=ZF[fi];
V3 op[3]={
ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]}
;
oldTris.push_back({
op[0],op[1],op[2]}
);
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
for(int k=0;
k<3;
++k)if(id[k]==a)id[k]=b;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
V3 np[3]={
id[0]==b?c._t:ZV[id[0]],id[1]==b?c._t:ZV[id[1]],id[2]==b?c._t:ZV[id[2]]}
;
V3 on=cross(op[1]-op[0],op[2]-op[0]),nn=cross(np[1]-np[0],np[2]-np[0]);
DB ol=norm(on),nl=norm(nn);
if(ol<A2||nl<1e-10*diag*diag)return false;
if(dot(on,nn)<=0.02*ol*nl)return false;
newTris.push_back({
np[0],np[1],np[2]}
);
}
if(newTris.empty())return false;
auto worst=[&](const VC<array<V3,3>>&A,const VC<array<V3,3>>&B){
DB w=0;
for(auto&t:A){
V3 s[7]={
t[0],t[1],t[2],(t[0]+t[1])*.5,(t[1]+t[2])*.5,(t[2]+t[0])*.5,(t[0]+t[1]+t[2])/3.}
;
for(V3&p:s){
DB d=_k;
for(auto&q:B)d=min(d,_z(p,q[0],q[1],q[2]));
w=max(w,d);
}
}
return w;
}
;
DB w2=max(worst(oldTris,newTris),worst(newTris,oldTris));
return isfinite(w2)&&w2<=(0.72*hausd)*(0.72*hausd);
}
bool _O3(const _m3&h,const A1&c,const VC<int>&patch)const{
for(int fi:patch)if(fi>=0&&fi<nF&&!FD[fi]&&h._T2[fi])return false;
int a=c._o,b=c.kept;
DB margin=max(2e-5,0.0012*diag);
for(int view=0;
view<6;
++view)for(int fi:patch){
if(FD[fi])continue;
const FC&f=ZF[fi];
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
for(int k=0;
k<3;
++k)if(id[k]==a)id[k]=b;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
V3 pp[3]={
id[0]==b?c._t:ZV[id[0]],id[1]==b?c._t:ZV[id[1]],id[2]==b?c._t:ZV[id[2]]}
;
auto p0=_n(pp[0],view,h.R),p1=_n(pp[1],view,h.R),p2=_n(pp[2],view,h.R);
if(!p0 .ok||!p1 .ok||!p2 .ok)return false;
DB den=(p1 .v-p2 .v)*(p0 .u-p2 .u)+(p2 .u-p1 .u)*(p0 .v-p2 .v);
if(fabs(den)<1e-18)return false;
int x0=max(0,(int)floor(min({
p0 .u,p1 .u,p2 .u}
))),x1=min(h.R-1,(int)ceil(max({
p0 .u,p1 .u,p2 .u}
))),y0=max(0,(int)floor(min({
p0 .v,p1 .v,p2 .v}
))),y1=min(h.R-1,(int)ceil(max({
p0 .v,p1 .v,p2 .v}
)));
for(int y=y0;
y<=y1;
++y)for(int x=x0;
x<=x1;
++x){
DB X=x+.5,Y=y+.5,w0=((p1 .v-p2 .v)*(X-p2 .u)+(p2 .u-p1 .u)*(Y-p2 .v))/den,w1=((p2 .v-p0 .v)*(X-p2 .u)+(p0 .u-p2 .u)*(Y-p2 .v))/den,w2=1-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/p0 .z+w1/p1 .z+w2/p2 .z;
if(iz<=0)return false;
DB zz=1./iz;
float front=h.z[view][y*h.R+x];
if(!isfinite(front)||front>1e20f||zz<=front+margin)return false;
}
}
return true;
}
struct _v4{
int a,b;
DB score;
bool operator<(const _v4&o)const{
return score<o.score;
}
}
;
int _O(int _M3=0){
int active=0;
for(int v=0;
v<nV;
++v)if(!ZD[v])++active;
if(active<20)return 0;
int tier=_M3?_M3:(nV<=5000?1:(nV<=25000?2:(nV<=45000?3:(nV<=50000?4:(nV<=400000?5:6)))));
int R=_M3?(tier<=4?512:(tier==5?256:160)):(nV<=50000?512:(nV<=400000?256:160));
_m3 h;
_V3(h,R);
VC<_v4>cand;
cand.reserve(active*2);
for(int a=0;
a<nV;
++a)if(!ZD[a]&&!h.B4[a])for(int b:ZN[a])if(a<b&&!ZD[b]&&!h.B4[b]){
DB s=(_O2(a,b)+1e-5*norm2(ZV[a]-ZV[b]))*0.7;
cand.push_back({
a,b,s}
);
}
sort(cand.begin(),cand.end());
DB frac=tier<=4?0.12:(tier==5?0.035:0.018);
int hard=tier==1?350:(tier==2?1600:(tier==3?2200:(tier==4?2800:(tier==5?3500:4500))));
int cap=min(hard,(int)floor(active*frac)),done=0;
VC<int>patch;
VC<array<V3,3>>ot,nt;
for(auto&e:cand){
if(done>=cap||ET()>A0-0.18)break;
if(!_n1(e.a,e.b)||h.B4[e.a]||h.B4[e.b])continue;
if(_A(e.a,e.b)!=2||B7(e.a,e.b)!=2)continue;
auto c=_X(e.a,e.b);
if(!c.valid())continue;
if(h.B4[c._o]||h.B4[c.kept])continue;
if(!_R2(c,patch,ot,nt))continue;
if(!_O3(h,c,patch))continue;
_D1(c._o,c.kept,c._t,c._E);
++_j;
++done;
}
return done;
}
V3 _m2(int fi)const{
const FC&f=ZF[fi];
return cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
}
bool _C4(int fi,int v)const{
const FC&f=ZF[fi];
return f.v[0]==v||f.v[1]==v||f.v[2]==v;
}
struct _c{
int _x1;
DB _f4;
DB _N;
DB _z4;
DB _g3;
int hardCap;
int _u;
int rounds;
DB _F3;
DB _K2;
}
;
int _F()const{
if(IV<=50000){
if(nV>=1000000)return 6;
if(nV>=350000)return 5;
if(nV>=45000)return 4;
if(nV>=35000)return 3;
if(nV>=20000)return 2;
return 1;
}
if(IV>400000)return 6;
return 5;
}
_c _M2()const{
_c p1={
12,0.160,0.220,1.18,0.0400,30000,820000,8,0.90,6.20}
;
_c p2={
5,0.004,0.006,0.40,0.0015,900,90000,1,0.22,0.75}
;
_c p3={
6,0.008,0.012,0.52,0.0030,1800,140000,2,0.32,1.20}
;
_c p4={
6,0.010,0.014,0.58,0.0035,2200,150000,2,0.35,1.35}
;
_c p5={
5,0.006,0.009,0.46,0.0022,1300,115000,1,0.27,0.95}
;
_c p6={
6,0.008,0.012,0.52,0.0030,1800,140000,2,0.32,1.20}
;
int tier=_F();
if(tier==1)return p1;
if(tier==2)return p2;
if(tier==3)return p3;
if(tier==4)return p4;
if(tier==5)return p5;
return p6;
}
_c _u4()const{
_c p0={
0,0,0,0,0,0,0,0,0,0}
;
_c p2={
8,0.024,0.040,0.90,0.0200,1250,175000,1,0.44,1.95}
;
_c p3={
8,0.026,0.042,0.92,0.0180,1250,170000,1,0.45,1.95}
;
int tier=_F();
if(tier==2)return p2;
if(tier==3)return p3;
if(tier==4)return p0;
return p0;
}
DB _Y4()const{
int tier=_F();
if(tier==2)return 0.95;
if(tier==3)return 0.940;
if(tier==4)return 1.01;
return 1.01;
}
DB _H4()const{
int tier=_F();
if(tier==2)return 0.052;
if(tier==3)return 0.057;
if(tier==4)return 0.;
return 0.;
}
bool A8(int v,const _c&sp,VC<int>&ring,VC<int>&inc)const{
ring.clear();
inc.clear();
if(sp._x1<=0)return false;
if(v<0||v>=(int)ZI.size()||ZD[v])return false;
for(int fi:ZI[v]){
if(FD[fi])continue;
if(_C4(fi,v))inc.push_back(fi);
}
int m=(int)inc.size();
if(m<3||m>sp._x1)return false;
VC<pair<int,int>>dir;
dir.reserve(m);
for(int fi:inc){
const FC&f=ZF[fi];
int pos=-1;
for(int k=0;
k<3;
++k)if(f.v[k]==v)pos=k;
if(pos<0)return false;
int a=f.v[(pos+1)%3],b=f.v[(pos+2)%3];
if(a==b||a==v||b==v||ZD[a]||ZD[b])return false;
dir.push_back({
a,b}
);
}
for(int i=0;
i<m;
++i)for(int j=i+1;
j<m;
++j){
if(dir[i].first==dir[j].first)return false;
if(dir[i].second==dir[j].second)return false;
}
int start=dir[0].first,cur=start;
ring.push_back(start);
for(int step=0;
step<m;
++step){
int nxt=-1;
for(auto&e:dir)if(e.first==cur){
nxt=e.second;
break;
}
if(nxt<0)return false;
if(step==m-1){
if(nxt!=start)return false;
}
else{
for(int x:ring)if(x==nxt)return false;
ring.push_back(nxt);
cur=nxt;
}
}
return(int)ring.size()==m;
}
bool _S1(int a,int b,int c,const VC<int>&skip)const{
array<int,3>key={
a,b,c}
;
sort(key.begin(),key.end());
for(int fi:ZI[a]){
if(FD[fi])continue;
bool _g5=false;
for(int s:skip)if(s==fi){
_g5=true;
break;
}
if(_g5)continue;
const FC&f=ZF[fi];
array<int,3>k2={
f.v[0],f.v[1],f.v[2]}
;
sort(k2 .begin(),k2 .end());
if(k2==key)return true;
}
return false;
}
struct A4{
int v=-1,root=0;
DB score=_k;
bool valid()const{
return v>=0&&score<_k;
}
bool operator<(const A4&o)const{
return score<o.score;
}
}
;
bool _p1(int v,const VC<int>&ring,const VC<int>&inc,int root,DB oldDev,const V3&avgN,const _c&sp,A4&out)const{
int m=(int)ring.size();
VC<int>rr;
rr.reserve(m);
for(int i=0;
i<m;
++i)rr.push_back(ring[(root+i)%m]);
int r0=rr[0];
for(int i=2;
i<=m-2;
++i){
if(ZN[r0]._z3(rr[i]))return false;
}
DB _N=0.;
DB _E4=_k;
for(int i=1;
i<m-1;
++i){
int a=rr[0],b=rr[i],c=rr[i+1];
if(a==b||b==c||a==c)return false;
if(_S1(a,b,c,inc))return false;
V3 n=cross(ZV[b]-ZV[a],ZV[c]-ZV[a]);
DB nl=norm(n);
if(nl<A2)return false;
V3 un=n/nl;
DB d=_v1(dot(un,avgN),-1.,1.);
if(d<=0.)return false;
_N=max(_N,1.-d);
if(_N>sp._N)return false;
_E4=min(_E4,_z(ZV[v],ZV[a],ZV[b],ZV[c]));
}
DB dist=sqrt(max(0.,_E4));
if(crad[v]+dist>hausd*sp._z4)return false;
out.v=v;
out.root=root;
out.score=(crad[v]+dist)/(hausd+A2)+0.35*oldDev+0.25*_N+1e-4*m;
return true;
}
A4 _e2(int v)const{
_c sp=_M2();
return A5(v,sp);
}
A4 A5(int v,const _c&sp)const{
A4 best;
VC<int>ring,inc;
if(!A8(v,sp,ring,inc))return best;
V3 avg;
DB areaSum=0.,oldDev=0.;
for(int fi:inc){
V3 n=_m2(fi);
DB nl=norm(n);
if(nl<A2)return best;
avg=avg+n;
areaSum+=0.5*nl;
}
DB al=norm(avg);
if(al<A2||areaSum<=0.)return best;
avg=avg/al;
for(int fi:inc){
V3 n=_m2(fi);
DB nl=norm(n);
V3 un=n/nl;
DB d=_v1(dot(un,avg),-1.,1.);
if(d<=0.)return best;
oldDev=max(oldDev,1.-d);
}
if(oldDev>sp._f4)return best;
for(int root=0;
root<(int)ring.size();
++root){
A4 c;
if(_p1(v,ring,inc,root,oldDev,avg,sp,c)&&c.score<best.score)best=c;
}
return best;
}
struct _C1{
DB frac=0.,_W4=0.,_j5=0.,minDot=1.01;
int _V4=0;
}
;
_C1 _g4()const{
_C1 z;
if(!_Y2)return z;
int tier=_F();
if(_P==1){
if(tier==4)return{
0.055,0.0012,0.96,0.992,34}
;
}
else if(_P==2){
if(tier==2)return{
0.035,0.0008,0.92,0.995,28}
;
if(tier==4)return{
0.050,0.0011,0.94,0.993,34}
;
}
else if(_P==3){
if(tier==2)return{
0.045,0.0010,0.94,0.993,30}
;
if(tier==3)return{
0.030,0.0007,0.90,0.996,30}
;
if(tier==4)return{
0.065,0.0015,0.98,0.990,36}
;
}
return z;
}
bool _G3(int root,const V3&target,const _C1&rp){
if(rp.frac<=0.||root<0||root>=(int)ZD.size()||ZD[root])return false;
if((int)ZI[root].size()>rp._V4)return false;
V3 cur=ZV[root];
V3 delta=(target-cur)*rp.frac;
DB dl=norm(delta);
if(dl<A2)return false;
DB cap=diag*rp._W4;
if(cap<=0.)return false;
if(dl>cap){
delta=delta*(cap/dl);
dl=cap;
}
if(crad[root]+dl>hausd*rp._j5)return false;
V3 np=cur+delta;
for(int fi:ZI[root]){
if(FD[fi])continue;
const FC&f=ZF[fi];
V3 oldN=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB oldLen=norm(oldN);
if(oldLen<A2)return false;
V3 p[3]={
ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]}
;
for(int k=0;
k<3;
++k)if(f.v[k]==root)p[k]=np;
V3 newN=cross(p[1]-p[0],p[2]-p[0]);
DB newLen=norm(newN);
if(newLen<A2)return false;
DB d=_v1(dot(oldN/oldLen,newN/newLen),-1.,1.);
if(d<rp.minDot)return false;
}
ZV[root]=np;
crad[root]+=dl;
++vver[root];
return true;
}
bool _Z3(int v){
A4 best=_e2(v);
if(!best.valid())return false;
return _p(v,best.root,_M2());
}
bool _p(int v,int root,const _c&sp){
A4 best=A5(v,sp);
if(!best.valid())return false;
root=best.root;
VC<int>ring,inc;
if(!A8(v,sp,ring,inc))return false;
int m=(int)ring.size();
VC<int>rr;
rr.reserve(m);
for(int i=0;
i<m;
++i)rr.push_back(ring[(root+i)%m]);
V3 _k5=ZV[v];
for(int fi:inc){
if(FD[fi])continue;
FC old=ZF[fi];
FD[fi]=1;
for(int k=0;
k<3;
++k){
int u=old.v[k];
if(u>=0&&u<(int)ZI.size())_B4(ZI[u],fi);
}
}
for(int nb:ring)if(nb>=0&&nb<(int)ZN.size())ZN[nb].erase(v);
ZN[v].clear();
ZI[v].clear();
ZD[v]=1;
crad[v]=0.;
++vver[v];
for(int i=1;
i<m-1;
++i){
FC nf;
nf.v[0]=rr[0];
nf.v[1]=rr[i];
nf.v[2]=rr[i+1];
int fi=(int)ZF.size();
ZF.push_back(nf);
FD.push_back(0);
for(int k=0;
k<3;
++k)ZI[nf.v[k]].push_back(fi);
for(int k=0;
k<3;
++k){
int a=nf.v[k],b=nf.v[(k+1)%3];
if(a!=b){
ZN[a].insert(b);
ZN[b].insert(a);
}
}
}
_C1 rp=_g4();
if(rp.frac>0.&&!rr.empty())_G3(rr[0],_k5,rp);
return true;
}
bool _p4()const{
return _F()==4;
}
_c _m5()const{
if(_F()==4)return{
6,0.015,0.024,0.66,0.0110,760,280000,1,0.52,2.30}
;
return{
0,0,0,0,0,0,0,0,0,0}
;
}
bool _z1(int v,const _c&sp)const{
VC<int>ring,inc;
return A8(v,sp,ring,inc)&&(int)ring.size()>=4&&(int)ring.size()<=sp._x1;
}
void _m1(){
if(!_p4())return;
_c sp=_m5();
if(sp._x1<=0||sp.hardCap<=0||sp._u<=0)return;
DB _c1=A0-ET();
if(_c1<0.45)return;
DB _G=ET()+min(sp._K2,_c1*sp._F3);
int _y=min(sp.hardCap,max(0,(int)floor(nV*sp._g3)));
if(_y<=0)return;
VC<A4>cands;
cands.reserve(min(sp._u,_y*12+512));
int scanned=0;
for(int v=0;
v<(int)ZV.size()&&scanned<sp._u&&ET()<_G;
++v){
if(ZD[v])continue;
++scanned;
if(!_z1(v,sp))continue;
A4 c=A5(v,sp);
if(c.valid())cands.push_back(c);
}
if(cands.empty())return;
sort(cands.begin(),cands.end());
int extra=0;
for(const A4&c:cands){
if(extra>=_y||ET()>=_G)break;
if(c.v<0||c.v>=(int)ZD.size()||ZD[c.v])continue;
if(!_z1(c.v,sp))continue;
if(_p(c.v,c.root,sp)){
++_j;
++extra;
}
}
}
struct B0{
int a=-1,b=-1,root=0;
DB score=_k;
VC<int>_Y3;
VC<int>_Q3;
bool valid()const{
return a>=0&&b>=0&&score<_k&&!_Y3 .empty()&&!_Q3 .empty();
}
bool operator<(const B0&o)const{
return score<o.score;
}
}
;
_c _s4()const{
if(IV>25000&&IV<=400000)return{
7,.010,.018,.62,.001,36,90000,1,.24,.60}
;
return{
0,0,0,0,0,0,0,0,0,0}
;
}
static long long _i5(int a,int b){
if(a>b)swap(a,b);
return((long long)a<<32)^(unsigned int)b;
}
bool _D3(const VC<pair<int,int>>&edges,VC<int>&ring)const{
ring.clear();
if(edges.size()<4||edges.size()>16)return false;
VC<int>_h;
_h.reserve(edges.size()*2);
for(auto&e:edges){
_h.push_back(e.first);
_h.push_back(e.second);
}
sort(_h.begin(),_h.end());
_h.erase(unique(_h.begin(),_h.end()),_h.end());
if(_h.size()!=edges.size())return false;
VC<VC<int>>adj(_h.size());
for(auto&e:edges){
int ia=(int)(lower_bound(_h.begin(),_h.end(),e.first)-_h.begin());
int ib=(int)(lower_bound(_h.begin(),_h.end(),e.second)-_h.begin());
if(ia<0||ib<0||ia==ib||ia>=(int)_h.size()||ib>=(int)_h.size())return false;
adj[ia].push_back(ib);
adj[ib].push_back(ia);
}
for(auto&v:adj)if(v.size()!=2)return false;
int start=0,prev=-1,cur=start;
for(int step=0;
step<(int)_h.size();
++step){
ring.push_back(_h[cur]);
int n0=adj[cur][0],n1=adj[cur][1];
int nxt=(n0==prev)?n1:n0;
prev=cur;
cur=nxt;
if(cur==start)return step+1==(int)_h.size();
}
return false;
}
DB _W1(const VC<array<V3,3>>&src,const VC<array<V3,3>>&dst)const{
if(src.empty()||dst.empty())return _k;
DB worst=0.;
for(const auto&t:src){
V3 samples[7]={
t[0],t[1],t[2],(t[0]+t[1])*0.5,(t[1]+t[2])*0.5,(t[2]+t[0])*0.5,(t[0]+t[1]+t[2])/3.}
;
for(const V3&p:samples){
DB best=_k;
for(const auto&q:dst)best=min(best,_z(p,q[0],q[1],q[2]));
worst=max(worst,best);
}
}
return worst;
}
bool _y2(int a,int b,const _c&sp,B0&out)const{
out=B0();
if(!_n1(a,b)||sp._x1<=0)return false;
VC<int>patch;
patch.reserve(ZI[a].size()+ZI[b].size());
for(int fi:ZI[a])if(!FD[fi])patch.push_back(fi);
for(int fi:ZI[b])if(!FD[fi])patch.push_back(fi);
sort(patch.begin(),patch.end());
patch.erase(unique(patch.begin(),patch.end()),patch.end());
if(patch.size()<4||patch.size()>24)return false;
map<long long,pair<int,int>>_A4;
map<long long,int>_L4;
for(int fi:patch){
const FC&f=ZF[fi];
bool touches=false;
for(int k=0;
k<3;
++k)if(f.v[k]==a||f.v[k]==b)touches=true;
if(!touches)return false;
for(int k=0;
k<3;
++k){
int x=f.v[k],y=f.v[(k+1)%3];
long long key=_i5(x,y);
_A4[key]={
min(x,y),max(x,y)}
;
++_L4[key];
}
}
VC<pair<int,int>>_y3;
for(auto&kv:_L4){
if(kv.second!=1)continue;
int x=_A4[kv.first].first,y=_A4[kv.first].second;
if(x==a||x==b||y==a||y==b)return false;
if(ZD[x]||ZD[y])return false;
_y3 .push_back({
x,y}
);
}
VC<int>ring;
if(!_D3(_y3,ring))return false;
int m=(int)ring.size();
if(m<4||m>sp._x1)return false;
V3 avg;
DB oldDev=0.;
for(int fi:patch){
V3 n=_m2(fi);
DB nl=norm(n);
if(nl<A2)return false;
avg=avg+n;
}
DB al=norm(avg);
if(al<A2)return false;
avg=avg/al;
for(int fi:patch){
V3 n=_m2(fi);
DB nl=norm(n);
V3 un=n/nl;
DB d=_v1(dot(un,avg),-1.,1.);
if(d<=0.)return false;
oldDev=max(oldDev,1.-d);
}
if(oldDev>sp._f4)return false;
VC<array<V3,3>>oldTris;
oldTris.reserve(patch.size());
for(int fi:patch){
const FC&f=ZF[fi];
oldTris.push_back({
ZV[f.v[0]],ZV[f.v[1]],ZV[f.v[2]]}
);
}
B0 best;
for(int root=0;
root<m;
++root){
VC<int>rr;
rr.reserve(m);
for(int i=0;
i<m;
++i)rr.push_back(ring[(root+i)%m]);
int r0=rr[0];
bool ok=true;
for(int i=2;
i<=m-2&&ok;
++i)if(ZN[r0]._z3(rr[i]))ok=false;
if(!ok)continue;
DB _N=0.;
VC<array<V3,3>>newTris;
newTris.reserve(max(0,m-2));
for(int i=1;
i<m-1&&ok;
++i){
int x=rr[0],y=rr[i],z=rr[i+1];
if(x==y||y==z||x==z){
ok=false;
break;
}
if(_S1(x,y,z,patch)){
ok=false;
break;
}
V3 n=cross(ZV[y]-ZV[x],ZV[z]-ZV[x]);
DB nl=norm(n);
if(nl<A2){
ok=false;
break;
}
V3 un=n/nl;
DB d=_v1(dot(un,avg),-1.,1.);
if(d<=0.){
ok=false;
break;
}
_N=max(_N,1.-d);
if(_N>sp._N){
ok=false;
break;
}
newTris.push_back({
ZV[x],ZV[y],ZV[z]}
);
}
if(!ok||newTris.empty())continue;
DB worst2=max(_W1(oldTris,newTris),_W1(newTris,oldTris));
if(!isfinite(worst2))continue;
DB worst=sqrt(max(0.,worst2));
DB _U3=max(crad[a],crad[b])+worst;
if(_U3>hausd*sp._z4)continue;
B0 cand;
cand.a=a;
cand.b=b;
cand.root=root;
cand._Y3=ring;
cand._Q3=patch;
cand.score=_U3/(hausd+A2)+0.35*oldDev+0.25*_N+1e-4*m;
if(cand.score<best.score)best=cand;
}
if(!best.valid())return false;
out=best;
return true;
}
bool _n3(const B0&c,const _c&sp){
if(c.a<0||c.b<0||ZD[c.a]||ZD[c.b]||!_n1(c.a,c.b))return false;
B0 fresh;
if(!_y2(c.a,c.b,sp,fresh))return false;
if(!fresh.valid())return false;
int m=(int)fresh._Y3 .size();
VC<int>rr;
rr.reserve(m);
for(int i=0;
i<m;
++i)rr.push_back(fresh._Y3[(fresh.root+i)%m]);
for(int fi:fresh._Q3){
if(FD[fi])continue;
FC old=ZF[fi];
FD[fi]=1;
for(int k=0;
k<3;
++k){
int u=old.v[k];
if(u>=0&&u<(int)ZI.size())_B4(ZI[u],fi);
}
}
VC<int>na=ZN[fresh.a].data,nb=ZN[fresh.b].data;
for(int x:na)if(x>=0&&x<(int)ZN.size())ZN[x].erase(fresh.a);
for(int x:nb)if(x>=0&&x<(int)ZN.size())ZN[x].erase(fresh.b);
ZN[fresh.a].clear();
ZN[fresh.b].clear();
ZI[fresh.a].clear();
ZI[fresh.b].clear();
ZD[fresh.a]=1;
ZD[fresh.b]=1;
crad[fresh.a]=0.;
crad[fresh.b]=0.;
++vver[fresh.a];
++vver[fresh.b];
for(int i=1;
i<m-1;
++i){
FC nf;
nf.v[0]=rr[0];
nf.v[1]=rr[i];
nf.v[2]=rr[i+1];
int fi=(int)ZF.size();
ZF.push_back(nf);
FD.push_back(0);
for(int k=0;
k<3;
++k)ZI[nf.v[k]].push_back(fi);
for(int k=0;
k<3;
++k){
int x=nf.v[k],y=nf.v[(k+1)%3];
if(x!=y){
ZN[x].insert(y);
ZN[y].insert(x);
}
}
}
return true;
}
void _U1(int lm=0,bool ignoreDeadline=false){
_c sp=_s4();
if(sp._x1<=0||sp.hardCap<=0||sp._u<=0)return;
DB _c1=A0-ET();
if(!ignoreDeadline&&_c1<0.55)return;
DB _G=ignoreDeadline?1e100:ET()+min(sp._K2,_c1*sp._F3);
int _y=min(sp.hardCap,max(0,(int)floor(nV*sp._g3)));
if(lm)_y=min(_y,lm);
if(_y<=0)return;
VC<B0>cands;
cands.reserve(min(sp._u,_y*8+256));
int scanned=0;
for(int a=0;
a<(int)ZV.size()&&scanned<sp._u&&ET()<_G;
++a){
if(ZD[a])continue;
for(int b:ZN[a]){
if(scanned>=sp._u||ET()>=_G)break;
if(b<=a||ZD[b])continue;
++scanned;
B0 c;
if(_y2(a,b,sp,c))cands.push_back(c);
}
}
if(cands.empty())return;
sort(cands.begin(),cands.end());
int extra=0;
for(const B0&c:cands){
if(extra>=_y||ET()>=_G)break;
if(c.a<0||c.b<0||c.a>=(int)ZD.size()||c.b>=(int)ZD.size())continue;
if(ZD[c.a]||ZD[c.b])continue;
if(_n3(c,sp)){
_j+=2;
extra+=2;
}
}
}
struct _P1{
DB u=0.,v=0.,z=0.;
bool ok=false;
}
;
struct VegaTri{
V3 p[3];
}
;
struct _d{
float n[3];
float d;
unsigned char fg;
}
;
struct _k2{
int v=-1,root=0;
DB ssim=1.,score=_k;
bool operator<(const _k2&o)const{
return score<o.score;
}
}
;
static _d _w1(){
_d p;
p.n[0]=127.5f;
p.n[1]=127.5f;
p.n[2]=127.5f;
p.d=255.f;
p.fg=0;
return p;
}
void _n4(int view,V3&eye,V3&right,V3&up,V3&fwd)const{
switch(view){
case 0:eye=V3(2.5,0,0);
break;
case 1:eye=V3(-2.5,0,0);
break;
case 2:eye=V3(0,2.5,0);
break;
case 3:eye=V3(0,-2.5,0);
break;
case 4:eye=V3(0,0,2.5);
break;
default:eye=V3(0,0,-2.5);
break;
}
fwd=eye*(-1.);
DB fl=norm(fwd);
if(fl<A2)fwd=V3(0,0,-1);
else fwd=fwd/fl;
V3 worldUp=(fabs(dot(fwd,V3(0,0,1)))>0.9)?V3(0,1,0):V3(0,0,1);
right=cross(worldUp,fwd);
DB rl=norm(right);
if(rl<A2)right=V3(1,0,0);
else right=right/rl;
up=cross(fwd,right);
DB ul=norm(up);
if(ul<A2)up=V3(0,1,0);
else up=up/ul;
}
_P1 A6(const V3&p,int view,int R)const{
V3 eye,right,up,fwd;
_n4(view,eye,right,up,fwd);
V3 rel=p-eye;
DB x=dot(rel,right),y=dot(rel,up),z=dot(rel,fwd);
if(z<=1e-8)return{
}
;
DB scale=DB(R)/1024.;
DB f=800.*scale;
DB c=0.5*DB(R);
return{
f*x/z+c,f*y/z+c,z,true}
;
}
bool _Z2(int v,int root,const _c&sp,VC<VegaTri>&oldTris,VC<VegaTri>&newTris)const{
oldTris.clear();
newTris.clear();
VC<int>ring,inc;
if(!A8(v,sp,ring,inc))return false;
A4 check=A5(v,sp);
if(!check.valid())return false;
root=check.root;
for(int fi:inc){
const FC&f=ZF[fi];
VegaTri t;
t.p[0]=ZV[f.v[0]];
t.p[1]=ZV[f.v[1]];
t.p[2]=ZV[f.v[2]];
oldTris.push_back(t);
}
int m=(int)ring.size();
VC<int>rr;
rr.reserve(m);
for(int i=0;
i<m;
++i)rr.push_back(ring[(root+i)%m]);
for(int i=1;
i<m-1;
++i){
VegaTri t;
t.p[0]=ZV[rr[0]];
t.p[1]=ZV[rr[i]];
t.p[2]=ZV[rr[i+1]];
newTris.push_back(t);
}
return!oldTris.empty()&&!newTris.empty();
}
DB _S3()const{
int tier=_F();
if(tier==2)return 0.80;
if(tier==3)return 0.42;
if(tier==4)return 0.;
return 1e100;
}
DB _e1(const VC<VegaTri>&src,const VC<VegaTri>&dst)const{
if(src.empty()||dst.empty())return _k;
DB worst=0.;
for(const VegaTri&t:src){
V3 samples[7]={
t.p[0],t.p[1],t.p[2],(t.p[0]+t.p[1])*0.5,(t.p[1]+t.p[2])*0.5,(t.p[2]+t.p[0])*0.5,(t.p[0]+t.p[1]+t.p[2])/3.}
;
for(const V3&p:samples){
DB best=_k;
for(const VegaTri&q:dst)best=min(best,_z(p,q.p[0],q.p[1],q.p[2]));
worst=max(worst,best);
}
}
return worst;
}
DB _d1(const VC<VegaTri>&oldTris,const VC<VegaTri>&newTris)const{
DB d2=max(_e1(oldTris,newTris),_e1(newTris,oldTris));
if(!isfinite(d2))return _k;
return sqrt(max(0.,d2));
}
static DB _C(const _d&p,int ch){
if(ch<3)return p.n[ch];
return p.d;
}
DB _R1(const VC<_d>&a,const VC<_d>&b,int ch)const{
DB sx=0,sy=0,sxx=0,syy=0,sxy=0;
int cnt=0;
int n=(int)a.size();
for(int i=0;
i<n;
++i){
if(!a[i].fg&&!b[i].fg)continue;
DB x=_C(a[i],ch),y=_C(b[i],ch);
sx+=x;
sy+=y;
sxx+=x*x;
syy+=y*y;
sxy+=x*y;
++cnt;
}
if(cnt<4)return 1.;
DB inv=1./DB(cnt);
DB mx=sx*inv,my=sy*inv;
DB vx=max(0.,sxx*inv-mx*mx),vy=max(0.,syy*inv-my*my);
DB cov=sxy*inv-mx*my;
DB num=(2.*mx*my+_i2)*(2.*cov+_j2);
DB den=(mx*mx+my*my+_i2)*(vx+vy+_j2);
if(den<=0.)return 1.;
return _v1(num/den,-1.,1.);
}
DB _u2(const VC<_d>&a,const VC<_d>&b)const{
int cnt=0;
for(int i=0;
i<(int)a.size();
++i)if(a[i].fg||b[i].fg)++cnt;
if(cnt==0)return 1.;
DB sn=(_R1(a,b,0)+_R1(a,b,1)+_R1(a,b,2))/3.;
DB sd=_R1(a,b,3);
return _a1*sn+(1.-_a1)*sd;
}
bool _l1(const VC<VegaTri>&tris,int view,int x0,int y0,int w,int h,VC<_d>&buf,int R=B8)const{
_d bg=_w1();
buf.assign(w*h,bg);
VC<float>zbuf(w*h,NL<float>::infinity());
for(const VegaTri&tri:tris){
_P1 p0=A6(tri.p[0],view,R);
_P1 p1=A6(tri.p[1],view,R);
_P1 p2=A6(tri.p[2],view,R);
if(!p0 .ok||!p1 .ok||!p2 .ok)continue;
DB area2=(p1 .u-p0 .u)*(p2 .v-p0 .v)-(p1 .v-p0 .v)*(p2 .u-p0 .u);
if(fabs(area2)<1e-12)continue;
V3 nr=cross(tri.p[1]-tri.p[0],tri.p[2]-tri.p[0]);
DB nl=norm(nr);
if(nl<A2)continue;
nr=nr/nl;
int bx0=max(x0,(int)floor(min({
p0 .u,p1 .u,p2 .u}
)));
int bx1=min(x0+w-1,(int)ceil(max({
p0 .u,p1 .u,p2 .u}
)));
int by0=max(y0,(int)floor(min({
p0 .v,p1 .v,p2 .v}
)));
int by1=min(y0+h-1,(int)ceil(max({
p0 .v,p1 .v,p2 .v}
)));
if(bx0>bx1||by0>by1)continue;
DB den=(p1 .v-p2 .v)*(p0 .u-p2 .u)+(p2 .u-p1 .u)*(p0 .v-p2 .v);
if(fabs(den)<1e-18)continue;
for(int py=by0;
py<=by1;
++py){
for(int px=bx0;
px<=bx1;
++px){
DB sx=px+0.5,sy=py+0.5;
DB w0=((p1 .v-p2 .v)*(sx-p2 .u)+(p2 .u-p1 .u)*(sy-p2 .v))/den;
DB w1=((p2 .v-p0 .v)*(sx-p2 .u)+(p0 .u-p2 .u)*(sy-p2 .v))/den;
DB w2=1.-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/p0 .z+w1/p1 .z+w2/p2 .z;
if(iz<=0.)continue;
DB z=1./iz;
int idx=(py-y0)*w+(px-x0);
if(z<zbuf[idx]){
zbuf[idx]=(float)z;
buf[idx].n[0]=(float)((nr.x+1.)*127.5);
buf[idx].n[1]=(float)((nr.y+1.)*127.5);
buf[idx].n[2]=(float)((nr.z+1.)*127.5);
buf[idx].d=(float)z;
buf[idx].fg=1;
}
}
}
}
return true;
}
void _Q1(const VC<V3>&vv,const VC<FC>&ff,int view,int R,VC<_d>&buf)const{
_d bg=_w1();
buf.assign(R*R,bg);
VC<float>zbuf(R*R,NL<float>::infinity());
for(const FC&f:ff){
VegaTri tri;
tri.p[0]=vv[f.v[0]];
tri.p[1]=vv[f.v[1]];
tri.p[2]=vv[f.v[2]];
_P1 p0=A6(tri.p[0],view,R);
_P1 p1=A6(tri.p[1],view,R);
_P1 p2=A6(tri.p[2],view,R);
if(!p0 .ok||!p1 .ok||!p2 .ok)continue;
DB area2=(p1 .u-p0 .u)*(p2 .v-p0 .v)-(p1 .v-p0 .v)*(p2 .u-p0 .u);
if(fabs(area2)<1e-12)continue;
V3 nr=cross(tri.p[1]-tri.p[0],tri.p[2]-tri.p[0]);
DB nl=norm(nr);
if(nl<A2)continue;
nr=nr/nl;
int bx0=max(0,(int)floor(min({
p0 .u,p1 .u,p2 .u}
)));
int bx1=min(R-1,(int)ceil(max({
p0 .u,p1 .u,p2 .u}
)));
int by0=max(0,(int)floor(min({
p0 .v,p1 .v,p2 .v}
)));
int by1=min(R-1,(int)ceil(max({
p0 .v,p1 .v,p2 .v}
)));
if(bx0>bx1||by0>by1)continue;
DB den=(p1 .v-p2 .v)*(p0 .u-p2 .u)+(p2 .u-p1 .u)*(p0 .v-p2 .v);
if(fabs(den)<1e-18)continue;
for(int py=by0;
py<=by1;
++py){
for(int px=bx0;
px<=bx1;
++px){
DB sx=px+0.5,sy=py+0.5;
DB w0=((p1 .v-p2 .v)*(sx-p2 .u)+(p2 .u-p1 .u)*(sy-p2 .v))/den;
DB w1=((p2 .v-p0 .v)*(sx-p2 .u)+(p0 .u-p2 .u)*(sy-p2 .v))/den;
DB w2=1.-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/p0 .z+w1/p1 .z+w2/p2 .z;
if(iz<=0.)continue;
DB z=1./iz;
int idx=py*R+px;
if(z<zbuf[idx]){
zbuf[idx]=(float)z;
buf[idx].n[0]=(float)((nr.x+1.)*127.5);
buf[idx].n[1]=(float)((nr.y+1.)*127.5);
buf[idx].n[2]=(float)((nr.z+1.)*127.5);
buf[idx].d=(float)z;
buf[idx].fg=1;
}
}
}
}
}
DB _s1(const A1&c)const{
VC<int>patch=ZI[c._o];
for(int fi:ZI[c.kept])if(find(patch.begin(),patch.end(),fi)==patch.end())patch.push_back(fi);
VC<VegaTri>ot,nt;
ot.reserve(patch.size());
nt.reserve(patch.size());
for(int fi:patch){
if(fi<0||fi>=(int)ZF.size()||FD[fi])continue;
const FC&f=ZF[fi];
VegaTri a;
a.p[0]=ZV[f.v[0]];
a.p[1]=ZV[f.v[1]];
a.p[2]=ZV[f.v[2]];
ot.push_back(a);
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
for(int k=0;
k<3;
++k)if(id[k]==c._o)id[k]=c.kept;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
VegaTri b;
for(int k=0;
k<3;
++k)b.p[k]=id[k]==c.kept?c._t:ZV[id[k]];
nt.push_back(b);
}
if(ot.empty()||nt.empty())return-1.;
if(_d1(ot,nt)>hausd)return-1.;
const int R=512;
DB total=0;
int used=0;
VC<_d>a,b;
for(int view=0;
view<6;
++view){
DB mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;
bool any=false;
auto inc=[&](const VegaTri&t){
for(int k=0;
k<3;
++k){
_P1 p=A6(t.p[k],view,R);
if(!p.ok)continue;
any=true;
mnU=min(mnU,p.u);
mxU=max(mxU,p.u);
mnV=min(mnV,p.v);
mxV=max(mxV,p.v);
}
}
;
for(const auto&t:ot)inc(t);
for(const auto&t:nt)inc(t);
if(!any)continue;
int x0=max(0,(int)floor(mnU)-5),y0=max(0,(int)floor(mnV)-5),x1=min(R-1,(int)ceil(mxU)+5),y1=min(R-1,(int)ceil(mxV)+5);
if(x0>x1||y0>y1)continue;
int w=x1-x0+1,h=y1-y0+1;
if(w*h>52000)return-1.;
_l1(ot,view,x0,y0,w,h,a);
_l1(nt,view,x0,y0,w,h,b);
total+=_u2(a,b);
++used;
}
return used?total/used:1.;
}
DB _E1(const A1&c,const VC<VC<_d>>&refBufs)const{
VC<int>patch=ZI[c._o];
for(int fi:ZI[c.kept])if(find(patch.begin(),patch.end(),fi)==patch.end())patch.push_back(fi);
VC<VegaTri>ot,nt;
ot.reserve(patch.size());
nt.reserve(patch.size());
for(int fi:patch){
if(fi<0||fi>=(int)ZF.size()||FD[fi])continue;
const FC&f=ZF[fi];
VegaTri a;
a.p[0]=ZV[f.v[0]];
a.p[1]=ZV[f.v[1]];
a.p[2]=ZV[f.v[2]];
ot.push_back(a);
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
for(int k=0;
k<3;
++k)if(id[k]==c._o)id[k]=c.kept;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
VegaTri b;
for(int k=0;
k<3;
++k)b.p[k]=id[k]==c.kept?c._t:ZV[id[k]];
nt.push_back(b);
}
if(ot.empty()||nt.empty())return-1.;
if(_d1(ot,nt)>hausd)return-1.;
const int R=B8;
DB total=0;
int used=0;
VC<_d>a,b;
for(int view=0;
view<6;
++view){
if(view>=(int)refBufs.size())continue;
DB mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;
bool any=false;
auto inc=[&](const VegaTri&t){
for(int k=0;
k<3;
++k){
_P1 p=A6(t.p[k],view,R);
if(!p.ok)continue;
any=true;
mnU=min(mnU,p.u);
mxU=max(mxU,p.u);
mnV=min(mnV,p.v);
mxV=max(mxV,p.v);
}
}
;
for(const auto&t:ot)inc(t);
for(const auto&t:nt)inc(t);
if(!any)continue;
int x0=max(0,(int)floor(mnU)-5),y0=max(0,(int)floor(mnV)-5),x1=min(R-1,(int)ceil(mxU)+5),y1=min(R-1,(int)ceil(mxV)+5);
if(x0>x1||y0>y1)continue;
int w=x1-x0+1,h=y1-y0+1;
if(w*h>52000)return-1.;
a.clear();
a.reserve(w*h);
const VC<_d>&ref=refBufs[view];
for(int py=y0;
py<=y1;
++py)for(int px=x0;
px<=x1;
++px)a.push_back(ref[py*R+px]);
_l1(nt,view,x0,y0,w,h,b);
total+=_u2(a,b);
++used;
}
return used?total/used:1.;
}
struct _o2{
int a,b;
DB damage,cost;
bool operator<(const _o2&o)const{
return damage<o.damage||(damage==o.damage&&cost<o.cost);
}
}
;
void _d2(bool _d4){
DB left=A0-ET();
if(IV>25000&&left<1.2)return;
DB stop=ET()+min(2.,left*0.42);
bool _N3=!_U.empty()&&!_T.empty();
VC<VC<_d>>refBufs;
if(_N3){
refBufs.resize(6);
for(int view=0;
view<6;
++view)_Q1(_U,_T,view,B8,refBufs[view]);
}
VC<_o2>cs;
cs.reserve(1024);
int scanned=0;
for(int a=0;
a<(int)ZV.size()&&scanned<3600&&(IV<=25000||ET()<stop);
++a)if(!ZD[a]){
for(int b:ZN[a]){
if(b<=a||ZD[b]||scanned>=3600||(IV>25000&&ET()>=stop))continue;
++scanned;
if(_A(a,b)!=2||B7(a,b)!=2)continue;
auto c=_X(a,b);
if(!c.valid()||c.cost>costCap)continue;
DB s=_N3?_E1(c,refBufs):_s1(c);
if(s>=0.99950)cs.push_back({
a,b,1.-s,c.cost}
);
}
}
sort(cs.begin(),cs.end());
VC<char>lock(ZV.size(),0);
int cap=max(12,min(300,(int)floor(IV*0.0042))),done=0;
for(const auto&e:cs){
if(done>=cap||(IV>25000&&ET()>=stop))break;
if(!_n1(e.a,e.b)||_A(e.a,e.b)!=2||B7(e.a,e.b)!=2)continue;
if(_d4&&(lock[e.a]||lock[e.b]))continue;
auto c=_X(e.a,e.b);
if(!c.valid()||c.cost>costCap)continue;
DB s=_N3?_E1(c,refBufs):_s1(c);
if(s<0.99950)continue;
if(_d4){
VC<int>near=ZN[e.a].data;
for(int x:ZN[e.b])near.push_back(x);
lock[e.a]=lock[e.b]=1;
for(int x:near)if(x>=0&&x<(int)lock.size())lock[x]=1;
}
_D1(c._o,c.kept,c._t,c._E);
++_j;
++done;
}
}
struct _m{
int x0=0,y0=0,x1=-1,y1=-1;
bool valid()const{
return x0<=x1&&y0<=y1;
}
}
;
struct _b3{
array<VC<DB>,4>_b1;
VC<int>_P2;
DB sum[4]={
0,0,0,0}
;
long long count=0;
}
;
struct _p2{
array<_b3,6>view;
_l score;
}
;
struct _B1{
A1 c;
array<_m,6>centers;
DB _y4[6][4]={
{
0}
}
;
long long _J3[6]={
0,0,0,0,0,0}
;
DB _D2=-1.;
DB _W2=-1.;
DB damage=_k;
DB _t2=0.;
}
;
static bool _W3(const _m&a,const _m&b){
if(!a.valid()||!b.valid())return false;
return!(a.x1<b.x0||b.x1<a.x0||a.y1<b.y0||b.y1<a.y0);
}
static DB _r2(const VC<DB>&p,int S,const _m&r){
if(!r.valid())return 0.;
int x0=r.x0,y0=r.y0,x1=r.x1+1,y1=r.y1+1;
return p[(size_t)y1*S+x1]-p[(size_t)y0*S+x1]-p[(size_t)y1*S+x0]+p[(size_t)y0*S+x0];
}
static long long _r2(const VC<int>&p,int S,const _m&r){
if(!r.valid())return 0;
int x0=r.x0,y0=r.y0,x1=r.x1+1,y1=r.y1+1;
return(long long)p[(size_t)y1*S+x1]-p[(size_t)y0*S+x1]-p[(size_t)y1*S+x0]+p[(size_t)y0*S+x0];
}
static DB _q1(DB sx,DB sy,DB sxx,DB syy,DB sxy){
CX DB inv=1./121.;
DB mx=sx*inv,my=sy*inv;
DB vx=max(0.,sxx*inv-mx*mx),vy=max(0.,syy*inv-my*my);
DB cov=sxy*inv-mx*my;
DB num=(2.*mx*my+_i2)*(2.*cov+_j2);
DB den=(mx*mx+my*my+_i2)*(vx+vy+_j2);
if(den<=0.)return 1.;
return _v1(num/den,-1.,1.);
}
void _l2(const VC<_d>&ref,const VC<_d>&cur,int R,_b3&out)const{
int S=R+1;
size_t N=(size_t)S*S;
out._P2 .assign(N,0);
for(int ch=0;
ch<4;
++ch)out._b1[ch].assign(N,0.);
VC<DB>ix(N),iy(N),ix2(N),iy2(N),ixy(N);
for(int ch=0;
ch<4;
++ch){
fill(ix.begin(),ix.end(),0.);
fill(iy.begin(),iy.end(),0.);
fill(ix2 .begin(),ix2 .end(),0.);
fill(iy2 .begin(),iy2 .end(),0.);
fill(ixy.begin(),ixy.end(),0.);
for(int y=0;
y<R;
++y){
DB sx=0,sy=0,sxx=0,syy=0,sxy=0;
size_t prow=(size_t)y*S,row=(size_t)(y+1)*S;
for(int x=0;
x<R;
++x){
DB a=_C(ref[y*R+x],ch),b=_C(cur[y*R+x],ch);
sx+=a;
sy+=b;
sxx+=a*a;
syy+=b*b;
sxy+=a*b;
size_t q=row+x+1,u=prow+x+1;
ix[q]=ix[u]+sx;
iy[q]=iy[u]+sy;
ix2[q]=ix2[u]+sxx;
iy2[q]=iy2[u]+syy;
ixy[q]=ixy[u]+sxy;
}
}
auto rect=[&](const VC<DB>&I,int x0,int y0,int x1,int y1){
return I[(size_t)y1*S+x1]-I[(size_t)y0*S+x1]-I[(size_t)y1*S+x0]+I[(size_t)y0*S+x0];
}
;
DB total=0.;
for(int y=0;
y<R;
++y){
DB rowSum=0.;
size_t prow=(size_t)y*S,row=(size_t)(y+1)*S;
for(int x=0;
x<R;
++x){
DB val=0.;
if(x>=5&&x<R-5&&y>=5&&y<R-5&&(ref[y*R+x].fg||cur[y*R+x].fg)){
int x0=x-5,y0=y-5,x1=x+6,y1=y+6;
val=_q1(rect(ix,x0,y0,x1,y1),rect(iy,x0,y0,x1,y1),rect(ix2,x0,y0,x1,y1),rect(iy2,x0,y0,x1,y1),rect(ixy,x0,y0,x1,y1));
}
rowSum+=val;
out._b1[ch][row+x+1]=out._b1[ch][prow+x+1]+rowSum;
total+=val;
}
}
out.sum[ch]=total;
}
long long _x4=0;
for(int y=0;
y<R;
++y){
int _f5=0;
size_t prow=(size_t)y*S,row=(size_t)(y+1)*S;
for(int x=0;
x<R;
++x){
int add=(x>=5&&x<R-5&&y>=5&&y<R-5&&(ref[y*R+x].fg||cur[y*R+x].fg))?1:0;
_f5+=add;
out._P2[row+x+1]=out._P2[prow+x+1]+_f5;
_x4+=add;
}
}
out.count=_x4;
}
_l _b2(const DB sums[6][4],const long long counts[6])const{
_l sc;
sc._f=0.;
sc.minView=1.;
sc._w=1.;
sc._R=1.;
for(int v=0;
v<6;
++v){
DB den=counts[v]>0?DB(counts[v]):1.;
DB sn=(sums[v][0]+sums[v][1]+sums[v][2])/(3.*den);
DB sd=sums[v][3]/den;
DB sv=0.5*(sn+sd);
sc._f+=sv;
sc.minView=min(sc.minView,sv);
sc._w=min(sc._w,sn);
sc._R=min(sc._R,sd);
}
sc._f/=6.;
return sc;
}
_p2 _V2(const VC<VC<_d>>&ref,const VC<VC<_d>>&cur,int R)const{
_p2 L;
DB sums[6][4]={
{
0}
}
;
long long counts[6]={
0,0,0,0,0,0}
;
for(int v=0;
v<6;
++v){
_l2(ref[v],cur[v],R,L.view[v]);
for(int ch=0;
ch<4;
++ch)sums[v][ch]=L.view[v].sum[ch];
counts[v]=L.view[v].count;
}
L.score=_b2(sums,counts);
return L;
}
bool _q3(const A1&c,array<_m,6>&_x3,array<_m,6>&centers,array<_m,6>&samples)const{
VC<int>patch=ZI[c._o];
for(int fi:ZI[c.kept])if(find(patch.begin(),patch.end(),fi)==patch.end())patch.push_back(fi);
VC<VegaTri>oldTris,newTris;
oldTris.reserve(patch.size());
newTris.reserve(patch.size());
for(int fi:patch){
if(fi<0||fi>=(int)ZF.size()||FD[fi])continue;
const FC&f=ZF[fi];
VegaTri a;
for(int k=0;
k<3;
++k)a.p[k]=ZV[f.v[k]];
oldTris.push_back(a);
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
for(int k=0;
k<3;
++k)if(id[k]==c._o)id[k]=c.kept;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
VegaTri b;
for(int k=0;
k<3;
++k)b.p[k]=(id[k]==c.kept?c._t:ZV[id[k]]);
newTris.push_back(b);
}
if(oldTris.empty()||newTris.empty())return false;
if(_d1(oldTris,newTris)>hausd)return false;
const int R=_b4?_b4:B5;
bool anyView=false;
for(int view=0;
view<6;
++view){
DB mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;
bool any=false;
auto inc=[&](const VegaTri&t){
for(int k=0;
k<3;
++k){
auto p=A6(t.p[k],view,R);
if(!p.ok)continue;
any=true;
mnU=min(mnU,p.u);
mnV=min(mnV,p.v);
mxU=max(mxU,p.u);
mxV=max(mxV,p.v);
}
}
;
for(auto&t:oldTris)inc(t);
for(auto&t:newTris)inc(t);
if(!any){
_x3[view]=centers[view]=samples[view]=_m();
continue;
}
_m a{
max(0,(int)floor(mnU)),max(0,(int)floor(mnV)),min(R-1,(int)ceil(mxU)),min(R-1,(int)ceil(mxV))}
;
if(!a.valid()){
_x3[view]=centers[view]=samples[view]=_m();
continue;
}
_m ce{
max(5,a.x0-5),max(5,a.y0-5),min(R-6,a.x1+5),min(R-6,a.y1+5)}
;
_m sa{
max(0,ce.x0-5),max(0,ce.y0-5),min(R-1,ce.x1+5),min(R-1,ce.y1+5)}
;
if(!ce.valid()||!sa.valid()||(sa.x1-sa.x0+1)*(sa.y1-sa.y0+1)>_I1)return false;
_x3[view]=a;
centers[view]=ce;
samples[view]=sa;
anyView=true;
}
return anyView;
}
void _o3(int R){
_b4=R;
int T=(R+_V-1)/_V;
_d3 .assign(6,VC<VC<int>>(T*T));
for(int view=0;
view<6;
++view)for(int fi=0;
fi<(int)ZF.size();
++fi){
if(FD[fi])continue;
const FC&f=ZF[fi];
auto a=A6(ZV[f.v[0]],view,R),b=A6(ZV[f.v[1]],view,R),c=A6(ZV[f.v[2]],view,R);
if(!a.ok||!b.ok||!c.ok)continue;
int x0=max(0,(int)floor(min({
a.u,b.u,c.u}
))),x1=min(R-1,(int)ceil(max({
a.u,b.u,c.u}
))),y0=max(0,(int)floor(min({
a.v,b.v,c.v}
))),y1=min(R-1,(int)ceil(max({
a.v,b.v,c.v}
)));
if(x0>x1||y0>y1)continue;
for(int ty=y0/_V;
ty<=y1/_V;
++ty)for(int tx=x0/_V;
tx<=x1/_V;
++tx)_d3[view][ty*T+tx].push_back(fi);
}
}
void _V1(const A1&c,int view,const _m&r,VC<_d>&buf)const{
int w=r.x1-r.x0+1,h=r.y1-r.y0+1;
_d bg=_w1();
buf.assign((size_t)w*h,bg);
VC<float>zbuf((size_t)w*h,NL<float>::infinity());
int R=_b4?_b4:B5;
VC<int>_B;
if(_b4==R&&view>=0&&view<(int)_d3 .size()){
int T=(R+_V-1)/_V;
for(int ty=r.y0/_V;
ty<=r.y1/_V;
++ty)for(int tx=r.x0/_V;
tx<=r.x1/_V;
++tx){
const auto&bin=_d3[view][ty*T+tx];
_B.insert(_B.end(),bin.begin(),bin.end());
}
sort(_B.begin(),_B.end());
_B.erase(unique(_B.begin(),_B.end()),_B.end());
}
else{
_B.resize(ZF.size());
iota(_B.begin(),_B.end(),0);
}
for(int fi:_B){
if(FD[fi])continue;
const FC&f=ZF[fi];
int id[3]={
f.v[0],f.v[1],f.v[2]}
;
for(int k=0;
k<3;
++k)if(id[k]==c._o)id[k]=c.kept;
if(id[0]==id[1]||id[1]==id[2]||id[0]==id[2])continue;
V3 pp[3];
for(int k=0;
k<3;
++k)pp[k]=(id[k]==c.kept?c._t:ZV[id[k]]);
auto p0=A6(pp[0],view,R),p1=A6(pp[1],view,R),p2=A6(pp[2],view,R);
if(!p0 .ok||!p1 .ok||!p2 .ok)continue;
int bx0=max(r.x0,(int)floor(min({
p0 .u,p1 .u,p2 .u}
))),bx1=min(r.x1,(int)ceil(max({
p0 .u,p1 .u,p2 .u}
))),by0=max(r.y0,(int)floor(min({
p0 .v,p1 .v,p2 .v}
))),by1=min(r.y1,(int)ceil(max({
p0 .v,p1 .v,p2 .v}
)));
if(bx0>bx1||by0>by1)continue;
DB den=(p1 .v-p2 .v)*(p0 .u-p2 .u)+(p2 .u-p1 .u)*(p0 .v-p2 .v);
if(fabs(den)<1e-18)continue;
V3 nr=cross(pp[1]-pp[0],pp[2]-pp[0]);
DB nl=norm(nr);
if(nl<A2)continue;
nr=nr/nl;
for(int py=by0;
py<=by1;
++py)for(int px=bx0;
px<=bx1;
++px){
DB sx=px+.5,sy=py+.5;
DB w0=((p1 .v-p2 .v)*(sx-p2 .u)+(p2 .u-p1 .u)*(sy-p2 .v))/den,w1=((p2 .v-p0 .v)*(sx-p2 .u)+(p0 .u-p2 .u)*(sy-p2 .v))/den,w2=1-w0-w1;
if(w0<-1e-9||w1<-1e-9||w2<-1e-9)continue;
DB iz=w0/p0 .z+w1/p1 .z+w2/p2 .z;
if(iz<=0)continue;
float z=(float)(1./iz);
int q=(py-r.y0)*w+(px-r.x0);
if(z>=zbuf[q])continue;
zbuf[q]=z;
buf[q].n[0]=(float)((nr.x+1)*127.5);
buf[q].n[1]=(float)((nr.y+1)*127.5);
buf[q].n[2]=(float)((nr.z+1)*127.5);
buf[q].d=z;
buf[q].fg=1;
}
}
}
void _I2(const VC<_d>&refFull,const VC<_d>&_k3,const _m&sample,const _m&center,int R,DB sums[4],long long&count)const{
int w=sample.x1-sample.x0+1,h=sample.y1-sample.y0+1,S=w+1;
size_t N=(size_t)(h+1)*S;
VC<DB>ix(N),iy(N),ix2(N),iy2(N),ixy(N);
count=0;
for(int ch=0;
ch<4;
++ch){
fill(ix.begin(),ix.end(),0.);
fill(iy.begin(),iy.end(),0.);
fill(ix2 .begin(),ix2 .end(),0.);
fill(iy2 .begin(),iy2 .end(),0.);
fill(ixy.begin(),ixy.end(),0.);
for(int y=0;
y<h;
++y){
DB sx=0,sy=0,sxx=0,syy=0,sxy=0;
size_t prow=(size_t)y*S,row=(size_t)(y+1)*S;
for(int x=0;
x<w;
++x){
const _d&a=refFull[(sample.y0+y)*R+sample.x0+x];
const _d&b=_k3[y*w+x];
DB X=_C(a,ch),Y=_C(b,ch);
sx+=X;
sy+=Y;
sxx+=X*X;
syy+=Y*Y;
sxy+=X*Y;
size_t q=row+x+1,u=prow+x+1;
ix[q]=ix[u]+sx;
iy[q]=iy[u]+sy;
ix2[q]=ix2[u]+sxx;
iy2[q]=iy2[u]+syy;
ixy[q]=ixy[u]+sxy;
}
}
auto rect=[&](const VC<DB>&I,int x0,int y0,int x1,int y1){
return I[(size_t)y1*S+x1]-I[(size_t)y0*S+x1]-I[(size_t)y1*S+x0]+I[(size_t)y0*S+x0];
}
;
DB total=0;
long long cc=0;
for(int gy=center.y0;
gy<=center.y1;
++gy)for(int gx=center.x0;
gx<=center.x1;
++gx){
const _d&a=refFull[gy*R+gx];
const _d&b=_k3[(gy-sample.y0)*w+(gx-sample.x0)];
if(!a.fg&&!b.fg)continue;
int lx=gx-sample.x0,ly=gy-sample.y0,x0=lx-5,y0=ly-5,x1=lx+6,y1=ly+6;
total+=_q1(rect(ix,x0,y0,x1,y1),rect(iy,x0,y0,x1,y1),rect(ix2,x0,y0,x1,y1),rect(iy2,x0,y0,x1,y1),rect(ixy,x0,y0,x1,y1));
++cc;
}
sums[ch]=total;
if(ch==0)count=cc;
}
}
void _e3(int a,int b,VC<A1>&out)const{
out.clear();
Quadric q=vquad[a];
q+=vquad[b];
V3 pos[5];
int np=0;
_k1(a,b,q,pos,np);
for(int i=0;
i<np;
++i){
A1 _J;
for(int dir=0;
dir<2;
++dir){
int ab=dir?b:a,kp=dir?a:b;
DB mr;
if(!_l3(ab,kp,pos[i],mr))continue;
A1 c=_n2(ab,kp,pos[i],q);
c._E=mr;
if(!_r1(c))continue;
if(!_J.valid()||mr<_J._E||(mr==_J._E&&ZN[kp].size()>ZN[_J.kept].size()))_J=c;
}
if(_J.valid())out.push_back(_J);
}
sort(out.begin(),out.end(),[](const A1&x,const A1&y){
return x.cost<y.cost;
}
);
}
DB _f3(const A1&c)const{
DB value=0.;
for(int x:ZN[c._o])if(x!=c.kept&&!ZD[x])value+=1./(1.+ZN[x].size());
for(int x:ZN[c.kept])if(x!=c._o&&!ZD[x])value+=1./(1.+ZN[x].size());
return value;
}
bool _a3(const A1&c,const VC<VC<_d>>&ref,const _p2&base,_B1&out)const{
array<_m,6>_x3,centers,samples;
if(!_q3(c,_x3,centers,samples))return false;
const int R=_b4?_b4:B5,S=R+1;
DB sums[6][4];
long long counts[6];
for(int v=0;
v<6;
++v){
for(int ch=0;
ch<4;
++ch)sums[v][ch]=base.view[v].sum[ch];
counts[v]=base.view[v].count;
}
for(int v=0;
v<6;
++v){
if(!centers[v].valid())continue;
VC<_d>cand;
_V1(c,v,samples[v],cand);
DB _e5[4]={
0,0,0,0}
;
long long _d5=0;
_I2(ref[v],cand,samples[v],centers[v],R,_e5,_d5);
long long _u5=_r2(base.view[v]._P2,S,centers[v]);
out._J3[v]=_d5-_u5;
counts[v]+=out._J3[v];
for(int ch=0;
ch<4;
++ch){
DB _v5=_r2(base.view[v]._b1[ch],S,centers[v]);
out._y4[v][ch]=_e5[ch]-_v5;
sums[v][ch]+=out._y4[v][ch];
}
}
_l sc=_b2(sums,counts);
out.c=c;
out.centers=centers;
out._D2=sc._f;
out._W2=sc.minView;
out.damage=base.score._f-sc._f;
out._t2=_f3(c);
return isfinite(out._D2);
}
_l _K3(const VC<VC<_d>>&ref,const VC<VC<_d>>&cur,int R)const{
_p2 L=_V2(ref,cur,R);
return L.score;
}
int _g(int fc=0,int ov=0,int rr=0){
if(!_L1||_U.empty()||_T.empty())return 0;
int tier=IV<=25000?2:(IV<=45000?3:4);
if(!tier)return 0;
DB left=A0-ET();
if(tier>2&&left<2.3)return 0;
DB stop=ET()+min(tier==2?2.20:5.00,left*(tier==2?0.40:0.70));
MidSnap safe=_K();
const int R=rr?rr:B5;
VC<VC<_d>>ref(6),cur(6);
VC<V3>activeV;
VC<FC>activeF;
A3(activeV,activeF);
if(tier>=3)_o3(R);
else{
_d3 .clear();
_b4=0;
}
for(int v=0;
v<6;
++v){
_Q1(_U,_T,v,R,ref[v]);
_Q1(activeV,activeF,v,R,cur[v]);
if(tier>2&&ET()>=stop){
_i(safe);
return 0;
}
}
_p2 base=_V2(ref,cur,R);
DB budget=tier==2?_Y1:_Z1;
struct Seed{
int a,b;
DB cheap,visual;
}
;
VC<Seed>all;
all.reserve(4096);
int breadth=tier>=3?3:1,scanCap=14000*breadth;
int scanned=0;
for(int a=0;
a<(int)ZV.size()&&scanned<scanCap&&(tier==2||ET()<stop);
++a)if(!ZD[a])for(int b:ZN[a]){
if(b<=a||ZD[b]||scanned>=scanCap)continue;
++scanned;
if(_A(a,b)!=2||B7(a,b)!=2)continue;
DB cc=_O2(a,b);
if(!(cc<=costCap))continue;
DB vis=0;
for(int fi:ZI[a])if(!FD[fi])vis+=1.+(fi<(int)faceSil.size()?18.*faceSil[fi]+2.*faceWin[fi]+0.05*facePix[fi]:0.);
for(int fi:ZI[b])if(!FD[fi])vis+=1.+(fi<(int)faceSil.size()?18.*faceSil[fi]+2.*faceWin[fi]+0.05*facePix[fi]:0.);
all.push_back({
a,b,cc,vis}
);
}
if(all.empty()||(tier>2&&ET()>=stop)){
_i(safe);
return 0;
}
VC<Seed>cheap=all,visual=all;
sort(cheap.begin(),cheap.end(),[](const Seed&x,const Seed&y){
return x.cheap<y.cheap;
}
);
sort(visual.begin(),visual.end(),[](const Seed&x,const Seed&y){
return x.visual<y.visual||(x.visual==y.visual&&x.cheap<y.cheap);
}
);
VC<Seed>seeds;
set<pair<int,int>>seen;
auto _Z4=[&](const VC<Seed>&src,int cap){
for(int i=0;
i<(int)src.size()&&i<cap;
++i)if(seen.insert({
src[i].a,src[i].b}
).second)seeds.push_back(src[i]);
}
;
_Z4(cheap,_q2*breadth);
_Z4(visual,_g2*breadth);
VC<_B1>evals;
evals.reserve(B2*breadth);
int _a5=0,_h3=0;
for(const Seed&s:seeds){
if(_a5>=_t1*breadth||_h3>=B2*breadth||(tier>2&&ET()>=stop))break;
if(!_n1(s.a,s.b))continue;
VC<A1>props;
_e3(s.a,s.b,props);
if(props.empty())continue;
++_a5;
for(const auto&c:props){
if(_h3>=B2*breadth||(tier>2&&ET()>=stop))break;
++_h3;
_B1 e;
if(_a3(c,ref,base,e)&&e._D2>=base.score._f-budget&&e._W2>=base.score.minView-_q)evals.push_back(e);
}
}
if(evals.empty()){
_i(safe);
return 0;
}
sort(evals.begin(),evals.end(),[](const _B1&a,const _B1&b){
if(a.damage!=b.damage)return a.damage<b.damage;
if(a._t2!=b._t2)return a._t2>b._t2;
return a.c.cost<b.c.cost;
}
);
VC<char>lock(ZV.size(),0);
VC<array<_m,6>>_a4;
DB _I4[6][4];
long long _T3[6];
for(int v=0;
v<6;
++v){
for(int ch=0;
ch<4;
++ch)_I4[v][ch]=base.view[v].sum[ch];
_T3[v]=base.view[v].count;
}
int cap=fc?fc:(tier==2?_G1:_H1)*breadth,done=0;
for(const auto&e:evals){
if(done>=cap||(IV>25000&&ET()>=stop))break;
int a=e.c._o,b=e.c.kept;
if(!_n1(a,b)||lock[a]||lock[b])continue;
bool _P3=false;
for(int x:ZN[a])if(lock[x]){
_P3=true;
break;
}
for(int x:ZN[b])if(lock[x]){
_P3=true;
break;
}
if(_P3)continue;
bool overlap=false;
for(const auto&r:_a4){
int hit=0;
for(int v=0;
v<6;
++v)if(_W3(e.centers[v],r[v]))++hit;
if(hit>ov){
overlap=true;
break;
}
if(overlap)break;
}
if(overlap)continue;
DB _k4[6][4];
long long _s3[6];
for(int v=0;
v<6;
++v){
_s3[v]=_T3[v]+e._J3[v];
for(int ch=0;
ch<4;
++ch)_k4[v][ch]=_I4[v][ch]+e._y4[v][ch];
}
_l trial=_b2(_k4,_s3);
if(trial._f<base.score._f-budget||trial.minView<base.score.minView-_q)continue;
_D1(a,b,e.c._t,e.c._E);
++_j;
++done;
for(int v=0;
v<6;
++v){
_T3[v]=_s3[v];
for(int ch=0;
ch<4;
++ch)_I4[v][ch]=_k4[v][ch];
}
lock[a]=lock[b]=1;
for(int x:ZN[b])if(x>=0&&x<(int)lock.size())lock[x]=1;
_a4 .push_back(e.centers);
}
if(done==0){
_i(safe);
return 0;
}
A3(activeV,activeF);
VC<VC<_d>>audit(6);
for(int v=0;
v<6;
++v)_Q1(activeV,activeF,v,R,audit[v]);
_l audited=_K3(ref,audit,R);
if(audited._f<base.score._f-budget||audited.minView<base.score.minView-_q){
_i(safe);
return 0;
}
return done;
}
DB _Y(int v,int root,const _c&sp)const{
VC<VegaTri>oldTris,newTris;
if(!_Z2(v,root,sp,oldTris,newTris))return-1.;
DB dev=_d1(oldTris,newTris);
if(!(dev<=hausd*_S3()))return-1.;
const int R=B8;
DB total=0.;
int _m4=0;
VC<_d>a,b;
for(int view=0;
view<6;
++view){
DB mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;
bool any=false;
auto _w4=[&](const VegaTri&t){
for(int k=0;
k<3;
++k){
_P1 p=A6(t.p[k],view,R);
if(!p.ok)continue;
any=true;
mnU=min(mnU,p.u);
mxU=max(mxU,p.u);
mnV=min(mnV,p.v);
mxV=max(mxV,p.v);
}
}
;
for(const VegaTri&t:oldTris)_w4(t);
for(const VegaTri&t:newTris)_w4(t);
if(!any)continue;
int pad=_a2;
int x0=max(0,(int)floor(mnU)-pad);
int y0=max(0,(int)floor(mnV)-pad);
int x1=min(R-1,(int)ceil(mxU)+pad);
int y1=min(R-1,(int)ceil(mxV)+pad);
if(x0>x1||y0>y1)continue;
int w=x1-x0+1,h=y1-y0+1;
if(w*h>_C2)return-1.;
_l1(oldTris,view,x0,y0,w,h,a);
_l1(newTris,view,x0,y0,w,h,b);
total+=_u2(a,b);
++_m4;
}
if(_m4==0)return 1.;
return total/DB(_m4);
}
void _D(){
_c sp=_u4();
if(sp._x1<=0)return;
DB _c1=A0-ET();
if(_c1<0.45)return;
DB _G=ET()+min(sp._K2,_c1*sp._F3);
int _y=min(sp.hardCap,max(0,(int)floor(nV*sp._g3)));
if(_y<=0)return;
DB minS=_Y4();
DB _O4=_H4();
VC<_k2>cands;
cands.reserve(min(_g1,_y*10+256));
int scanned=0,visited=0,total=(int)ZV.size();
int start=(total>0)?(_O1%total):0;
for(;
visited<total&&scanned<sp._u&&ET()<_G;
++visited){
int v=(start+visited)%total;
if(ZD[v])continue;
++scanned;
A4 geom=A5(v,sp);
if(!geom.valid())continue;
DB s=_Y(geom.v,geom.root,sp);
if(s<0.)continue;
DB damage=1.-s;
if(s<minS||damage>_O4)continue;
_k2 vc;
vc.v=geom.v;
vc.root=geom.root;
vc.ssim=s;
vc.score=damage+_w2*geom.score;
cands.push_back(vc);
if((int)cands.size()>=_g1)break;
}
if(total>0)_O1=(start+max(1,visited))%total;
if(cands.empty())return;
sort(cands.begin(),cands.end());
int extra=0;
for(const _k2&c:cands){
if(extra>=_y||ET()>=_G)break;
if(c.v<0||c.v>=(int)ZD.size()||ZD[c.v])continue;
DB s=_Y(c.v,c.root,sp);
if(s<minS||1.-s>_O4)continue;
if(_p(c.v,c.root,sp)){
++_j;
++extra;
}
}
}
void _Z(){
_c sp=_M2();
DB _c1=A0-ET();
if(IV>25000&&_c1<0.35)return;
DB _G=ET()+min(sp._K2,_c1*sp._F3);
int _y=min(sp.hardCap,max(0,(int)floor(nV*sp._g3)));
if(_y<=0)return;
int extra=0;
for(int round=0;
round<sp.rounds&&extra<_y&&(IV<=25000||ET()<_G);
++round){
VC<A4>cands;
cands.reserve(4096);
int scanned=0,visited=0,total=(int)ZV.size();
int start=(total>0)?(_o1%total):0;
for(;
visited<total&&scanned<sp._u&&(IV<=25000||ET()<_G);
++visited){
int v=(start+visited)%total;
if(ZD[v])continue;
++scanned;
A4 c=_e2(v);
if(c.valid())cands.push_back(c);
}
if(total>0)_o1=(start+max(1,visited))%total;
if(cands.empty())break;
sort(cands.begin(),cands.end());
bool _E3=false;
for(const A4&c:cands){
if(extra>=_y||(IV>25000&&ET()>=_G))break;
if(ZD[c.v])continue;
if(_Z3(c.v)){
++_j;
++extra;
_E3=true;
}
}
if(!_E3)break;
}
}
bool _I3(const _l&safe,const _l&cand)const{
return cand._f>=max(0.9105,safe._f-0.0085)&&cand.minView>=max(0.8750,safe.minView-0.0090)&&cand._w>=max(0.6200,safe._w-0.0150)&&cand._R>=max(0.9550,safe._R-0.0016);
}
void _S2(const VC<V3>&safeV,const VC<FC>&safeF,const VC<DB>&safeR,DB oldDiag,DB _F4){
ZV=safeV;
ZF=safeF;
nV=(int)ZV.size();
nF=(int)ZF.size();
ZN.assign(nV,_X3());
_M1();
if(_frag){
vmoment=vquad;
for(auto&q:vmoment)q.scale(.045);
}
if(safeR.size()==ZV.size())crad=safeR;
else crad.assign(nV,0.);
diag=oldDiag;
hausd=_F4;
_D4=diag>A2?1./(diag*diag):0.;
costCap=_J1*diag*diag;
PQ<A1>empty;
pq.swap(empty);
_j=0;
}
void _p3(DB ratio){
_a=2;
int R=_frag?512:1024;
_h1(R);
_v();
targetV=max(10,(int)floor(IV*ratio));
targetV=min(targetV,nV);
B1=max(0,nV-targetV);
_j=0;
PQ<A1>empty;
pq.swap(empty);
_S=A0-0.72;
_F1=true;
_s2();
_i1();
_F1=false;
_Z();
compact();
}
__attribute__((noinline,cold,section(".text.unlikely.medium")))void _w3(){
if(!(IV>5000&&IV<=25000)||_U.empty()||_T.empty())return;
VC<V3>preV=ZV,bestV=ZV,rootV;
VC<FC>preF=ZF,bestF=ZF,rootF;
VC<DB>preR=_e,bestR=_e,rootR;
if(preR.size()!=preV.size())preR.assign(preV.size(),0);
bestR=preR;
DB od=diag,hd=hausd;
VC<_u1>r(6),r5;
for(int v=0;
v<6;
++v)r[v]=_x(_U,_T,v,1024);
_l pre=A7(r,preV,preF,1024),pre5,root,root5;
bool ex=pre._f>=.94&&pre.minView>=.92&&pre._w>=.84;
if(ex){
r5 .resize(6);
for(int v=0;
v<6;
++v)r5[v]=_x(_U,_T,v,512);
pre5=A7(r5,preV,preF,512);
}
_U.clear();
_T.clear();
DB hi=DB(preV.size())/IV,lo=.16;
bool got=0,deep=0;
int pass=0,n=ex?11:5;
for(int it=0;
it<n;
++it){
DB x=.5*(lo+hi);
_frag=deep;
if(deep)_S2(rootV,rootF,rootR,od,hd);
else _S2(preV,preF,preR,od,hd);
_p3(x);
if(ZV.size()>=bestV.size()){
lo=x;
continue;
}
_l p5,base5;
if(ex){
p5=A7(r5,ZV,ZF,512);
base5=deep?root5:pre5;
DB f=deep?.009:.011,v=deep?.011:.012,nm=deep?.017:.019,dd=deep?.003:.0028;
if(p5 ._f<base5 ._f-f||p5 .minView<base5 .minView-v||p5 ._w<base5 ._w-nm||p5 ._R<base5 ._R-dd){
lo=x;
continue;
}
}
_l sc=A7(r,ZV,ZF,1024,ex&&!deep&&pass==1);
bool ok=deep?sc._f>=max(.925,root._f-.0065)&&sc.minView>=max(.91,root.minView-.0075)&&sc._w>=max(.78,root._w-.011)&&sc._R>=max(.966,root._R-.0018):_I3(pre,sc);
if(ok){
bestV=ZV;
bestF=ZF;
bestR=_e;
hi=x;
got=1;
if(ex&&!deep&&++pass==2){
rootV=ZV;
rootF=ZF;
rootR=_e;
root=sc;
root5=p5;
deep=1;
lo=max(.075,x*.66);
}
}
else lo=x;
}
if(got){
ZV.swap(bestV);
ZF.swap(bestF);
_e.swap(bestR);
}
else{
ZV.swap(preV);
ZF.swap(preF);
_e.swap(preR);
}
nV=ZV.size();
nF=ZF.size();
}
void _R6(int t,const VC<V3>&ov,const VC<FC>&of){
int R=t==4?512:384;
VC<V3>rv=ov,cv;
VC<FC>rf=of,cf;
VC<_u1>ref(6);
for(int v=0;
v<6;
++v)ref[v]=_x(rv,rf,v,R);
A3(cv,cf);
_l bs=A7(ref,cv,cf,R);
DB od=diag,oh=hausd;
auto qe=[&](int rr,const VC<_u1>&rf0,_l&b0,int da,int db,DB ef,int dg,DB af,DB vf,DB nf,DB df,DB rs){
VC<V3>sv=ZV;
VC<FC>sf=ZF;
VC<DB>sr=_e;
int nv=nV;
_S2(sv,sf,sr,od,oh);
_frag=0;
_a=4;
_h1(rr);
_v();
DB av=0;
int ac=0,fr=0;
for(int i=0;
i<nV;
++i)if(!ZD[i]){
av+=_b[i];
++ac;
}
av/=max(1,ac);
for(int i=0;
i<nV;
++i)if(!ZD[i]&&_b[i]<av*ef&&ZN[i].size()<=dg)++fr;
int cut=min(max(1,fr/da),max(1,nV/db));
targetV=max(10,nV-cut);
B1=nV-targetV;
_j=0;
PQ<A1>e;
pq.swap(e);
DB os=_S;
if(rs)_S=A0-rs;
_F1=true;
_s2();
_i1();
_F1=false;
_S=os;
compact();
if(nV>=nv){
ZV.swap(sv);
ZF.swap(sf);
_e.swap(sr);
nV=ZV.size();
nF=ZF.size();
return false;
}
A3(cv,cf);
_l q=A7(rf0,cv,cf,rr);
bool ok=q._f>=b0 ._f-af&&q.minView>=b0 .minView-vf&&q._w>=b0 ._w-nf&&q._R>=b0 ._R-df;
if(ok)b0=q;
else{
ZV.swap(sv);
ZF.swap(sf);
_e.swap(sr);
nV=ZV.size();
nF=ZF.size();
}
return ok;
}
;
auto pp=[&](DB af,DB vf,DB nf,DB df){
VC<V3>sv=ZV;
VC<FC>sf=ZF;
VC<DB>sr=_e;
_S2(sv,sf,sr,od,oh);
_frag=0;
_a=4;
_h1(R);
_v();
_U1();
compact();
A3(cv,cf);
_l q=A7(ref,cv,cf,R);
bool ok=q._f>=bs._f-af&&q.minView>=bs.minView-vf&&q._w>=bs._w-nf&&q._R>=bs._R-df;
if(ok)bs=q;
else{
ZV.swap(sv);
ZF.swap(sf);
_e.swap(sr);
nV=ZV.size();
nF=ZF.size();
}
return ok;
}
;
if(t==4){
qe(R,ref,bs,42,280,.50,9,.00022,.00030,.00052,.00010,0);
if(ET()>A0-4.0)return;
VC<_u1>rh(6);
for(int v=0;
v<6;
++v)rh[v]=_x(rv,rf,v,1024);
A3(cv,cf);
_l bh=A7(rh,cv,cf,1024);
qe(1024,rh,bh,64,420,.42,8,.00012,.00018,.00030,.00006,1.15);
if(ET()>A0-2.35)return;
VC<V3>sv=ZV;
VC<FC>sf=ZF;
VC<DB>sr=_e;
_S2(sv,sf,sr,od,oh);
int gd=_g(4,2,1024);
compact();
if(gd){
A3(cv,cf);
_l q=A7(rh,cv,cf,1024);
if(q._f>=bh._f-.00010&&q.minView>=bh.minView-.00016&&q._w>=bh._w-.00024&&q._R>=bh._R-.00005)bh=q;
else{
ZV.swap(sv);
ZF.swap(sf);
_e.swap(sr);
nV=ZV.size();
nF=ZF.size();
}
}
return;
}
qe(R,ref,bs,24,190,.50,9,.00045,.00065,.0010,.00020,0);
pp(.00045,.00065,.0010,.00020);
qe(R,ref,bs,34,250,.50,9,.00038,.00055,.00075,.00016,0);
pp(.00034,.00048,.00068,.00014);
for(int gi=0;
gi<2;
++gi){
VC<V3>sv=ZV;
VC<FC>sf=ZF;
VC<DB>sr=_e;
_S2(sv,sf,sr,od,oh);
int gd=_g(4);
compact();
if(!gd)break;
A3(cv,cf);
_l q=A7(ref,cv,cf,R);
DB af=gi?.00024:.00016,vf=gi?.00034:.00024,nf=gi?.00046:.00032,df=gi?.00010:.00007;
if(q._f>=bs._f-af&&q.minView>=bs.minView-vf&&q._w>=bs._w-nf&&q._R>=bs._R-df)bs=q;
else{
ZV.swap(sv);
ZF.swap(sf);
_e.swap(sr);
nV=ZV.size();
nF=ZF.size();
break;
}
}
}
bool _r3(const VC<V3>&vv,const VC<FC>&ff)const{
if(vv.empty()||ff.empty())return false;
struct EC{
int count=0,dir=0;
}
;
UM<unsigned long long,EC>em;
em.reserve(ff.size()*4);
VC<VC<pair<int,int>>>links(vv.size());
auto key=[](int a,int b){
if(a>b)swap(a,b);
return(unsigned long long)(unsigned int)a<<32|(unsigned int)b;
}
;
for(const FC&f:ff){
int a=f.v[0],b=f.v[1],c=f.v[2];
if(a<0||b<0||c<0||a>=(int)vv.size()||b>=(int)vv.size()||c>=(int)vv.size()||a==b||b==c||a==c)return false;
if(norm2(cross(vv[b]-vv[a],vv[c]-vv[a]))<1e-24*max(1.,diag*diag*diag*diag))return false;
int x[3]={
a,b,c}
;
for(int k=0;
k<3;
++k){
int u=x[k],v=x[(k+1)%3],lo=min(u,v);
auto&e=em[key(u,v)];
e.count++;
e.dir+=(u==lo?1:-1);
}
links[a].push_back({
b,c}
);
links[b].push_back({
c,a}
);
links[c].push_back({
a,b}
);
}
for(auto&kv:em)if(kv.second.count!=2||kv.second.dir!=0)return false;
for(int v=0;
v<(int)links.size();
++v){
if(links[v].empty())continue;
UM<int,VC<int>>g;
g.reserve(links[v].size()*2);
for(auto p:links[v]){
g[p.first].push_back(p.second);
g[p.second].push_back(p.first);
}
for(auto&kv:g)if(kv.second.size()!=2)return false;
US<int>seen;
VC<int>st{
g.begin()->first}
;
while(!st.empty()){
int x=st.back();
st.pop_back();
if(!seen.insert(x).second)continue;
for(int y:g[x])st.push_back(y);
}
if(seen.size()!=g.size())return false;
}
return true;
}
__attribute__((noinline,cold))void _C3(const VC<FC>&ff,int nv,VC<VC<int>>&vf,VC<VC<int>>&fa)const{
vf.assign(nv,{
}
);
fa.assign(ff.size(),{
}
);
UM<unsigned long long,pair<int,int>>first;
first.reserve(ff.size()*4);
auto key=[](int a,int b){
if(a>b)swap(a,b);
return(unsigned long long)(unsigned int)a<<32|(unsigned int)b;
}
;
for(int fi=0;
fi<(int)ff.size();
++fi){
const FC&f=ff[fi];
for(int k=0;
k<3;
++k)vf[f.v[k]].push_back(fi);
for(int k=0;
k<3;
++k){
int a=f.v[k],b=f.v[(k+1)%3];
auto K=key(a,b);
auto it=first.find(K);
if(it==first.end())first[K]={
fi,k}
;
else{
int fj=it->second.first;
fa[fi].push_back(fj);
fa[fj].push_back(fi);
}
}
}
}
struct _H3{
VC<int>fs,ring,anchors;
VC<FC>tris;
}
;
bool _L(int a,int b,int c,const V3&nn,FC&f)const{
if(a==b||b==c||a==c)return false;
f.v[0]=a;
f.v[1]=b;
f.v[2]=c;
V3 n=cross(ZV[b]-ZV[a],ZV[c]-ZV[a]);
if(norm(n)<1e-12*max(1.,diag*diag))return false;
if(dot(n,nn)<0)swap(f.v[1],f.v[2]);
return true;
}
DB _D7(const V3&p,const FC&f)const{
const V3&a=ZV[f.v[0]],&b=ZV[f.v[1]],&c=ZV[f.v[2]];
V3 ab=b-a,ac=c-a,ap=p-a;
DB d1=dot(ab,ap),d2=dot(ac,ap);
if(d1<=0&&d2<=0)return norm2(ap);
V3 bp=p-b;
DB d3=dot(ab,bp),d4=dot(ac,bp);
if(d3>=0&&d4<=d3)return norm2(bp);
DB vc=d1*d4-d3*d2;
if(vc<=0&&d1>=0&&d3<=0){
DB v=d1/(d1-d3);
return norm2(p-(a+ab*v));
}
V3 cp=p-c;
DB d5=dot(ab,cp),d6=dot(ac,cp);
if(d6>=0&&d5<=d6)return norm2(cp);
DB vb=d5*d2-d1*d6;
if(vb<=0&&d2>=0&&d6<=0){
DB w=d2/(d2-d6);
return norm2(p-(a+ac*w));
}
DB va=d3*d6-d5*d4;
if(va<=0&&d4>=d3&&d5>=d6){
DB w=(d4-d3)/((d4-d3)+(d5-d6));
return norm2(p-(b+(c-b)*w));
}
DB z=1./(va+vb+vc),v=vb*z,w=vc*z;
return norm2(p-(a+ab*v+ac*w));
}
bool _L2(const VC<int>&ring,const VC<int>&anchors,const V3&nn,VC<FC>&out)const{
out.clear();
int m=(int)ring.size();
if(m<3)return false;
V3 ax=fabs(nn.x)<.8?V3(1,0,0):V3(0,1,0),u=cross(ax,nn);
DB ul=norm(u);
if(ul<1e-20)return false;
u=u/ul;
V3 v=cross(nn,u);
UM<int,pair<DB,DB>>p;
p.reserve(ring.size()+anchors.size());
auto pp=[&](int x){
auto it=p.find(x);
if(it!=p.end())return it->second;
pair<DB,DB>z={
dot(ZV[x],u),dot(ZV[x],v)}
;
p[x]=z;
return z;
}
;
auto ori=[&](int a,int b,int c){
auto A=pp(a),B=pp(b),C=pp(c);
return(B.first-A.first)*(C.second-A.second)-(B.second-A.second)*(C.first-A.first);
}
;
DB ar=0;
for(int i=0;
i<m;
++i){
auto A=pp(ring[i]),B=pp(ring[(i+1)%m]);
ar+=A.first*B.second-B.first*A.second;
}
if(fabs(ar)<1e-22*max(1.,diag*diag))return false;
DB sg=ar>0?1.:-1.,eps=1e-10*max(1.,diag*diag);
VC<int>w=ring;
int guard=0;
while(w.size()>3&&guard++<m*m){
int bj=-1;
DB bs=_k;
FC bf;
for(int j=0;
j<(int)w.size();
++j){
int a=w[(j+(int)w.size()-1)%w.size()],b=w[j],c=w[(j+1)%w.size()];
if(sg*ori(a,b,c)<=eps)continue;
bool hit=false;
for(int q:w)if(q!=a&&q!=b&&q!=c){
DB A=sg*ori(a,b,q),B=sg*ori(b,c,q),C=sg*ori(c,a,q);
if(A>=-eps&&B>=-eps&&C>=-eps){
hit=true;
break;
}
}
if(hit)continue;
FC f;
if(!_L(a,b,c,nn,f))continue;
V3 tn=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB tl=norm(tn);
if(tl<=0)continue;
DB sc=1.-dot(tn,nn)/tl+1e-6*norm2(ZV[a]-ZV[c])/(diag*diag+1e-30);
if(sc<bs){
bs=sc;
bj=j;
bf=f;
}
}
if(bj<0)return false;
out.push_back(bf);
w.erase(w.begin()+bj);
}
if(w.size()!=3)return false;
FC lf;
if(!_L(w[0],w[1],w[2],nn,lf))return false;
out.push_back(lf);
for(int x:anchors){
bool _K4=false;
for(auto&f:out)if(f.v[0]==x||f.v[1]==x||f.v[2]==x){
_K4=true;
break;
}
if(_K4)continue;
int ti=-1,ea=-1,eb=-1;
for(int i=0;
i<(int)out.size();
++i){
FC f=out[i];
DB T=ori(f.v[0],f.v[1],f.v[2]);
DB s=T>=0?1.:-1.,A=s*ori(f.v[0],f.v[1],x),B=s*ori(f.v[1],f.v[2],x),C=s*ori(f.v[2],f.v[0],x);
if(A>=-eps&&B>=-eps&&C>=-eps){
ti=i;
if(fabs(A)<=eps){
ea=f.v[0];
eb=f.v[1];
}
else if(fabs(B)<=eps){
ea=f.v[1];
eb=f.v[2];
}
else if(fabs(C)<=eps){
ea=f.v[2];
eb=f.v[0];
}
break;
}
}
if(ti<0)return false;
if(ea<0){
FC old=out[ti],f0,f1,f2;
if(!_L(old.v[0],old.v[1],x,nn,f0)||!_L(old.v[1],old.v[2],x,nn,f1)||!_L(old.v[2],old.v[0],x,nn,f2))return false;
out[ti]=f0;
out.push_back(f1);
out.push_back(f2);
}
else{
VC<int>hit;
for(int i=0;
i<(int)out.size();
++i){
FC f=out[i];
bool h=false;
for(int k=0;
k<3;
++k){
int a=f.v[k],b=f.v[(k+1)%3];
if((a==ea&&b==eb)||(a==eb&&b==ea)){
h=true;
break;
}
}
if(h)hit.push_back(i);
}
if(hit.empty()||hit.size()>2)return false;
sort(hit.rbegin(),hit.rend());
VC<FC>add;
for(int i:hit){
FC old=out[i];
int c=-1;
for(int k=0;
k<3;
++k)if(old.v[k]!=ea&&old.v[k]!=eb)c=old.v[k];
FC f0,f1;
if(c<0||!_L(ea,x,c,nn,f0)||!_L(x,eb,c,nn,f1))return false;
out.erase(out.begin()+i);
add.push_back(f0);
add.push_back(f1);
}
out.insert(out.end(),add.begin(),add.end());
}
}
return true;
}
int _Q2(int _t5,int _U4,DB _p5){
if(nV<20||nF<20)return 0;
VC<V3>oldV=ZV;
VC<FC>oldF=ZF;
VC<DB>oldR=_e;
VC<VC<int>>vf,fa;
_C3(ZF,nV,vf,fa);
VC<V3>fn(nF);
VC<DB>pd(nF),wa(nF);
VC<char>good(nF,0);
for(int fi=0;
fi<nF;
++fi){
FC&f=ZF[fi];
V3 r=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB l=norm(r);
if(l>1e-18*max(1.,diag*diag)){
fn[fi]=r/l;
pd[fi]=-dot(fn[fi],ZV[f.v[0]]);
wa[fi]=l;
good[fi]=1;
}
}
DB nt=IV<=25000?3.75e-4:(IV<=45000?7.5e-5:1e-10),pt=IV<=25000?.10*hausd:(IV<=45000?.03*hausd:max(1e-13*max(1.,diag),1e-10*hausd));
VC<int>lab(nF,-1);
VC<VC<int>>cs;
for(int s=0;
s<nF;
++s)if(good[s]&&lab[s]<0){
int id=(int)cs.size();
cs.push_back({
}
);
VC<int>q{
s}
;
lab[s]=id;
for(size_t h=0;
h<q.size();
++h){
int f=q[h];
cs[id].push_back(f);
for(int g:fa[f])if(good[g]&&lab[g]<0&&dot(fn[s],fn[g])>=1.-nt&&fabs(pd[s]-pd[g])<=pt){
lab[g]=id;
q.push_back(g);
}
}
}
VC<_H3>reps;
VC<char>rmF(nF,0);
VC<DB>rad(nV,0.);
if(_e.size()==(size_t)nV)rad=_e;
auto ek=[](int a,int b){
if(a>b)swap(a,b);
return(unsigned long long)(unsigned int)a<<32|(unsigned int)b;
}
;
for(auto&fs:cs){
if((int)fs.size()<_t5)continue;
V3 an,cen;
DB sw=0;
US<int>pv;
UM<unsigned long long,pair<int,pair<int,int>>>em;
pv.reserve(fs.size()*2);
em.reserve(fs.size()*4);
for(int fi:fs){
an=an+fn[fi]*wa[fi];
FC&f=ZF[fi];
cen=cen+(ZV[f.v[0]]+ZV[f.v[1]]+ZV[f.v[2]])*(wa[fi]/3.);
sw+=wa[fi];
for(int k=0;
k<3;
++k)pv.insert(f.v[k]);
for(int k=0;
k<3;
++k){
int a=f.v[k],b=f.v[(k+1)%3];
auto K=ek(a,b);
auto it=em.find(K);
if(it==em.end())em[K]={
1,{
a,b}
}
;
else it->second.first++;
}
}
DB al=norm(an);
if(al<1e-20||sw<=0)continue;
an=an/al;
cen=cen/sw;
DB md=0,mp=0;
for(int fi:fs){
md=max(md,1.-dot(fn[fi],an));
FC&f=ZF[fi];
for(int k=0;
k<3;
++k)mp=max(mp,fabs(dot(ZV[f.v[k]]-cen,an)));
}
if(md>nt||mp>pt)continue;
UM<int,int>nx,in;
US<int>bv;
bool ok=true;
for(auto&kv:em){
if(kv.second.first==1){
int a=kv.second.second.first,b=kv.second.second.second;
if(nx.count(a)||in.count(b)){
ok=false;
break;
}
nx[a]=b;
in[b]=a;
bv.insert(a);
bv.insert(b);
}
else if(kv.second.first!=2){
ok=false;
break;
}
}
if(!ok||bv.size()<3||bv.size()>(size_t)_U4||nx.size()!=bv.size())continue;
int st=*min_element(bv.begin(),bv.end()),cur=st;
VC<int>ring;
for(size_t z=0;
z<bv.size();
++z){
if(!nx.count(cur)){
ok=false;
break;
}
ring.push_back(cur);
cur=nx[cur];
if(cur==st){
if(z+1!=bv.size())ok=false;
break;
}
}
if(!ok||cur!=st||ring.size()!=bv.size())continue;
if((int)pv.size()-(int)em.size()+(int)fs.size()!=1)continue;
VC<int>inside;
for(int x:pv)if(!bv.count(x))inside.push_back(x);
if(inside.empty())continue;
VC<char>flag(nF,0);
for(int f:fs)flag[f]=1;
for(int x:inside)for(int f:vf[x])if(!flag[f])ok=false;
if(!ok)continue;
sort(inside.begin(),inside.end(),[&](int a,int b){
if(rad[a]!=rad[b])return rad[a]>rad[b];
return a<b;
}
);
VC<int>anchors;
DB cap=_p5*hausd;
VC<FC>tr;
if(!_L2(ring,anchors,an,tr))continue;
US<int>held;
bool covered=false,failed=false;
for(size_t pass=0;pass<=inside.size();++pass){
VC<pair<DB,int>>miss;
for(int x:inside)if(!held.count(x)){
DB d2=_k;
for(const FC&f:tr)d2=min(d2,_D7(ZV[x],f));
DB err=rad[x]+sqrt(max(0.,d2));
if(err>cap)miss.push_back({err-cap,x});
}
if(miss.empty()){
covered=true;
break;
}
sort(miss.begin(),miss.end(),[](const pair<DB,int>&a,const pair<DB,int>&b){
if(a.first!=b.first)return a.first>b.first;
return a.second<b.second;
});
for(auto z:miss){
anchors.push_back(z.second);
held.insert(z.second);
}
if(anchors.size()>=inside.size())break;
if(!_L2(ring,anchors,an,tr)){
reverse(anchors.begin(),anchors.end());
if(!_L2(ring,anchors,an,tr)){
failed=true;
break;
}
}
}
if(failed||!covered||anchors.size()>=inside.size())continue;
for(int f:fs)rmF[f]=1;
reps.push_back({
fs,ring,anchors,tr}
);
}
if(reps.empty())return 0;
VC<FC>nf;
nf.reserve(nF);
for(int fi=0;
fi<nF;
++fi)if(!rmF[fi])nf.push_back(ZF[fi]);
for(auto&r:reps)nf.insert(nf.end(),r.tris.begin(),r.tris.end());
VC<char>used(nV,0);
for(auto&f:nf)for(int k=0;
k<3;
++k)used[f.v[k]]=1;
VC<int>mp(nV,-1);
VC<V3>nv;
VC<DB>nr;
nv.reserve(nV);
nr.reserve(nV);
for(int i=0;
i<nV;
++i)if(used[i]){
mp[i]=(int)nv.size();
nv.push_back(ZV[i]);
nr.push_back(rad[i]);
}
set<array<int,3>>keys;
for(auto&f:nf){
for(int k=0;
k<3;
++k)f.v[k]=mp[f.v[k]];
array<int,3>k={
f.v[0],f.v[1],f.v[2]}
;
sort(k.begin(),k.end());
if(k[0]<0||k[0]==k[1]||k[1]==k[2]||!keys.insert(k).second){
ZV.swap(oldV);
ZF.swap(oldF);
_e.swap(oldR);
nV=(int)ZV.size();
nF=(int)ZF.size();
return 0;
}
}
if(nv.size()>=oldV.size()||!_r3(nv,nf)){
ZV.swap(oldV);
ZF.swap(oldF);
_e.swap(oldR);
nV=(int)ZV.size();
nF=(int)ZF.size();
return 0;
}
int gain=(int)oldV.size()-(int)nv.size();
ZV.swap(nv);
ZF.swap(nf);
_e.swap(nr);
nV=(int)ZV.size();
nF=(int)ZF.size();
return gain;
}
DB _P6(int kept,int removed,const VC<VC<int>>&vertexFaces)const{
VC<int>affectedFaces=vertexFaces[kept];
affectedFaces.insert(affectedFaces.end(),vertexFaces[removed].begin(),vertexFaces[removed].end());
sort(affectedFaces.begin(),affectedFaces.end());
affectedFaces.erase(unique(affectedFaces.begin(),affectedFaces.end()),affectedFaces.end());
VC<VegaTri>beforeTriangles;
VC<VegaTri>afterTriangles;
beforeTriangles.reserve(ZF.size());
afterTriangles.reserve(ZF.size());
for(auto rewritten:ZF){
VegaTri triangle;
for(int k=0;
k<3;
++k)triangle.p[k]=ZV[rewritten.v[k]];
beforeTriangles.push_back(triangle);
for(int k=0;
k<3;
++k)if(rewritten.v[k]==removed)rewritten.v[k]=kept;
if(rewritten.v[0]==rewritten.v[1]||rewritten.v[1]==rewritten.v[2]||rewritten.v[2]==rewritten.v[0])continue;
for(int k=0;
k<3;
++k)triangle.p[k]=ZV[rewritten.v[k]];
afterTriangles.push_back(triangle);
}
constexpr int resolution=1024;
DB totalLoss=0.0;
int evaluatedViews=0;
VC<_d>beforeBuffer,afterBuffer;
for(int view=0;
view<6;
++view){
DB minX=resolution,minY=resolution,maxX=0,maxY=0;
bool anyProjection=false;
for(int faceIndex:affectedFaces){
FC face=ZF[faceIndex];
for(int after=0;
after<2;
++after){
for(int k=0;
k<3;
++k){
int vertex=face.v[k];
if(after&&vertex==removed)vertex=kept;
auto projected=A6(ZV[vertex],view,resolution);
if(!projected.ok)continue;
anyProjection=true;
minX=min(minX,projected.u);
minY=min(minY,projected.v);
maxX=max(maxX,projected.u);
maxY=max(maxY,projected.v);
}
}
}
if(!anyProjection)continue;
int x0=max(0,(int)floor(minX)-6);
int y0=max(0,(int)floor(minY)-6);
int side=max((int)ceil(maxX)+7-x0,(int)ceil(maxY)+7-y0);
side=min(side,resolution);
x0=min(x0,resolution-side);
y0=min(y0,resolution-side);
if(side<11||side>384)return _k;
_l1(beforeTriangles,view,x0,y0,side,side,beforeBuffer,resolution);
_l1(afterTriangles,view,x0,y0,side,side,afterBuffer,resolution);
int pixelCount=side*side;
VC<float>bnx(pixelCount),anx(pixelCount),bny(pixelCount),any(pixelCount);
VC<float>bnz(pixelCount),anz(pixelCount),bd(pixelCount),ad(pixelCount);
VC<unsigned char>bf(pixelCount),af(pixelCount);
for(int i=0;
i<pixelCount;
++i){
bnx[i]=beforeBuffer[i].n[0];
anx[i]=afterBuffer[i].n[0];
bny[i]=beforeBuffer[i].n[1];
any[i]=afterBuffer[i].n[1];
bnz[i]=beforeBuffer[i].n[2];
anz[i]=afterBuffer[i].n[2];
bd[i]=beforeBuffer[i].d;
ad[i]=afterBuffer[i].d;
bf[i]=beforeBuffer[i].fg;
af[i]=afterBuffer[i].fg;
}
DB normalSsim=(refSsim(bnx,anx,bf,af,side)+refSsim(bny,any,bf,af,side)+refSsim(bnz,anz,bf,af,side))/3.0;
DB depthSsim=refSsim(bd,ad,bf,af,side);
totalLoss+=(1.0-0.5*(normalSsim+depthSsim))*side*side;
evaluatedViews+=side*side;
}
return evaluatedViews?totalLoss/evaluatedViews:_k;
}
__attribute__((noinline,cold,section(".text.unlikely.hiddenedge")))int _P5(const VC<V3>&ov,int maxCandidates=0,DB scoreCap=hparam_T3EndpointWeldScoreCap,bool strategicStrike=false,int strategicProbeCount=16){
if(nV<100)return 0;
VC<V3>sv=ZV;
FD.assign(nF,0);
ZD.assign(nV,0);
_h1(512);
VC<VC<int>>vf,fa;
_C3(ZF,nV,vf,fa);
VC<VC<int>>nb(nV);
for(auto f:ZF)for(int k=0;
k<3;
++k){
int a=f.v[k],b=f.v[(k+1)%3];
nb[a].push_back(b);
nb[b].push_back(a);
}
for(auto&x:nb){
sort(x.begin(),x.end());
x.erase(unique(x.begin(),x.end()),x.end());
}
VC<int>mp(nV);
iota(mp.begin(),mp.end(),0);
VC<tuple<DB,int,int>>ca;
for(int b=0;
b<nV;
++b)for(int a:nb[b]){
int cm=0;
for(int q:nb[b])if(binary_search(nb[a].begin(),nb[a].end(),q))++cm;
if(cm!=2)continue;
DB co=0;
bool ok=true;
for(int fi:vf[b]){
FC f=ZF[fi];
bool ha=false;
for(int k=0;
k<3;
++k)if(f.v[k]==a)ha=true;
if(ha)continue;
V3 n0=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
for(int k=0;
k<3;
++k)if(f.v[k]==b)f.v[k]=a;
V3 n1=cross(ZV[f.v[1]]-ZV[f.v[0]],ZV[f.v[2]]-ZV[f.v[0]]);
DB l0=norm(n0),l1=norm(n1);
if(l0<1e-14||l1<1e-14){
ok=false;
break;
}
co+=(facePix[fi]+4.*faceSil[fi]+0.1)*(1.-dot(n0,n1)/(l0*l1));
}
if(ok){
DB mv=0;
for(int vv=0;
vv<6;
++vv){
auto pa=_n(ZV[a],vv,512),pb=_n(ZV[b],vv,512);
if(pa.ok&&pb.ok)mv=max(mv,hypot(pa.u-pb.u,pa.v-pb.v));
}
ca.push_back({
co+.0020*mv*mv+1e-4*norm2(ZV[a]-ZV[b])/(diag*diag),a,b}
);
}
}
sort(ca.begin(),ca.end());
if(strategicStrike){
int probeCount=min(strategicProbeCount,(int)ca.size());
int bestIndex=-1;
DB bestRank=_k;
for(int i=0;
i<probeCount&&get<0>(ca[i])<=scoreCap;
++i){
int kept=get<1>(ca[i]);
int removed=get<2>(ca[i]);
DB accumulatedDebt=0.0;
int debtSamples=0;
for(int faceIndex:vf[removed]){
for(int k=0;
k<3;
++k){
accumulatedDebt+=_fd[ZF[faceIndex].v[k]];
++debtSamples;
}
}
DB localSsimLoss=_P6(kept,removed,vf);
DB averageDebt=accumulatedDebt/max(1,debtSamples);
DB rank=localSsimLoss-strategicDebtWeight*averageDebt;
if(rank<bestRank){
bestRank=rank;
bestIndex=i;
}
}
if(bestIndex>0)swap(ca[0],ca[bestIndex]);
}
VC<char>lk(nV,0);
int g=0;
int candidateCap=maxCandidates>0?min(maxCandidates,nV/8):nV/8;
for(auto t:ca){
if(get<0>(t)>scoreCap||g>=candidateCap)break;
int a=get<1>(t),b=get<2>(t);
if(lk[a]||lk[b])continue;
mp[b]=a;
lk[a]=lk[b]=1;
for(int q:nb[a])lk[q]=1;
for(int q:nb[b])lk[q]=1;
++g;
}
if(!g)return 0;
VC<FC>nf;
nf.reserve(nF-2*g);
set<array<int,3>>fk;
for(auto f:ZF){
for(int k=0;
k<3;
++k)f.v[k]=mp[f.v[k]];
if(f.v[0]==f.v[1]||f.v[1]==f.v[2]||f.v[2]==f.v[0])continue;
array<int,3>q={
f.v[0],f.v[1],f.v[2]}
;
sort(q.begin(),q.end());
if(!fk.insert(q).second)return 0;
nf.push_back(f);
}
VC<char>us(nV,0);
for(auto f:nf)for(int k=0;
k<3;
++k)us[f.v[k]]=1;
VC<int>nm(nV,-1);
VC<V3>nv;
for(int i=0;
i<nV;
++i)if(us[i])nm[i]=nv.size(),nv.push_back(ZV[i]);
for(auto&f:nf)for(int k=0;
k<3;
++k)f.v[k]=nm[f.v[k]];
if(!_r3(nv,nf))return 0;
V3 lo=ov[0],hi=lo;
for(auto p:ov){
lo.x=min(lo.x,p.x);
lo.y=min(lo.y,p.y);
lo.z=min(lo.z,p.z);
hi.x=max(hi.x,p.x);
hi.y=max(hi.y,p.y);
hi.z=max(hi.z,p.z);
}
DB H=.05*norm(hi-lo),H2=H*H;
auto ce=[&](V3 p){
return array<int,3>{
(int)floor((p.x-lo.x)/H),(int)floor((p.y-lo.y)/H),(int)floor((p.z-lo.z)/H)}
;
}
;
map<array<int,3>,VC<int>>Q;
for(int i=0;
i<(int)nv.size();
++i)Q[ce(nv[i])].push_back(i);
for(auto p:ov){
auto c=ce(p);
bool ok=false;
for(int x=-1;
x<=1&&!ok;
++x)for(int y=-1;
y<=1&&!ok;
++y)for(int z=-1;
z<=1&&!ok;
++z){
auto it=Q.find({
c[0]+x,c[1]+y,c[2]+z}
);
if(it!=Q.end())for(int j:it->second)if(norm2(p-nv[j])<=H2){
ok=true;
break;
}
}
if(!ok){
Q[c].push_back(nv.size());
nv.push_back(p);
}
}
if(nv.size()>=sv.size())return 0;
ZV.swap(nv);
ZF.swap(nf);
_e.assign(ZV.size(),0);
nV=ZV.size();
nF=ZF.size();
return sv.size()-nV;
}
void compact(){
VC<int>o2n(ZV.size(),-1);
VC<V3>nv;
nv.reserve(ZV.size()-_j);
VC<DB>nr;
nr.reserve(ZV.size()-_j);
for(int i=0;
i<(int)ZV.size();
++i)if(!ZD[i]){
o2n[i]=(int)nv.size();
nv.push_back(ZV[i]);
nr.push_back(i<(int)crad.size()?crad[i]:0.);
}
struct FK{
array<int,3>key;
FC face;
bool operator<(const FK&o)const{
return key<o.key;
}
}
;
VC<FK>fc;
fc.reserve(ZF.size());
for(int fi=0;
fi<(int)ZF.size();
++fi){
if(FD[fi])continue;
int a=ZF[fi].v[0],b=ZF[fi].v[1],c=ZF[fi].v[2];
if(a<0||b<0||c<0||a>=(int)ZV.size()||b>=(int)ZV.size()||c>=(int)ZV.size())continue;
if(ZD[a]||ZD[b]||ZD[c]||a==b||b==c||a==c)continue;
int na=o2n[a],nb=o2n[b],nc=o2n[c];
if(na<0||nb<0||nc<0||na==nb||nb==nc||na==nc)continue;
FC nf;
nf.v[0]=na;
nf.v[1]=nb;
nf.v[2]=nc;
array<int,3>key={
na,nb,nc}
;
sort(key.begin(),key.end());
fc.push_back({
key,nf}
);
}
sort(fc.begin(),fc.end());
VC<FC>nf;
nf.reserve(fc.size());
array<int,3>prev={
-1,-1,-1}
;
for(auto&item:fc){
if(item.key==prev)continue;
prev=item.key;
nf.push_back(item.face);
}
ZV.swap(nv);
ZF.swap(nf);
_e.swap(nr);
nV=(int)ZV.size();
nF=(int)ZF.size();
}
__attribute__((noinline,cold,section(".text.kale.t2")))void _w4(){
VC<V3>u=_U,p=ZV;
VC<FC>t=_T;
_w3();
VC<_u1>r(6);
for(int v=0;
v<6;
++v)r[v]=_x(u,t,v,1024);
A7(r,ZV,ZF,1024,true);
_P5(p,2,.3,true,8);
}
}
;
bool _v3::MEMLESS=false;
int main(){
_v3 s;
s.run();
return 0;
}
