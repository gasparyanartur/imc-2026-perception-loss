#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, json, math, os, subprocess, time
from pathlib import Path
import numpy as np
import trimesh
from scipy.spatial import cKDTree


def normalize(m: trimesh.Trimesh) -> trimesh.Trimesh:
    m = m.copy()
    m.remove_unreferenced_vertices()
    m.fix_normals(multibody=True)
    m.vertices -= m.bounds.mean(axis=0)
    mx = np.linalg.norm(m.vertices, axis=1).max()
    if mx > 0: m.vertices /= mx
    return m

def ico(sub=5): return trimesh.creation.icosphere(subdivisions=sub, radius=1.0)

def deform_sphere(kind: str, sub=5):
    m=ico(sub); p=m.vertices.copy(); r=np.linalg.norm(p,axis=1); u=p/r[:,None]
    x,y,z=u[:,0],u[:,1],u[:,2]
    if kind=='ellipsoid': p*=np.array([1.0,.68,.43])
    elif kind=='peanut': p[:,0]*=.62+.42*np.abs(z); p[:,1]*=.62+.42*np.abs(z); p[:,2]*=1.05
    elif kind=='bumpy':
        th=np.arctan2(y,x); ph=np.arccos(np.clip(z,-1,1)); rr=1+.055*np.sin(7*th)*np.sin(5*ph)+.035*np.cos(11*th+2*ph); p=u*rr[:,None]
    elif kind=='wavy':
        th=np.arctan2(y,x); ph=np.arccos(np.clip(z,-1,1)); rr=1+.10*np.sin(14*th)*np.sin(9*ph); p=u*rr[:,None]
    elif kind=='dimple':
        d2=(x-1)**2+y*y+z*z; rr=1-.28*np.exp(-d2/.10); p=u*rr[:,None]
    elif kind=='rounded_cube':
        q=.48; p=np.sign(u)*np.abs(u)**q; p/=np.linalg.norm(p,axis=1)[:,None]
    elif kind=='organic':
        th=np.arctan2(y,x); ph=np.arccos(np.clip(z,-1,1)); rr=1+.09*np.sin(3*th+1.7)*np.sin(4*ph)+.06*np.cos(5*th-3*ph); p=u*rr[:,None]
    m.vertices=p; return normalize(m)

def torus(kind='torus', major=128, minor=64):
    th=np.linspace(0,2*np.pi,major,endpoint=False); ph=np.linspace(0,2*np.pi,minor,endpoint=False)
    vv=[]
    for i,t in enumerate(th):
        mod=0
        if kind=='gear_torus': mod=.07*np.cos(12*t)
        for q in ph:
            rr=.34*(1+(.16*np.cos(5*t+3*q) if kind=='wavy_torus' else 0))
            R=.72+mod
            vv.append(((R+rr*np.cos(q))*np.cos(t),(R+rr*np.cos(q))*np.sin(t),rr*np.sin(q)))
    ff=[]
    for i in range(major):
        for j in range(minor):
            a=i*minor+j;b=((i+1)%major)*minor+j;c=i*minor+(j+1)%minor;d=((i+1)%major)*minor+(j+1)%minor
            ff.append((a,b,c));ff.append((b,d,c))
    return normalize(trimesh.Trimesh(vv,ff,process=False))

def subdivided(primitive: str, levels: int):
    if primitive=='cube': m=trimesh.creation.box(extents=(1.6,1.6,1.6))
    elif primitive=='thin_box': m=trimesh.creation.box(extents=(1.8,1.2,.12))
    elif primitive=='cylinder': m=trimesh.creation.cylinder(radius=.75,height=1.5,sections=192)
    elif primitive=='cone': m=trimesh.creation.cone(radius=.8,height=1.6,sections=192)
    elif primitive=='capsule': return normalize(trimesh.creation.capsule(height=1.2,radius=.45,count=[128,64]))
    else: raise ValueError(primitive)
    for _ in range(levels):
        v,f=trimesh.remesh.subdivide(m.vertices,m.faces)
        m=trimesh.Trimesh(v,f,process=False)
    return normalize(m)

def shape_set():
    return {
        'sphere_smooth': normalize(ico(5)),
        'ellipsoid': deform_sphere('ellipsoid'),
        'peanut_concave': deform_sphere('peanut'),
        'dimple_concave': deform_sphere('dimple'),
        'rounded_cube': deform_sphere('rounded_cube'),
        'organic_lowfreq': deform_sphere('organic'),
        'bumpy_midfreq': deform_sphere('bumpy'),
        'wavy_highfreq': deform_sphere('wavy'),
        'torus': torus('torus'),
        'thin_torus': torus('torus',160,48),
        'gear_torus': torus('gear_torus'),
        'wavy_torus': torus('wavy_torus'),
        'cube_planar_sharp': subdivided('cube',5),
        'thin_box_sharp': subdivided('thin_box',5),
        'cylinder_sharp': subdivided('cylinder',3),
        'cone_sharp': subdivided('cone',3),
        'capsule': subdivided('capsule',0),
    }

def write_mesh(path:Path,m:trimesh.Trimesh):
    with path.open('w') as f:
        f.write(f'{len(m.vertices)} {len(m.faces)}\n')
        for x,y,z in m.vertices: f.write(f'v {x:.12g} {y:.12g} {z:.12g}\n')
        for a,b,c in m.faces: f.write(f'f {a+1} {b+1} {c+1}\n')

def read_mesh(path:Path):
    with path.open() as f:
        nv,nf=map(int,f.readline().split()); v=np.empty((nv,3)); faces=np.empty((nf,3),dtype=np.int64)
        for i in range(nv):
            q=f.readline().split(); v[i]=list(map(float,q[1:4]))
        for i in range(nf):
            q=f.readline().split(); faces[i]=np.array(list(map(int,q[1:4])))-1
    return trimesh.Trimesh(v,faces,process=False)

def surface_points(m:trimesh.Trimesh,n=18000,seed=123):
    rng=np.random.default_rng(seed)
    tri=m.vertices[m.faces]
    area=np.linalg.norm(np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]),axis=1)*.5
    prob=area/area.sum()
    ids=rng.choice(len(tri),size=n,p=prob)
    r1=np.sqrt(rng.random(n)); r2=rng.random(n)
    pts=(1-r1)[:,None]*tri[ids,0]+(r1*(1-r2))[:,None]*tri[ids,1]+(r1*r2)[:,None]*tri[ids,2]
    cent=tri.mean(axis=1)
    return np.vstack([m.vertices,cent[::max(1,len(cent)//4000)],pts])

def approx_hausdorff(a,b):
    pa=surface_points(a,seed=111); pb=surface_points(b,seed=222)
    da=cKDTree(pb).query(pa,k=1,workers=-1)[0].max(); db=cKDTree(pa).query(pb,k=1,workers=-1)[0].max()
    diag=np.linalg.norm(a.bounds[1]-a.bounds[0])
    return max(da,db)/diag if diag else 0

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--solvers',nargs='+',required=True,help='name=executable');ap.add_argument('--out',default='benchmark_out');ap.add_argument('--eval',default='/mnt/data/imc_proxy_eval');ap.add_argument('--resolution',type=int,default=256);ap.add_argument('--timeout',type=float,default=28);ap.add_argument('--only',nargs='*');ap.add_argument('--skip-hausdorff',action='store_true');args=ap.parse_args()
    out=Path(args.out); meshes=out/'meshes'; results=out/'results';meshes.mkdir(parents=True,exist_ok=True);results.mkdir(exist_ok=True)
    solvers=dict(s.split('=',1) for s in args.solvers)
    rows=[]
    for name,m in shape_set().items():
        if args.only and name not in args.only: continue
        src=meshes/f'{name}.mesh';write_mesh(src,m)
        print(f'[{name}] V={len(m.vertices)} F={len(m.faces)}',flush=True)
        for sn,exe in solvers.items():
            dst=results/f'{name}__{sn}.mesh'; t=time.perf_counter(); status='ok'
            try:
                with src.open('rb') as fi,dst.open('wb') as fo:
                    cp=subprocess.run([exe],stdin=fi,stdout=fo,stderr=subprocess.PIPE,timeout=args.timeout)
                if cp.returncode!=0: status=f'exit{cp.returncode}'
            except subprocess.TimeoutExpired: status='timeout'
            runtime=time.perf_counter()-t
            row={'shape':name,'solver':sn,'input_v':len(m.vertices),'input_f':len(m.faces),'runtime':runtime,'status':status}
            if status=='ok':
                try:
                    ev=json.loads(subprocess.check_output([args.eval,str(src),str(dst),str(args.resolution)],text=True))
                    row.update(ev)
                    cand=read_mesh(dst); row['hausdorff_sample_frac']=0.0 if args.skip_hausdorff else approx_hausdorff(m,cand)
                    row['compression']=1-ev['vertices']/len(m.vertices)
                except Exception as e: row['status']='eval_error';row['error']=repr(e)
            rows.append(row); print(' ',sn,row['status'],row.get('vertices'),f"score={row.get('final',float('nan')):.5f}",flush=True)
    keys=sorted({k for r in rows for k in r})
    with (out/'results.csv').open('w',newline='') as f:w=csv.DictWriter(f,fieldnames=keys);w.writeheader();w.writerows(rows)
    with (out/'results.json').open('w') as f:json.dump(rows,f,indent=2)
    # markdown summary
    with (out/'REPORT.md').open('w') as f:
        f.write('# IMC multi-shape benchmark\n\n')
        f.write(f'Render proxy resolution: {args.resolution}. Sampled Hausdorff is approximate.\n\n')
        f.write('| Shape | Solver | V in | V out | Compression | Final | Min normal | Min depth | H/sample diag | Manifold | Time |\n|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|\n')
        for r in rows:
            f.write(f"| {r['shape']} | {r['solver']} | {r['input_v']} | {r.get('vertices','-')} | {r.get('compression',0):.3%} | {r.get('final',0):.5f} | {r.get('min_normal',0):.5f} | {r.get('min_depth',0):.5f} | {r.get('hausdorff_sample_frac',0):.4f} | {r.get('manifold','-')} | {r['runtime']:.2f}s |\n")
    print(out/'REPORT.md')
if __name__=='__main__':main()
