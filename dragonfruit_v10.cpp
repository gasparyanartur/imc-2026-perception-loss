#include <bits/stdc++.h>
using namespace std;

static constexpr double hparam_HausdorffDiagFraction = 0.055;
static constexpr double CParam_MinNormalNorm = 1e-12;
static constexpr double CParam_QemSolveDeterminantEps = 1e-12;
static constexpr double CParam_Inf = 1e100;

static constexpr double hparam_TotalBudgetSeconds = 24.0;

// Active stage budgets.
static constexpr double hparam_FirstQemBudgetSeconds = 17;
static constexpr double hparam_GeneralStarPostBudgetSeconds = 18;
static constexpr int    hparam_OutputPrecisionSignificantDigits = 10;
static constexpr double hparam_KeepRatio_UpTo5k    = 0.00;
static constexpr double hparam_KeepRatio_UpTo25k   = 0.347;
static constexpr double hparam_KeepRatio_UpTo45k   = 0.175;
static constexpr double hparam_KeepRatio_UpTo50k   = 0.098;
static constexpr double hparam_KeepRatio_UpTo400k  = 0.025;
static constexpr double hparam_KeepRatio_Huge      = 0.033; // user sweep safe frontier
static constexpr double hparam_QemCostCapCoeff = 0.0375;

struct StarDeleteParams {
    int maxValence;
    double maxOldDev;
    double maxNewDev;
    double distFrac;
    double extraFrac;
    int hardCap;
    int scanVertices;
    int rounds;
    double timeFrac;
    double maxSeconds;
};

// Tier index is originalTier(): 1 small, 6 huge. Index 0 unused.
static constexpr StarDeleteParams hparam_GeneralStarParamsByTier[7] = {
    {0,0,0,0,0,0,0,0,0,0},
    {12,0.170,0.235,1.22,0.0450,33000,950000,8,0.94,6.90},
    {5,0.004,0.006,0.40,0.0015,900,105000,1,0.25,0.90},
    {6,0.008,0.012,0.52,0.0030,1800,160000,2,0.36,1.40},
    {5,0.0088,0.0130,0.56,0.0036,1950,160000,1,0.38,1.55},
    {5,0.006,0.009,0.46,0.0022,1300,130000,1,0.31,1.10},
    {7,0.012,0.018,0.64,0.0050,3200,240000,3,0.48,2.05}
};



// Third pass: local image-cost star deletion.
// This pass uses a local SSIM proxy between the current post-QEM/post-star patch and a
// candidate retriangulated patch. It does not compare against the original mesh; it is a
// conservative "take only visually near-identical extra deletions" pass after the known-safe
// frontier has already been reached.
static constexpr bool   hparam_EnableSsimThirdPass = true;
static constexpr int    hparam_SsimPatchResolution = 1024;
static constexpr int    hparam_SsimPatchPaddingPixels = 6;
static constexpr int    hparam_SsimPatchMaxPixels = 90000;
static constexpr int    hparam_SsimCandidatePoolCap = 40000;
static constexpr double hparam_SsimAcceptMin = 0.90;
static constexpr double hparam_SsimMaxDamage = 0.01;
static constexpr double hparam_SsimThirdPassMinTimeSeconds = 0.45;
static constexpr double hparam_SsimThirdPassTimeFrac = 0.9;
static constexpr double hparam_SsimThirdPassMaxSeconds = 9;
static constexpr double hparam_SsimNormalDepthWeight = 0.5;
static constexpr double hparam_SsimScoreGeomWeight = 0.0025;
static constexpr double hparam_SsimC1 = (0.01 * 255.0) * (0.01 * 255.0);
static constexpr double hparam_SsimC2 = (0.03 * 255.0) * (0.03 * 255.0);

// More permissive geometry than the general star pass, but still manifold/envelope guarded.
// Index 0 unused.
static constexpr StarDeleteParams hparam_SsimStarParamsByTier[7] = {
    {0,0,0,0,0,0,0,0,0,0},
    {14,0.260,0.340,1.00,0.0120,6000,280000,1,0.60,1.10},
    {7, 0.018,0.026,0.82,0.0012,500, 90000, 1,0.45,0.55},
    {8, 0.026,0.038,0.86,0.0016,700, 110000,1,0.45,0.65},
    {8, 0.028,0.042,0.88,0.0018,850, 120000,1,0.45,0.75},
    {7, 0.018,0.028,0.80,0.0008,350, 80000, 1,0.38,0.45},
    {9, 0.040,0.060,0.92,0.0020,1600,180000,1,0.48,0.95}
};

// Camera-aware face weight (scale‑invariant, bounded)
static constexpr double hparam_ViewWeightK = 3.0;
static constexpr double hparam_MaxFaceWeight = 3.0;

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

// Fast small sorted set
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
    bool contains(int v)const{
        return binary_search(data.begin(),data.end(),v);
    }
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
    void run(){
        readMesh();
        if(nV<=4){writeMesh();return;}
        startTime=chrono::steady_clock::now();
        initScale();
        buildConnectivity();
        initFaceWeights();
        initQueue();

        // Pass 1: ordinary QEM. No rendered visibility is used here.
        stageTimeBudget = hparam_FirstQemBudgetSeconds;
        collapseLoop();

        // General star-delete post-pass. This is the only post-QEM deletion pass in this
        // cleaned version; previous rendered hidden-star code was a measured no-op under
        // the current first-pass frontier.
        stageTimeBudget = hparam_GeneralStarPostBudgetSeconds;
        int postTier = originalTier();
        if (elapsed() < stageTimeBudget - 1.2) {
            generalStarDeletePostPass();
        }
        if (postTier != 2 && postTier != 3 && elapsed() < stageTimeBudget - 0.8) {
            generalStarDeletePostPass();
        }

        // Pass 3: SSIM-cost star-delete. This pass uses a local rendered normal/depth SSIM
        // proxy as an acceptance cost, while reusing the same topological and envelope guards.
        if (hparam_EnableSsimThirdPass && elapsed() < hparam_TotalBudgetSeconds - hparam_SsimThirdPassMinTimeSeconds) {
            stageTimeBudget = hparam_TotalBudgetSeconds;
            ssimStarDeletePass();
        }

        compact();
        writeMesh();
    }
private:
    int nV=0,nF=0;
    vector<Vec3> verts;
    vector<Face> faces;
    vector<char> vdead,fdead;
    vector<int> vver;
    vector<Quadric> vquad;
    vector<double> crad;
    vector<vector<int>> vfaces;
    vector<SmallSet> vneigh;
    priority_queue<CollapseCandidate> pq;
    int targetV=0,collapseLimit=0,accepted=0;
    double diag=0,hausd=0,costCap=CParam_Inf;
    double invDiag2 = 0.0;
    double stageTimeBudget = hparam_TotalBudgetSeconds;
    chrono::steady_clock::time_point startTime;

    // --- I/O and init (unchanged) ---
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
        for(int i=0;i<nV;++i){snprintf(line,sizeof(line),"v %.*g %.*g %.*g\n",hparam_OutputPrecisionSignificantDigits,verts[i].x,hparam_OutputPrecisionSignificantDigits,verts[i].y,hparam_OutputPrecisionSignificantDigits,verts[i].z);out+=line;}
        for(int i=0;i<nF;++i){snprintf(line,sizeof(line),"f %d %d %d\n",faces[i].v[0]+1,faces[i].v[1]+1,faces[i].v[2]+1);out+=line;}
        fwrite(out.data(),1,out.size(),stdout);
    }
    void initScale(){
        Vec3 mn=verts[0],mx=verts[0];
        for(auto&p:verts){mn.x=min(mn.x,p.x);mn.y=min(mn.y,p.y);mn.z=min(mn.z,p.z);mx.x=max(mx.x,p.x);mx.y=max(mx.y,p.y);mx.z=max(mx.z,p.z);}
        diag=norm(mx-mn);hausd=hparam_HausdorffDiagFraction*diag;
        costCap=hparam_QemCostCapCoeff*diag*diag;
        invDiag2 = (diag > CParam_MinNormalNorm) ? (1.0 / (diag*diag)) : 0.0;
        double kr;
        if(nV<=5000)kr=hparam_KeepRatio_UpTo5k;
        else if(nV<=25000)kr=hparam_KeepRatio_UpTo25k;
        else if(nV<=45000)kr=hparam_KeepRatio_UpTo45k;
        else if(nV<=50000)kr=hparam_KeepRatio_UpTo50k;
        else if(nV<=400000)kr=hparam_KeepRatio_UpTo400k;
        else kr=hparam_KeepRatio_Huge;
        targetV=max(10,(int)floor(nV*kr));
        targetV=min(targetV,nV-1);
        collapseLimit=nV-targetV;
    }
    void buildConnectivity(){
        vdead.assign(nV,0);fdead.assign(nF,0);vver.assign(nV,0);
        vquad.assign(nV,Quadric());crad.assign(nV,0.0);
        vfaces.assign(nV,{});vneigh.resize(nV);
        for(int fi=0;fi<nF;++fi){
            auto&f=faces[fi];
            Quadric q=Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);
            for(int k=0;k<3;++k){vfaces[f.v[k]].push_back(fi);vquad[f.v[k]]+=q;}
            for(int k=0;k<3;++k){int a=f.v[k],b=f.v[(k+1)%3];if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);}}
        }
    }

    // --- Scale‑invariant, bounded face weight ---
    double faceWeightFor(const Vec3& unitNormal, double area) const {
        double absSum = fabs(unitNormal.x) + fabs(unitNormal.y) + fabs(unitNormal.z);
        double normalizedArea = area * invDiag2;
        double w = 1.0 + hparam_ViewWeightK * normalizedArea * absSum;
        return min(w, hparam_MaxFaceWeight);
    }

    void initFaceWeights(){
        vquad.assign(nV, Quadric());
        for(int fi=0;fi<nF;++fi){
            const Face& f=faces[fi];
            Vec3 n=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);
            double area=0.5*norm(n);
            Quadric q = Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);
            if(area >= 1e-30){
                n = n / (2.0*area);
                double w = faceWeightFor(n, area);
                if(w != 1.0) q.scale(w);
            }
            for(int k=0;k<3;++k) vquad[f.v[k]] += q;
        }
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
    void collapseLoop(){
        int tick=0;
        while(accepted<collapseLimit&&!pq.empty()){
            if((++tick&8191)==0&&elapsed()>stageTimeBudget)break;
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
        vquad[kp]+=vquad[ab];
        for(int nb:vneigh[ab]){if(nb==kp||vdead[nb])continue;vneigh[nb].erase(ab);vneigh[nb].insert(kp);vneigh[kp].insert(nb);}
        vneigh[ab].clear();vneigh[kp].erase(ab);vneigh[kp].erase(kp);
        for(int nb:vneigh[kp]){if(nb==kp||vdead[nb])continue;auto c=computeQueueCandidate(kp,nb);if(c.valid())pq.push(c);}
    }

    // ---------- Star-delete retriangulation post-pass ----------
    static double clampDouble(double x,double lo,double hi){
        return x<lo?lo:(x>hi?hi:x);
    }

    static double pointTriangleDistance2(const Vec3& p,const Vec3& a,const Vec3& b,const Vec3& c){
        Vec3 ab=b-a, ac=c-a, ap=p-a;
        double d1=dot(ab,ap), d2=dot(ac,ap);
        if(d1<=0.0&&d2<=0.0)return norm2(ap);
        Vec3 bp=p-b;
        double d3=dot(ab,bp), d4=dot(ac,bp);
        if(d3>=0.0&&d4<=d3)return norm2(bp);
        double vc=d1*d4-d3*d2;
        if(vc<=0.0&&d1>=0.0&&d3<=0.0){
            double v=d1/(d1-d3);
            Vec3 q=a+ab*v;
            return norm2(p-q);
        }
        Vec3 cp=p-c;
        double d5=dot(ab,cp), d6=dot(ac,cp);
        if(d6>=0.0&&d5<=d6)return norm2(cp);
        double vb=d5*d2-d1*d6;
        if(vb<=0.0&&d2>=0.0&&d6<=0.0){
            double w=d2/(d2-d6);
            Vec3 q=a+ac*w;
            return norm2(p-q);
        }
        double va=d3*d6-d5*d4;
        if(va<=0.0&&(d4-d3)>=0.0&&(d5-d6)>=0.0){
            double w=(d4-d3)/((d4-d3)+(d5-d6));
            Vec3 q=b+(c-b)*w;
            return norm2(p-q);
        }
        Vec3 n=cross(ab,ac);
        double nn=norm2(n);
        if(nn<CParam_MinNormalNorm*CParam_MinNormalNorm)return CParam_Inf;
        double dist=dot(p-a,n);
        return (dist*dist)/nn;
    }

    Vec3 faceNormalRaw(int fi)const{
        const Face& f=faces[fi];
        return cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);
    }

    bool faceHasVertex(int fi,int v)const{
        const Face& f=faces[fi];
        return f.v[0]==v||f.v[1]==v||f.v[2]==v;
    }

    int originalTier() const{
        if(nV>=1000000)return 6;
        if(nV>=350000)return 5;
        if(nV>=45000)return 4;
        if(nV>=35000)return 3;
        if(nV>=20000)return 2;
        return 1;
    }

    StarDeleteParams starParams() const{
        int tier=originalTier();
        if(tier<1)tier=1;
        if(tier>6)tier=6;
        return hparam_GeneralStarParamsByTier[tier];
    }

    bool orientedRingForVertexWithParams(int v,const StarDeleteParams& sp,vector<int>& ring,vector<int>& inc)const{
        ring.clear();inc.clear();
        if(v<0||v>=(int)vfaces.size()||vdead[v])return false;
        for(int fi:vfaces[v]){
            if(fdead[fi])continue;
            if(faceHasVertex(fi,v))inc.push_back(fi);
        }
        int m=(int)inc.size();
        if(m<3||m>sp.maxValence)return false;
        vector<pair<int,int>> dir;
        dir.reserve(m);
        for(int fi:inc){
            const Face& f=faces[fi];
            int pos=-1;
            for(int k=0;k<3;++k)if(f.v[k]==v)pos=k;
            if(pos<0)return false;
            int a=f.v[(pos+1)%3],b=f.v[(pos+2)%3];
            if(a==b||a==v||b==v||vdead[a]||vdead[b])return false;
            dir.push_back({a,b});
        }
        for(int i=0;i<m;++i){
            for(int j=i+1;j<m;++j){
                if(dir[i].first==dir[j].first)return false;
                if(dir[i].second==dir[j].second)return false;
            }
        }
        int start=dir[0].first,cur=start;
        ring.push_back(start);
        for(int step=0;step<m;++step){
            int nxt=-1;
            for(auto&e:dir)if(e.first==cur){nxt=e.second;break;}
            if(nxt<0)return false;
            if(step==m-1){
                if(nxt!=start)return false;
            }else{
                for(int x:ring)if(x==nxt)return false;
                ring.push_back(nxt);
                cur=nxt;
            }
        }
        return (int)ring.size()==m;
    }

    bool activeFaceWithSameKey(int a,int b,int c,const vector<int>& skip)const{
        array<int,3> key={a,b,c};
        sort(key.begin(),key.end());
        for(int fi:vfaces[a]){
            if(fdead[fi])continue;
            bool skipFace=false;
            for(int s:skip)if(s==fi){skipFace=true;break;}
            if(skipFace)continue;
            const Face& f=faces[fi];
            array<int,3> k2={f.v[0],f.v[1],f.v[2]};
            sort(k2.begin(),k2.end());
            if(k2==key)return true;
        }
        return false;
    }

    struct StarCandidate{
        int v=-1,root=0;
        double score=CParam_Inf;
        bool valid()const{return v>=0&&score<CParam_Inf;}
        bool operator<(const StarCandidate& o)const{return score<o.score;}
    };

    bool evaluateStarRootWithParams(int v,const vector<int>& ring,const vector<int>& inc,int root,double oldDev,const Vec3& avgN,const StarDeleteParams& sp,StarCandidate& out)const{
        int m=(int)ring.size();
        vector<int> rr;
        rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        int r0=rr[0];
        for(int i=2;i<=m-2;++i){
            if(vneigh[r0].contains(rr[i]))return false;
        }
        double maxNewDev=0.0;
        double minDist2=CParam_Inf;
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
        out.v=v;
        out.root=root;
        out.score=(crad[v]+dist)/(hausd+CParam_MinNormalNorm)+0.35*oldDev+0.25*maxNewDev+1e-4*m;
        return true;
    }

    StarCandidate computeStarCandidateWithParams(int v,const StarDeleteParams& sp)const{
        StarCandidate best;
        vector<int> ring,inc;
        if(!orientedRingForVertexWithParams(v,sp,ring,inc))return best;
        Vec3 avg;
        double areaSum=0.0,oldDev=0.0;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            if(nl<CParam_MinNormalNorm)return best;
            avg=avg+n;
            areaSum+=0.5*nl;
        }
        double al=norm(avg);
        if(al<CParam_MinNormalNorm||areaSum<=0.0)return best;
        avg=avg/al;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            Vec3 un=n/nl;
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

    bool applyStarDeleteWithParams(int v,int root,const StarDeleteParams& sp){
        StarCandidate best=computeStarCandidateWithParams(v,sp);
        if(!best.valid())return false;
        root=best.root;
        vector<int> ring,inc;
        if(!orientedRingForVertexWithParams(v,sp,ring,inc))return false;
        int m=(int)ring.size();
        vector<int> rr;
        rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        for(int fi:inc){
            if(fdead[fi])continue;
            Face old=faces[fi];
            fdead[fi]=1;
            for(int k=0;k<3;++k){
                int u=old.v[k];
                if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi);
            }
        }
        for(int nb:ring)if(nb>=0&&nb<(int)vneigh.size())vneigh[nb].erase(v);
        vneigh[v].clear();
        vfaces[v].clear();
        vdead[v]=1;
        crad[v]=0.0;
        ++vver[v];
        for(int i=1;i<m-1;++i){
            Face nf;
            nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];
            int fi=(int)faces.size();
            faces.push_back(nf);
            fdead.push_back(0);
            for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);
            for(int k=0;k<3;++k){
                int a=nf.v[k],b=nf.v[(k+1)%3];
                if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);}
            }
        }
        return true;
    }


    // Replay state for trying multiple safe star-delete orders and keeping the best one.
    // This is only used in the post-pass, after the QEM priority queue is done.
    struct StarState{
        vector<Vec3> verts;
        vector<Face> faces;
        vector<char> vdead,fdead;
        vector<int> vver;
        vector<double> crad;
        vector<vector<int>> vfaces;
        vector<SmallSet> vneigh;
        int accepted=0;
    };

    StarState saveStarState()const{
        StarState st;
        st.verts=verts;
        st.faces=faces;
        st.vdead=vdead;
        st.fdead=fdead;
        st.vver=vver;
        st.crad=crad;
        st.vfaces=vfaces;
        st.vneigh=vneigh;
        st.accepted=accepted;
        return st;
    }

    void loadStarState(const StarState& st){
        verts=st.verts;
        faces=st.faces;
        vdead=st.vdead;
        fdead=st.fdead;
        vver=st.vver;
        crad=st.crad;
        vfaces=st.vfaces;
        vneigh=st.vneigh;
        accepted=st.accepted;
    }

    int runStarCandidates(const StarDeleteParams& sp, vector<StarCandidate> cands,int maxExtra,int& extra,double stopTime,int mode){
        // mode 0: original score order
        // mode 1: high-valence first, then score
        // mode 2: low-neighborhood first, then score
        if(mode==0){
            sort(cands.begin(),cands.end());
        }else if(mode==1){
            sort(cands.begin(),cands.end(),[&](const StarCandidate& a,const StarCandidate& b){
                int va=(a.v>=0&&a.v<(int)vfaces.size())?(int)vfaces[a.v].size():0;
                int vb=(b.v>=0&&b.v<(int)vfaces.size())?(int)vfaces[b.v].size():0;
                if(va!=vb)return va>vb;
                return a.score<b.score;
            });
        }else{
            sort(cands.begin(),cands.end(),[&](const StarCandidate& a,const StarCandidate& b){
                int na=(a.v>=0&&a.v<(int)vneigh.size())?vneigh[a.v].size():0;
                int nb=(b.v>=0&&b.v<(int)vneigh.size())?vneigh[b.v].size():0;
                if(na!=nb)return na<nb;
                return a.score<b.score;
            });
        }
        int before=extra;
        for(const StarCandidate& c:cands){
            if(extra>=maxExtra||elapsed()>=stopTime)break;
            if(c.v<0||c.v>=(int)vdead.size()||vdead[c.v])continue;
            if(applyStarDeleteWithParams(c.v,c.root,sp)){
                ++accepted;
                ++extra;
            }
        }
        return extra-before;
    }


    struct DoubleStarCandidate{
        int a=-1,b=-1,root=0;
        double score=CParam_Inf;
        vector<int> boundary;
        vector<int> patchFaces;
        bool valid()const{return a>=0&&b>=0&&score<CParam_Inf;}
        bool operator<(const DoubleStarCandidate& o)const{return score<o.score;}
    };

    bool appendUniqueInt(vector<int>& xs,int v)const{
        for(int x:xs)if(x==v)return false;
        xs.push_back(v);
        return true;
    }

    bool combinedPatchForEdge(int a,int b,vector<int>& boundary,vector<int>& patchFaces)const{
        boundary.clear();
        patchFaces.clear();
        if(!edgeExists(a,b))return false;
        for(int fi:vfaces[a]){
            if(fdead[fi])continue;
            if(faceHasVertex(fi,a)||faceHasVertex(fi,b))appendUniqueInt(patchFaces,fi);
        }
        for(int fi:vfaces[b]){
            if(fdead[fi])continue;
            if(faceHasVertex(fi,a)||faceHasVertex(fi,b))appendUniqueInt(patchFaces,fi);
        }
        if(patchFaces.size()<4||patchFaces.size()>14)return false;
        vector<pair<int,int>> edges;
        edges.reserve(patchFaces.size()*3);
        for(int fi:patchFaces){
            const Face& f=faces[fi];
            edges.push_back({f.v[0],f.v[1]});
            edges.push_back({f.v[1],f.v[2]});
            edges.push_back({f.v[2],f.v[0]});
        }
        vector<pair<int,int>> bedges;
        bedges.reserve(edges.size());
        for(auto& e:edges){
            bool internal=false;
            for(auto& r:edges){
                if(e.first==r.second&&e.second==r.first){internal=true;break;}
            }
            if(!internal)bedges.push_back(e);
        }
        int m=(int)bedges.size();
        if(m<4||m>10)return false;
        for(auto& e:bedges){
            if(e.first==a||e.first==b||e.second==a||e.second==b)return false;
        }
        vector<int> starts,ends;
        for(auto& e:bedges){
            if(!appendUniqueInt(starts,e.first))return false;
            if(!appendUniqueInt(ends,e.second))return false;
        }
        int start=bedges[0].first;
        int cur=start;
        boundary.push_back(cur);
        for(int step=0;step<m;++step){
            int nxt=-1;
            for(auto& e:bedges)if(e.first==cur){nxt=e.second;break;}
            if(nxt<0)return false;
            if(step==m-1){
                if(nxt!=start)return false;
            }else{
                for(int x:boundary)if(x==nxt)return false;
                boundary.push_back(nxt);
                cur=nxt;
            }
        }
        return (int)boundary.size()==m;
    }

    bool evaluateDoubleRoot(int a,int b,const vector<int>& boundary,const vector<int>& patchFaces,int root,const Vec3& avgN,double oldDev,DoubleStarCandidate& out)const{
        StarDeleteParams sp=starParams();
        int m=(int)boundary.size();
        vector<int> rr;
        rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(boundary[(root+i)%m]);
        int r0=rr[0];
        for(int i=2;i<=m-2;++i){
            if(vneigh[r0].contains(rr[i]))return false;
        }
        double maxNewDev=0.0;
        double minDa=CParam_Inf,minDb=CParam_Inf;
        for(int i=1;i<m-1;++i){
            int x=rr[0],y=rr[i],z=rr[i+1];
            if(x==y||y==z||x==z)return false;
            if(activeFaceWithSameKey(x,y,z,patchFaces))return false;
            Vec3 n=cross(verts[y]-verts[x],verts[z]-verts[x]);
            double nl=norm(n);
            if(nl<CParam_MinNormalNorm)return false;
            Vec3 un=n/nl;
            double d=clampDouble(dot(un,avgN),-1.0,1.0);
            if(d<=0.0)return false;
            maxNewDev=max(maxNewDev,1.0-d);
            if(maxNewDev>sp.maxNewDev*1.12)return false;
            minDa=min(minDa,pointTriangleDistance2(verts[a],verts[x],verts[y],verts[z]));
            minDb=min(minDb,pointTriangleDistance2(verts[b],verts[x],verts[y],verts[z]));
        }
        double da=sqrt(max(0.0,minDa));
        double db=sqrt(max(0.0,minDb));
        if(crad[a]+da>hausd*sp.distFrac)return false;
        if(crad[b]+db>hausd*sp.distFrac)return false;
        out.a=a;
        out.b=b;
        out.root=root;
        out.boundary=boundary;
        out.patchFaces=patchFaces;
        double nd=max(crad[a]+da,crad[b]+db)/(hausd+CParam_MinNormalNorm);
        out.score=nd+0.35*oldDev+0.25*maxNewDev+2e-4*m;
        return true;
    }

    DoubleStarCandidate computeDoubleStarCandidate(int a,int b)const{
        DoubleStarCandidate best;
        vector<int> boundary,patchFaces;
        if(!combinedPatchForEdge(a,b,boundary,patchFaces))return best;
        Vec3 avg;
        vector<Vec3> ns;
        ns.reserve(patchFaces.size());
        for(int fi:patchFaces){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            if(nl<CParam_MinNormalNorm)return best;
            Vec3 un=n/nl;
            ns.push_back(un);
            avg=avg+un;
        }
        double al=norm(avg);
        if(al<CParam_MinNormalNorm)return best;
        avg=avg/al;
        double oldDev=0.0;
        for(const Vec3& n:ns){
            double d=clampDouble(dot(n,avg),-1.0,1.0);
            if(d<=0.0)return best;
            oldDev=max(oldDev,1.0-d);
        }
        StarDeleteParams sp=starParams();
        if(oldDev>sp.maxOldDev*1.35)return best;
        for(int root=0;root<(int)boundary.size();++root){
            DoubleStarCandidate c;
            if(evaluateDoubleRoot(a,b,boundary,patchFaces,root,avg,oldDev,c)&&c.score<best.score)best=c;
        }
        return best;
    }

    bool applyDoubleStarDelete(int a,int b){
        DoubleStarCandidate best=computeDoubleStarCandidate(a,b);
        if(!best.valid())return false;
        const vector<int>& boundary=best.boundary;
        const vector<int>& patchFaces=best.patchFaces;
        int m=(int)boundary.size();
        vector<int> rr;
        rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(boundary[(best.root+i)%m]);
        for(int fi:patchFaces){
            if(fdead[fi])continue;
            Face old=faces[fi];
            fdead[fi]=1;
            for(int k=0;k<3;++k){
                int u=old.v[k];
                if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi);
            }
        }
        for(int x:{a,b}){
            for(int nb:vneigh[x])if(nb>=0&&nb<(int)vneigh.size())vneigh[nb].erase(x);
            vneigh[x].clear();
            vfaces[x].clear();
            vdead[x]=1;
            crad[x]=0.0;
            ++vver[x];
        }
        for(int i=1;i<m-1;++i){
            Face nf;
            nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];
            int fi=(int)faces.size();
            faces.push_back(nf);
            fdead.push_back(0);
            for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);
            for(int k=0;k<3;++k){
                int x=nf.v[k],y=nf.v[(k+1)%3];
                if(x!=y){vneigh[x].insert(y);vneigh[y].insert(x);}
            }
        }
        return true;
    }

    int collapseDoubleStars(const StarDeleteParams& sp,double stopTime,int maxExtra,int& extra){
        if(extra+2>maxExtra)return 0;
        vector<DoubleStarCandidate> cands;
        cands.reserve(2048);
        int scanned=0;
        int scanEdges=max(1000,sp.scanVertices*2);
        for(int a=0;a<(int)verts.size()&&scanned<scanEdges&&elapsed()<stopTime;++a){
            if(vdead[a])continue;
            for(int b:vneigh[a]){
                if(scanned>=scanEdges||elapsed()>=stopTime)break;
                if(b<=a||vdead[b])continue;
                ++scanned;
                DoubleStarCandidate c=computeDoubleStarCandidate(a,b);
                if(c.valid())cands.push_back(c);
            }
        }
        if(cands.empty())return 0;
        sort(cands.begin(),cands.end());
        int deleted=0;
        int maxDeleted=max(2,min(maxExtra-extra,sp.hardCap/3));
        for(const DoubleStarCandidate& c:cands){
            if(deleted+2>maxDeleted||extra+2>maxExtra||elapsed()>=stopTime)break;
            if(vdead[c.a]||vdead[c.b]||!edgeExists(c.a,c.b))continue;
            if(applyDoubleStarDelete(c.a,c.b)){
                accepted+=2;
                extra+=2;
                deleted+=2;
            }
        }
        return deleted;
    }

    void generalStarDeletePostPass(){
        StarDeleteParams sp=starParams();
        double timeLeft=stageTimeBudget-elapsed();
        if(timeLeft<0.35)return;
        double stopTime=elapsed()+min(sp.maxSeconds,timeLeft*sp.timeFrac);
        int maxExtra=min(sp.hardCap,max(0,(int)floor(nV*sp.extraFrac)));
        if(maxExtra<=0)return;
        int extra=0;
        int tier=originalTier();
        for(int round=0;round<sp.rounds&&extra<maxExtra&&elapsed()<stopTime;++round){
            vector<StarCandidate> cands;
            cands.reserve(4096);
            int scanned=0;
            for(int v=0;v<(int)verts.size()&&scanned<sp.scanVertices&&elapsed()<stopTime;++v){
                if(vdead[v])continue;
                ++scanned;
                StarCandidate c=computeStarCandidateWithParams(v,sp);
                if(c.valid())cands.push_back(c);
            }
            if(cands.empty())break;
            bool progress=false;

            // Replay only on tiers that we now want to explore.
            // Tiers 2/3 are fragile, so they use normal deterministic order.
            // Replay is enabled for tiers 1/4/5/6.
            if(tier==1 || tier==4 || tier==5 || tier==6){
                StarState base=saveStarState();
                StarState bestState=base;
                int bestExtra=extra;
                int bestGain=-1;
                int modes = 3;
                for(int mode=0;mode<modes&&elapsed()<stopTime;++mode){
                    loadStarState(base);
                    int trialExtra=extra;
                    int gain=runStarCandidates(sp,cands,maxExtra,trialExtra,stopTime,mode);
                    if(trialExtra<maxExtra&&elapsed()<stopTime){
                        int beforeDouble=trialExtra;
                        collapseDoubleStars(sp,stopTime,maxExtra,trialExtra);
                        gain += trialExtra-beforeDouble;
                    }
                    if(gain>bestGain){
                        bestGain=gain;
                        bestExtra=trialExtra;
                        bestState=saveStarState();
                    }
                }
                loadStarState(bestState);
                extra=bestExtra;
                progress=bestGain>0;
            }else{
                int gain=runStarCandidates(sp,cands,maxExtra,extra,stopTime,0);
                progress=gain>0;
                if(extra<maxExtra&&elapsed()<stopTime){
                    int beforeDouble=extra;
                    collapseDoubleStars(sp,stopTime,maxExtra,extra);
                    if(extra>beforeDouble)progress=true;
                }
            }
            if(!progress)break;
        }
    }




    // ---------- Third pass: local SSIM-cost star-delete ----------
    StarDeleteParams ssimStarParams() const{
        int tier=originalTier();
        if(tier<1)tier=1;
        if(tier>6)tier=6;
        return hparam_SsimStarParamsByTier[tier];
    }

    struct SsimProj{
        double u=0.0,v=0.0,z=0.0;
        bool ok=false;
    };

    struct SsimTri{
        Vec3 p[3];
    };

    struct SsimPixel{
        float n[3];
        float d;
        unsigned char fg;
    };

    struct SsimDeleteCandidate{
        int v=-1,root=0;
        double ssim=1.0,score=CParam_Inf;
        bool operator<(const SsimDeleteCandidate& o)const{return score<o.score;}
    };

    static SsimPixel ssimBackgroundPixel(){
        SsimPixel p;
        p.n[0]=127.5f;p.n[1]=127.5f;p.n[2]=127.5f;
        p.d=255.0f;p.fg=0;
        return p;
    }

    void ssimCameraBasis(int view,Vec3& eye,Vec3& right,Vec3& up,Vec3& fwd)const{
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
        if(fl<CParam_MinNormalNorm){fwd=Vec3(0,0,-1);}else fwd=fwd/fl;
        Vec3 worldUp=(fabs(dot(fwd,Vec3(0,0,1)))>0.9)?Vec3(0,1,0):Vec3(0,0,1);
        right=cross(worldUp,fwd);
        double rl=norm(right);
        if(rl<CParam_MinNormalNorm)right=Vec3(1,0,0);else right=right/rl;
        up=cross(fwd,right);
        double ul=norm(up);
        if(ul<CParam_MinNormalNorm)up=Vec3(0,1,0);else up=up/ul;
    }

    SsimProj ssimProjectPoint(const Vec3& p,int view,int R)const{
        Vec3 eye,right,up,fwd;
        ssimCameraBasis(view,eye,right,up,fwd);
        Vec3 rel=p-eye;
        double x=dot(rel,right);
        double y=dot(rel,up);
        double z=dot(rel,fwd);
        if(z<=1e-8)return {};
        double scale=double(R)/1024.0;
        double f=800.0*scale;
        double c=0.5*double(R);
        return {f*x/z+c,f*y/z+c,z,true};
    }

    bool buildStarPatchTris(int v,int root,const StarDeleteParams& sp,vector<SsimTri>& oldTris,vector<SsimTri>& newTris)const{
        oldTris.clear();newTris.clear();
        vector<int> ring,inc;
        if(!orientedRingForVertexWithParams(v,sp,ring,inc))return false;
        Vec3 avg;
        double areaSum=0.0,oldDev=0.0;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            if(nl<CParam_MinNormalNorm)return false;
            avg=avg+n;
            areaSum+=0.5*nl;
        }
        double al=norm(avg);
        if(al<CParam_MinNormalNorm||areaSum<=0.0)return false;
        avg=avg/al;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            Vec3 un=n/nl;
            double d=clampDouble(dot(un,avg),-1.0,1.0);
            if(d<=0.0)return false;
            oldDev=max(oldDev,1.0-d);
        }
        if(oldDev>sp.maxOldDev)return false;
        StarCandidate tmp;
        if(root<0||root>=(int)ring.size())return false;
        if(!evaluateStarRootWithParams(v,ring,inc,root,oldDev,avg,sp,tmp))return false;

        oldTris.reserve(inc.size());
        for(int fi:inc){
            const Face& f=faces[fi];
            SsimTri t;
            t.p[0]=verts[f.v[0]];t.p[1]=verts[f.v[1]];t.p[2]=verts[f.v[2]];
            oldTris.push_back(t);
        }
        int m=(int)ring.size();
        vector<int> rr;
        rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        newTris.reserve(max(0,m-2));
        for(int i=1;i<m-1;++i){
            SsimTri t;
            t.p[0]=verts[rr[0]];t.p[1]=verts[rr[i]];t.p[2]=verts[rr[i+1]];
            newTris.push_back(t);
        }
        return !oldTris.empty()&&!newTris.empty();
    }

    static double ssimPixelChannel(const SsimPixel& p,int ch){
        if(ch<3)return p.n[ch];
        return p.d;
    }

    double ssimScalarChannel(const vector<SsimPixel>& a,const vector<SsimPixel>& b,int ch)const{
        double sx=0.0,sy=0.0,sxx=0.0,syy=0.0,sxy=0.0;
        int cnt=0;
        int n=(int)a.size();
        for(int i=0;i<n;++i){
            if(!a[i].fg&&!b[i].fg)continue;
            double x=ssimPixelChannel(a[i],ch);
            double y=ssimPixelChannel(b[i],ch);
            sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;++cnt;
        }
        if(cnt<4)return 1.0;
        double inv=1.0/double(cnt);
        double mx=sx*inv,my=sy*inv;
        double vx=max(0.0,sxx*inv-mx*mx);
        double vy=max(0.0,syy*inv-my*my);
        double cov=sxy*inv-mx*my;
        double num=(2.0*mx*my+hparam_SsimC1)*(2.0*cov+hparam_SsimC2);
        double den=(mx*mx+my*my+hparam_SsimC1)*(vx+vy+hparam_SsimC2);
        if(den<=0.0)return 1.0;
        return clampDouble(num/den,-1.0,1.0);
    }

    double ssimBuffers(const vector<SsimPixel>& a,const vector<SsimPixel>& b)const{
        int cnt=0;
        for(int i=0;i<(int)a.size();++i)if(a[i].fg||b[i].fg)++cnt;
        if(cnt==0)return 1.0;
        double sn=(ssimScalarChannel(a,b,0)+ssimScalarChannel(a,b,1)+ssimScalarChannel(a,b,2))/3.0;
        double sd=ssimScalarChannel(a,b,3);
        return hparam_SsimNormalDepthWeight*sn+(1.0-hparam_SsimNormalDepthWeight)*sd;
    }

    bool renderSsimPatch(const vector<SsimTri>& tris,int view,int x0,int y0,int w,int h,vector<SsimPixel>& buf)const{
        SsimPixel bg=ssimBackgroundPixel();
        buf.assign(w*h,bg);
        vector<float> zbuf(w*h,numeric_limits<float>::infinity());
        const int R=hparam_SsimPatchResolution;
        for(const SsimTri& tri:tris){
            SsimProj p0=ssimProjectPoint(tri.p[0],view,R);
            SsimProj p1=ssimProjectPoint(tri.p[1],view,R);
            SsimProj p2=ssimProjectPoint(tri.p[2],view,R);
            if(!p0.ok||!p1.ok||!p2.ok)continue;
            double area2=(p1.u-p0.u)*(p2.v-p0.v)-(p1.v-p0.v)*(p2.u-p0.u);
            if(fabs(area2)<1e-12)continue;
            Vec3 nr=cross(tri.p[1]-tri.p[0],tri.p[2]-tri.p[0]);
            double nl=norm(nr);
            if(nl<CParam_MinNormalNorm)continue;
            nr=nr/nl;
            int bx0=max(x0,(int)floor(min({p0.u,p1.u,p2.u})));
            int bx1=min(x0+w-1,(int)ceil (max({p0.u,p1.u,p2.u})));
            int by0=max(y0,(int)floor(min({p0.v,p1.v,p2.v})));
            int by1=min(y0+h-1,(int)ceil (max({p0.v,p1.v,p2.v})));
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

    double localSsimForStarCandidate(int v,int root,const StarDeleteParams& sp)const{
        vector<SsimTri> oldTris,newTris;
        if(!buildStarPatchTris(v,root,sp,oldTris,newTris))return -1.0;
        const int R=hparam_SsimPatchResolution;
        double total=0.0;
        int usedViews=0;
        vector<SsimPixel> a,b;
        for(int view=0;view<6;++view){
            double mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;
            bool any=false;
            auto includeTri=[&](const SsimTri& t){
                for(int k=0;k<3;++k){
                    SsimProj p=ssimProjectPoint(t.p[k],view,R);
                    if(!p.ok)continue;
                    any=true;
                    mnU=min(mnU,p.u);mxU=max(mxU,p.u);
                    mnV=min(mnV,p.v);mxV=max(mxV,p.v);
                }
            };
            for(const SsimTri& t:oldTris)includeTri(t);
            for(const SsimTri& t:newTris)includeTri(t);
            if(!any)continue;
            int pad=hparam_SsimPatchPaddingPixels;
            int x0=max(0,(int)floor(mnU)-pad);
            int y0=max(0,(int)floor(mnV)-pad);
            int x1=min(R-1,(int)ceil(mxU)+pad);
            int y1=min(R-1,(int)ceil(mxV)+pad);
            if(x0>x1||y0>y1)continue;
            int w=x1-x0+1,h=y1-y0+1;
            if(w*h>hparam_SsimPatchMaxPixels)return -1.0;
            renderSsimPatch(oldTris,view,x0,y0,w,h,a);
            renderSsimPatch(newTris,view,x0,y0,w,h,b);
            double s=ssimBuffers(a,b);
            total+=s;
            ++usedViews;
        }
        if(usedViews==0)return 1.0;
        return total/double(usedViews);
    }

    bool applyStarDeleteExactWithParams(int v,int root,const StarDeleteParams& sp){
        vector<int> ring,inc;
        if(!orientedRingForVertexWithParams(v,sp,ring,inc))return false;
        Vec3 avg;
        double areaSum=0.0,oldDev=0.0;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            if(nl<CParam_MinNormalNorm)return false;
            avg=avg+n;
            areaSum+=0.5*nl;
        }
        double al=norm(avg);
        if(al<CParam_MinNormalNorm||areaSum<=0.0)return false;
        avg=avg/al;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            Vec3 un=n/nl;
            double d=clampDouble(dot(un,avg),-1.0,1.0);
            if(d<=0.0)return false;
            oldDev=max(oldDev,1.0-d);
        }
        if(oldDev>sp.maxOldDev)return false;
        StarCandidate check;
        if(root<0||root>=(int)ring.size())return false;
        if(!evaluateStarRootWithParams(v,ring,inc,root,oldDev,avg,sp,check))return false;

        int m=(int)ring.size();
        vector<int> rr;
        rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        for(int fi:inc){
            if(fdead[fi])continue;
            Face old=faces[fi];
            fdead[fi]=1;
            for(int k=0;k<3;++k){
                int u=old.v[k];
                if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi);
            }
        }
        for(int nb:ring)if(nb>=0&&nb<(int)vneigh.size())vneigh[nb].erase(v);
        vneigh[v].clear();
        vfaces[v].clear();
        vdead[v]=1;
        crad[v]=0.0;
        ++vver[v];
        for(int i=1;i<m-1;++i){
            Face nf;
            nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];
            int fi=(int)faces.size();
            faces.push_back(nf);
            fdead.push_back(0);
            for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);
            for(int k=0;k<3;++k){
                int a=nf.v[k],b=nf.v[(k+1)%3];
                if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);}
            }
        }
        return true;
    }

    void ssimStarDeletePass(){
        StarDeleteParams sp=ssimStarParams();
        double timeLeft=stageTimeBudget-elapsed();
        if(timeLeft<hparam_SsimThirdPassMinTimeSeconds)return;
        double stopTime=elapsed()+min(hparam_SsimThirdPassMaxSeconds,timeLeft*hparam_SsimThirdPassTimeFrac);
        int maxExtra=min(sp.hardCap,max(0,(int)floor(nV*sp.extraFrac)));
        if(maxExtra<=0)return;
        vector<SsimDeleteCandidate> cands;
        cands.reserve(min(hparam_SsimCandidatePoolCap, maxExtra*8+128));
        int scanned=0;
        for(int v=0;v<(int)verts.size()&&scanned<sp.scanVertices&&elapsed()<stopTime;++v){
            if(vdead[v])continue;
            ++scanned;
            StarCandidate geom=computeStarCandidateWithParams(v,sp);
            if(!geom.valid())continue;
            double s=localSsimForStarCandidate(geom.v,geom.root,sp);
            if(s<0.0)continue;
            double damage=1.0-s;
            if(s<hparam_SsimAcceptMin||damage>hparam_SsimMaxDamage)continue;
            SsimDeleteCandidate dc;
            dc.v=geom.v;
            dc.root=geom.root;
            dc.ssim=s;
            dc.score=damage+hparam_SsimScoreGeomWeight*geom.score;
            cands.push_back(dc);
            if((int)cands.size()>=hparam_SsimCandidatePoolCap)break;
        }
        if(cands.empty())return;
        sort(cands.begin(),cands.end());
        int extra=0;
        for(const SsimDeleteCandidate& c:cands){
            if(extra>=maxExtra||elapsed()>=stopTime)break;
            if(c.v<0||c.v>=(int)vdead.size()||vdead[c.v])continue;
            double s=localSsimForStarCandidate(c.v,c.root,sp);
            if(s<hparam_SsimAcceptMin||1.0-s>hparam_SsimMaxDamage)continue;
            if(applyStarDeleteExactWithParams(c.v,c.root,sp)){
                ++accepted;
                ++extra;
            }
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

int main(){QemSimplifier s;s.run();return 0;}