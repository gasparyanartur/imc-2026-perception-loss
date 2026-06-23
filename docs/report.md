# Report: Perception-Aware Lossless Simplification of Million-Vertex 3D Meshes for Mobile Platforms

## Executive Summary

The released Huawei IMC mesh-simplification challenge is a constrained optimization problem over triangle meshes. The target is not generic file compression, not neural rendering, and not ordinary geometry simplification. The submitted output must be a closed watertight triangular 2-manifold with non-degenerate faces, must satisfy a symmetric Hausdorff-distance constraint, must achieve a foreground-only multi-view SSIM score of at least 0.9, and is then ranked purely by vertex-count reduction.

The practical implication is direct:

$$
\min_{M'} |V'|
\quad\text{s.t.}\quad
\mathrm{FinalSSIM}(M,M') \ge 0.9,
\quad
d_H(M,M') \le 0.05\cdot \mathrm{Diagonal},
\quad
M'\in\mathcal{M}_{\mathrm{closed}}.
$$

The best starting point is an edge-collapse simplification engine, using the review paper by Kulkarni and Narayanan (2025) as the conceptual and mathematical base. Their review is useful because it derives the standard edge-collapse loop, half-edge/corner-table connectivity requirements, QEM, Lindstrom--Turk-style constraints, validity checks, and fallback vertex-placement strategies. However, the competition-specific evaluator changes the priorities: we should optimize for six fixed axial cameras, flat face-normal maps, depth maps, foreground-only SSIM, and a hard manifold/Hausdorff validity gate.

The most promising near-term solution is therefore:

$$
\boxed{
\text{compact C++ edge-collapse solver}
+ \text{QEM}
+ \text{planar/coplanar collapse pass}
+ \text{view-aware normal/depth penalties}
+ \text{strict topology and degeneracy filters}
+ \text{offline evaluator clone}
}
$$

The Python baseline supplied by the competition is only an input/output scaffold. It reads the modified OBJ-like format into lists of vertices and faces, leaves `simplify()` empty, and writes the same mesh back out. It is useful for understanding the format, but not competitive for the largest cases under the 21 second / 2GB constraints.

---

## 1. Introduction to Mesh Simplification

### 1.1 What mesh simplification is

A triangle mesh is a surface representation

$$
M=(V,F),
$$

where

$$
V=\{v_i\in\mathbb{R}^3\}_{i=1}^{n},
\qquad
F\subset V^3.
$$

Mesh simplification reduces the number of vertices, edges, and faces while preserving the geometric and visual properties needed by the downstream application. In real-time graphics, this is used for level-of-detail systems, mobile rendering, collision proxies, simulation pre-processing, digital twins, and transmission/storage reduction.

Kulkarni and Narayanan (2025) define mesh simplification as reducing vertices, edges, and triangles while preserving overall shape and salient features. They emphasize that edge collapse is one of the most widely adopted practical methods: an edge connecting two vertices is merged into a single vertex, with the collapse selected by a cost function estimating introduced error.

### 1.2 Main families of simplification algorithms

A useful taxonomy, following Cignoni et al. (1998) and the Kulkarni--Narayanan review, is:

1. **Coplanar patch merging / polygonal decimation**  
   Detect near-planar regions, merge them into larger regions, and retriangulate. Strong for CAD-like meshes with large planar faces. Risk: topology mistakes and poor behavior on curved surfaces.

2. **Vertex clustering**  
   Partition space into cells and replace all vertices in each cell by a representative:

   $$
   v_C = \frac{1}{|C|}\sum_{v_i\in C}v_i.
   $$

   Fast but blunt. It can easily damage topology, silhouettes, and surface detail.

3. **Iterative local decimation / edge collapse**  
   Repeatedly collapse locally cheap edges while checking validity. This is the dominant practical direction and the most relevant one for this competition.

4. **Error-bounded simplification envelopes**  
   Maintain a guaranteed geometric deviation bound, e.g.

   $$
   M' \subseteq \{x : d(x,M)\le \epsilon\}.
   $$

   Strong theoretically, but implementation is heavier.

5. **Energy-based optimization / progressive meshes**  
   Define a global or semi-global energy and simplify by local operations such as collapse, split, or swap. Hoppe's progressive meshes belong here.

6. **Voxelization / implicit reconstruction**  
   Convert geometry into a volumetric representation and re-extract a lower-resolution surface. Usually poor for preserving exact topology and sharp CAD-like features.

7. **Neural simplification / neural LOD**  
   Useful in research settings, but not a natural fit for this challenge because the evaluator and input/output constraints are fully classical: vertices, faces, manifoldness, Hausdorff, depth maps, normal maps, and SSIM.

### 1.3 Why edge collapse is the base algorithm

An edge collapse takes an edge

$$
e=(v_i,v_j)
$$

and replaces it by a new vertex

$$
v' = \phi(v_i,v_j,\operatorname{star}(e)).
$$

The two incident triangles are removed, neighboring faces are rewired, and local costs are updated. The generic loop is:

```text
for each edge e:
    compute placement v'(e)
    compute cost C(e)
    push (C(e), e) into priority queue

while not stopped:
    pop cheapest edge e
    if e is stale: continue
    recompute / validate e
    if invalid: reject
    else:
        collapse e
        update local connectivity
        update local costs
```

This matches both the Kulkarni--Narayanan review and CGAL's surface simplification documentation. CGAL describes the method as iterative edge collapse on oriented 2-manifold surfaces, using a cost function to select collapses and a placement function to choose the replacement vertex. CGAL also notes that arbitrary vertex-pair contraction can create non-manifold surfaces; edge-only collapse is safer for manifold preservation.

---

## 2. Problem Statement: The Challenge

### 2.1 Input and output

The challenge input is a modified OBJ-like triangle mesh format:

```text
V F
v x y z
...
f i j k
...
```

The input mesh is guaranteed to be:

- connected;
- closed and watertight;
- a triangular 2-manifold;
- non-degenerate;
- duplicate-free;
- centered at the origin;
- scaled so all vertices lie in the unit sphere;
- bounded by coordinates $[-1,1]$.

The output must use the same format and must satisfy:

$$
1 \le |V'| \le |V|.
$$

The output must also be a closed watertight triangular 2-manifold. In the problem statement, every edge must be shared by exactly two triangular faces. All output faces must have positive area.

The supplied `baseline.py` merely parses the input, stores vertices as Python tuples, stores faces as zero-indexed tuples, does nothing in `simplify()`, and writes the same mesh back out using `%.10g` coordinate formatting. It is an I/O reference, not a solver.

### 2.2 Evaluation camera model

The evaluator renders the original mesh $M$ and simplified mesh $M'$ from six axial views:

$$
E_k \in \{(\pm D,0,0),(0,\pm D,0),(0,0,\pm D)\},
\qquad
D=2.5.
$$

The model is already centered and normalized; no additional scaling or recentering is applied.

Resolution:

$$
1024\times 1024.
$$

Principal point:

$$
(c_u,c_v)=(512,512).
$$

Focal length:

$$
f_x=f_y=800.
$$

Projection formula:

$$
u=f_x\frac{x}{z}+c_u,
\qquad
v=f_y\frac{y}{z}+c_v.
$$

Each pixel is sampled once at its center:

$$
(u+0.5,v+0.5).
$$

The nearest projected triangle covering that point determines the pixel value.

### 2.3 Normal map

The evaluator uses flat shading. Each triangle has one unit face normal:

$$
n_f = \frac{(v_j-v_i)\times(v_k-v_i)}{\|(v_j-v_i)\times(v_k-v_i)\|}.
$$

Every pixel covered by that face receives the same normal. There is no vertex-normal interpolation.

Normal-to-RGB mapping:

$$
I_N(p)=(n_f+[1,1,1]^T)\cdot 127.5.
$$

Background normal:

$$
n_b=(0,0,0),
$$

which maps to neutral gray:

$$
(127.5,127.5,127.5).
$$

### 2.4 Depth map

Depth is interpolated perspective-correctly. Given barycentric weights $w_0,w_1,w_2$ and camera-space vertex depths $z_0,z_1,z_2$, the depth at pixel $P$ is:

$$
z_P =
\frac{1}{\frac{w_0}{z_0}+\frac{w_1}{z_1}+\frac{w_2}{z_2}}.
$$

Background depth:

$$
z_b=255.
$$

### 2.5 SSIM metric

For two images $X,Y$, SSIM is:

$$
\operatorname{SSIM}(X,Y)=
\frac{(2\mu_X\mu_Y+c_1)(2\sigma_{XY}+c_2)}
{(\mu_X^2+\mu_Y^2+c_1)(\sigma_X^2+\sigma_Y^2+c_2)}.
$$

Constants:

$$
c_1=(k_1L)^2,
\qquad
c_2=(k_2L)^2,
$$

with

$$
k_1=0.01,
\qquad
k_2=0.03,
\qquad
L=255.
$$

The local sliding window is $11\times 11$. The final image SSIM is averaged over foreground-only windows: a window is included if the original and/or simplified rendering is non-background at the window center. Common-background regions are excluded. For RGB normal maps, SSIM is computed per channel and averaged.

The final score is:

$$
\mathrm{FinalSSIM}=\frac{1}{6}\sum_{i=1}^{6}
\left(
0.5\cdot \mathrm{SSIM}(I_{N,i},I'_{N,i})
+
0.5\cdot \mathrm{SSIM}(I_{D,i},I'_{D,i})
\right).
$$

Validity threshold:

$$
\mathrm{FinalSSIM}\ge 0.9.
$$

### 2.6 Hausdorff constraint

The challenge imposes symmetric Hausdorff distance:

$$
d_H(M,M')=\max\left(\vec d(M,M'),\vec d(M',M)\right),
$$

where

$$
\vec d(A,B)=\max_{a\in A}\min_{b\in B}\|a-b\|.
$$

The bound is:

$$
d_H(M,M')\le 0.05\cdot \mathrm{Diagonal}.
$$

The AABB diagonal is:

$$
\mathrm{Diagonal}=\sqrt{L_x^2+L_y^2+L_z^2},
$$

with

$$
L_x=x_{\max}-x_{\min},
\quad
L_y=y_{\max}-y_{\min},
\quad
L_z=z_{\max}-z_{\min}.
$$

Since the input is normalized into the unit sphere and centered, this bound is not huge, but it is loose enough to permit aggressive simplification in visually insignificant regions.

### 2.7 Ranking objective

Only valid submissions are ranked. Compression rate is:

$$
\mathrm{CompressionRate}=100-100\frac{|V'|}{|V|}.
$$

Thus the entire challenge reduces to:

$$
\boxed{
\text{minimize vertices while barely satisfying SSIM, Hausdorff, and manifold constraints.}
}
$$

This means a solution with slightly worse visual fidelity but fewer vertices wins, as long as it stays above $0.9$ SSIM and passes all validity checks.

### 2.8 Computational constraints

Global constraints:

$$
|V|\le 1.1\cdot 10^6,
\qquad
|F|\le 2.1\cdot 10^6.
$$

Time and memory:

$$
21\text{ seconds},
\qquad
2048\text{ MB}.
$$

This strongly favors C++ with compact arrays. The problem statement says Eigen is available for C++, which is useful for small $3\times 3$ linear solves in QEM and Lindstrom--Turk-style placement.

---

## 3. Theoretical Toolbox

### 3.1 Triangle mesh topology

A triangle mesh can be treated as a 2-dimensional simplicial complex embedded in $\mathbb{R}^3$. Each face is a 2-simplex, each edge is a 1-simplex, and each vertex is a 0-simplex.

For a closed triangular 2-manifold:

- every edge has exactly two incident faces;
- the one-ring around each vertex is topologically a cycle;
- the surface is locally disk-like around every vertex.

A cheap global topology invariant is the Euler characteristic:

$$
\chi = |V|-|E|+|F|.
$$

For a connected closed orientable surface of genus $g$:

$$
\chi=2-2g.
$$

The challenge does not explicitly require preserving genus, but preserving a closed watertight 2-manifold while simplifying by valid edge collapses should avoid unexpected topology changes. In practice, $\chi$ is an excellent debugging signal.

### 3.2 Edge-collapse topology: the link condition

For vertex $v$, define:

$$
\operatorname{Star}(v)=\{\sigma\in K: v\subseteq \sigma\},
$$

$$
\operatorname{Link}(v)=\{\tau\in K: \tau\cap v=\emptyset,\ \tau\cup v\in K\}.
$$

The link is the local ring around a vertex. For an interior vertex of a 2-manifold, the link should be a topological circle.

For an edge collapse $ab\rightarrow c$, a topology-preserving contraction must not create non-manifold connectivity. The formal criterion is the link condition, associated with Dey, Edelsbrunner, Guha, and Nekhayev:

$$
\operatorname{Link}(ab)=\operatorname{Link}(a)\cap \operatorname{Link}(b),
$$

with boundary-aware variants when boundaries exist. Since this challenge requires closed meshes, the practical closed-manifold condition becomes:

$$
|N(a)\cap N(b)|=2.
$$

That is: the two endpoints of a collapsible edge should share exactly the two opposite vertices of the two incident triangles. This matches the two-neighbor validity check emphasized in the Kulkarni--Narayanan review.

### 3.3 Face normals, areas, and degeneracy

For face $f=(i,j,k)$:

$$
\tilde n_f=(v_j-v_i)\times(v_k-v_i).
$$

Area:

$$
A_f=\frac12\|\tilde n_f\|.
$$

Unit normal:

$$
n_f=\frac{\tilde n_f}{\|\tilde n_f\|}.
$$

Degenerate face condition:

$$
A_f=0.
$$

In code, reject if:

$$
A_f<\epsilon_A.
$$

Because the evaluator uses flat face normals, face orientation and face normal stability matter directly. A normal flip can cause catastrophic normal-map error even if the geometric position remains close.

Normal flip check:

$$
n_f^{\mathrm{old}}\cdot n_f^{\mathrm{new}} > \cos \theta_{\max}.
$$

A conservative first threshold is:

$$
\theta_{\max}\in[60^\circ,90^\circ],
$$

with stricter values in high-visibility regions.

### 3.4 Triangle quality

Skinny triangles can lead to numerical and rasterization instability. A common normalized quality metric is:

$$
q(f)=\frac{4\sqrt{3}A_f}{\|e_1\|^2+\|e_2\|^2+\|e_3\|^2}.
$$

For an equilateral triangle:

$$
q=1.
$$

For a degenerate triangle:

$$
q\rightarrow 0.
$$

Reject a collapse if any affected face has:

$$
q(f)<q_{\min}.
$$

The review also derives a triangle-shape optimization objective:

$$
\varepsilon(v)=\sum_{i=1}^{n}\|v-v_i\|^2,
$$

whose minimizer is the neighbor centroid:

$$
v^\star=\frac1n\sum_{i=1}^{n}v_i.
$$

This is useful as a weak regularizer or fallback, but not as the main simplification objective.

### 3.5 Quadric Error Metrics (QEM)

QEM is the central simplification cost.

A plane is represented by unit normal $n$ and offset $d$:

$$
n^T x+d=0.
$$

The squared signed distance from point $v$ to the plane is:

$$
(n^Tv+d)^2.
$$

For a set of local planes $\mathcal{P}$, the QEM error is:

$$
\varepsilon(v)=\sum_{(n,d)\in\mathcal P}(n^Tv+d)^2.
$$

Expanding:

$$
\varepsilon(v)
=\sum_{(n,d)\in\mathcal P}\left(v^Tnn^Tv+2dn^Tv+d^2\right).
$$

So:

$$
\varepsilon(v)=v^T H v + 2c^Tv+k,
$$

where

$$
H=\sum_{(n,d)\in\mathcal P}nn^T,
$$

$$
c=\sum_{(n,d)\in\mathcal P}dn,
$$

$$
k=\sum_{(n,d)\in\mathcal P}d^2.
$$

Set the gradient to zero:

$$
\nabla \varepsilon(v)=2Hv+2c=0.
$$

Thus:

$$
v^\star=-H^{-1}c,
$$

if $H$ is invertible.

In homogeneous coordinates:

$$
\bar v=\begin{bmatrix}v\\1\end{bmatrix},
\qquad
Q=\begin{bmatrix}H & c\\ c^T & k\end{bmatrix},
$$

and

$$
\varepsilon(v)=\bar v^T Q \bar v.
$$

For edge $e=(i,j)$, combine endpoint quadrics:

$$
Q_e=Q_i+Q_j.
$$

The collapse cost is:

$$
C_{\mathrm{QEM}}(e)=\min_v \bar v^T Q_e\bar v.
$$

If $H$ is singular, which often happens in planar regions, use candidate fallback points:

$$
v'\in\left\{v_i, v_j, \frac{v_i+v_j}{2}\right\}.
$$

For this competition, endpoint candidates are especially attractive because they simplify Hausdorff tracking.

### 3.6 Planar-patch invariance theorem

This is one of the most competition-specific insights.

Suppose a connected patch $P$ lies exactly on a plane:

$$
\pi: n^Tx+d=0.
$$

If we replace the internal triangulation of $P$ with another triangulation that preserves:

- the same boundary;
- the same orientation;
- the same plane;
- the same visibility/occlusion ordering;

then both the normal map and depth map remain unchanged for pixels covered by the patch.

Reason:

1. Flat normal is identical:

   $$
   n_f'=n_f=n.
   $$

2. The projected planar surface is unchanged.

3. Perspective-correct depth over a planar triangle is consistent because $1/z$ is affine in screen space over the projected triangle.

Therefore:

$$
I_N(P)=I_N(P'),
\qquad
I_D(P)=I_D(P').
$$

So local SSIM remains 1, up to rasterization boundary tie-breaking and floating-point effects.

This is why planar/coplanar simplification is disproportionately valuable for this challenge. It can remove vertices without changing the evaluator's rendered features.

### 3.7 Lindstrom--Turk-style constraints

Lindstrom--Turk simplification is a memory-efficient alternative/complement to QEM. The review derives several local quadratic constraints that can be written as:

$$
\varepsilon(v)=v^THv+2c^Tv+k.
$$

Important components:

#### Volume preservation

The signed volume of a closed triangular mesh can be computed by summing tetrahedral volumes:

$$
V(M)=\frac16\sum_{(i,j,k)\in F}v_i\cdot(v_j\times v_k).
$$

A collapse should avoid large local volume change because the depth maps encode morphology and occlusion. A local volume penalty can be added:

$$
C_{\mathrm{vol}}=|\Delta V|,
$$

or squared:

$$
C_{\mathrm{vol}}=(\Delta V)^2.
$$

#### Volume optimization

The review derives a squared-volume objective:

$$
\varepsilon(v)=\sum_{t\in T}V(v,v^t_1,v^t_2,v^t_3)^2,
$$

which again becomes quadratic in $v$:

$$
\varepsilon(v)=v^THv+2c^Tv+k.
$$

This can be used as a secondary cost or placement constraint.

#### Shape optimization

The centroid objective above improves triangle shape. It is cheap and useful for fallback placement.

### 3.8 Hausdorff and cluster-radius proxy

Exact symmetric Hausdorff distance between large triangle meshes is too expensive inside the solver. We need a conservative proxy.

If we only collapse to existing original vertices, then every output vertex is an original vertex. This helps with the direction:

$$
\vec d(M',M).
$$

To track the other direction, maintain a cluster radius for each surviving vertex:

$$
C_s = \{\text{original vertices represented by survivor }s\}.
$$

If survivor position is $p_s$, define:

$$
r_s=\max_{x\in C_s}\|x-p_s\|.
$$

When collapsing clusters $a,b\rightarrow c$:

$$
r_c=\max\left(r_a+\|p_a-p_c\|,\ r_b+\|p_b-p_c\|\right).
$$

Reject if:

$$
r_c>0.05\cdot \mathrm{Diagonal}.
$$

This is conservative and vertex-based, not a full surface Hausdorff guarantee. But it is cheap and aligned with the problem statement's practical interpretation that original vertices must remain covered and simplified vertices must not stray far.

### 3.9 Rasterization and barycentric interpolation

For a projected triangle with screen vertices $p_0,p_1,p_2$, a pixel center $P$ inside the triangle satisfies:

$$
P=w_0p_0+w_1p_1+w_2p_2,
$$

$$
w_0+w_1+w_2=1,
\qquad
w_i\ge 0.
$$

The evaluator uses the nearest triangle at that pixel. This is exactly the rasterization abstraction used in graphics libraries: per pixel, identify the nearest face, compute barycentric coordinates, and interpolate attributes. PyTorch3D's rasterizer documentation describes outputs such as nearest face indices (`pix_to_face`), z-buffer values (`zbuf`), and barycentric coordinates (`bary_coords`), which are the same conceptual quantities needed for an offline evaluator.

For IMC, the only interpolated attribute is depth, and it is perspective-correct:

$$
z_P=\frac{1}{\frac{w_0}{z_0}+\frac{w_1}{z_1}+\frac{w_2}{z_2}}.
$$

Normals are not interpolated.

### 3.10 SSIM behavior and implications

SSIM is local-window based. It compares local means, variances, and covariance. For this competition, the decisive details are:

- foreground-only averaging removes the large easy background region;
- silhouette errors create foreground/background disagreement and hurt depth and normal maps;
- flat-normal differences are severe because every triangle has a constant normal color;
- large triangles on curved surfaces produce blocky normal maps;
- depth errors are most visible near silhouettes and occlusion boundaries.

Therefore, an edge-collapse cost should not only approximate surface distance. It should protect:

$$
\text{silhouettes},\quad
\text{high-normal-change regions},\quad
\text{large projected-area regions},\quad
\text{occlusion-critical edges}.
$$

### 3.11 View-aware projected area

The six camera directions are known. For a face $f$ and view direction $d_k$, a rough visibility/projection weight is:

$$
w_f^{(k)}\approx
A_f\cdot |n_f^T d_k|\cdot \frac{f_xf_y}{z_f^2}.
$$

Total face importance:

$$
w_f=1+\lambda_{\mathrm{view}}\sum_{k=1}^{6}w_f^{(k)}.
$$

Weighted QEM:

$$
Q_i=\sum_{f\sim i}w_fK_f.
$$

This biases simplification away from highly visible large projected faces.

### 3.12 Silhouette proxy

For adjacent faces $f,g$, an edge is silhouette-like under view $k$ if the adjacent faces face opposite relative to the camera:

$$
(n_f^Td_k)(n_g^Td_k)<0.
$$

A collapse touching such an edge should be expensive:

$$
C_{\mathrm{sil}}(e)=
\sum_{k=1}^{6}\mathbf{1}\left[(n_f^Td_k)(n_g^Td_k)<0\right].
$$

In practice:

$$
C(e)\leftarrow C(e)+\lambda_{\mathrm{sil}}C_{\mathrm{sil}}(e).
$$

Or, in an early conservative solver, forbid collapses on strong silhouettes.

### 3.13 Candidate combined collapse cost

A competition-specific edge cost can be:

$$
C(e)=
C_{\mathrm{QEM}}
+\lambda_N C_{\mathrm{normal}}
+\lambda_D C_{\mathrm{depth}}
+\lambda_S C_{\mathrm{silhouette}}
+\lambda_H C_{\mathrm{hausdorff}}
+\lambda_Q C_{\mathrm{quality}}
+\lambda_P C_{\mathrm{planarity}}.
$$

Where:

$$
C_{\mathrm{normal}}=\sum_{f\in \operatorname{star}(e)}w_f(1-n_f^{old}\cdot n_f^{new}),
$$

$$
C_{\mathrm{hausdorff}}=r_{\mathrm{new}},
$$

$$
C_{\mathrm{quality}}=\sum_{f\in\operatorname{star}(e)}\max(0,q_{\min}-q(f))^2,
$$

$$
C_{\mathrm{planarity}}=1-|n_{f_1}^Tn_{f_2}|.
$$

The initial implementation should avoid overcomplication. Start with QEM + hard validity filters + cluster-radius guard + normal flip guard, then add view/silhouette weighting.

---

## 4. Candidate Solutions

### Solution 0: No-op baseline

Return the input mesh unchanged.

Pros:

- guaranteed valid;
- useful for validating I/O;
- should pass SSIM and Hausdorff trivially.

Cons:

- vertex reduction is zero;
- score is zero or near zero.

Use only as a sanity check.

### Solution 1: Planar redundant-vertex removal

This solution targets exact or near-exact simplification of planar/coplanar regions.

#### Idea

Adjacent faces are considered coplanar if:

$$
1-|n_f^Tn_g|<\epsilon_n,
$$

and

$$
|d_f-d_g|<\epsilon_d.
$$

Within such regions, collapse internal edges first. If the boundary and plane are preserved, normal/depth maps are almost unchanged.

#### Algorithm

```text
compute face normals and plane offsets
mark edges whose adjacent faces are near-coplanar
push such edges into priority queue with very low cost
while queue not empty:
    pop edge
    if edge still coplanar and valid:
        collapse using endpoint/midpoint
        update local faces/normals/edges
```

#### Cost

$$
C_{\mathrm{plane}}(e)=
\alpha(1-|n_1^Tn_2|)+
\beta|d_1-d_2|+
\gamma r_{\mathrm{new}}.
$$

#### Pros

- very aligned with evaluator;
- can preserve SSIM almost exactly on planar surfaces;
- strong for CAD/digital-twin models;
- simpler than full render-aware optimization.

#### Cons

- weak on smooth organic shapes;
- local edge collapse inside a planar region may still produce poor triangles;
- full patch extraction + retriangulation is more complex than edge collapse.

#### Recommendation

Implement as the first real simplification pass. It will likely give quick gains and produce a safe base mesh before generic QEM.

---

### Solution 2: Endpoint-only QEM edge collapse

#### Idea

Use QEM to rank collapses, but restrict placement to:

$$
v'\in\{v_i,v_j\}.
$$

Optionally include midpoint:

$$
v'=\frac{v_i+v_j}{2}.
$$

Endpoint-only collapse simplifies Hausdorff tracking and avoids introducing new vertices that may drift away from the original surface.

#### Cost

For each candidate $v'$:

$$
C(v')=\bar v'^T(Q_i+Q_j)\bar v'.
$$

Pick:

$$
v^*=\arg\min_{v'\in\{v_i,v_j\}}C(v').
$$

Reject if topology, degeneracy, normal flip, or cluster-radius checks fail.

#### Pros

- robust;
- memory-efficient;
- easier to implement;
- safer for manifoldness and Hausdorff;
- good first competitive submission.

#### Cons

- less accurate than optimal QEM placement;
- may leave more vertices than necessary;
- can create poor local triangle shapes without additional checks.

#### Recommendation

This should be the first full solver after planar collapse.

---

### Solution 3: Full QEM with fallback placement

#### Idea

Use the QEM optimum:

$$
v^*=-H^{-1}c.
$$

If singular or invalid, evaluate fallback candidates:

$$
\left\{v_i,v_j,\frac{v_i+v_j}{2}\right\}.
$$

#### Pros

- better geometric approximation;
- fewer vertices may be possible under the same SSIM;
- standard method with strong literature support.

#### Cons

- harder Hausdorff tracking;
- can move vertices off-surface;
- can damage depth/silhouette if not constrained;
- needs robust placement filters.

#### Recommendation

Implement after endpoint-only QEM works. Use only when local normal/depth/silhouette proxy says it is safe.

---

### Solution 4: View-aware QEM

#### Idea

Modify QEM weights using the known six-view evaluator.

For each face:

$$
w_f=1+\lambda_{\mathrm{view}}\sum_{k=1}^{6}
A_f |n_f^Td_k|\frac{f_xf_y}{z_{f,k}^2}.
$$

Then:

$$
Q_i=\sum_{f\sim i}w_fK_f.
$$

Add silhouette penalty:

$$
C(e)=C_{\mathrm{QEM}}+\lambda_{\mathrm{sil}}C_{\mathrm{sil}}.
$$

#### Pros

- directly aligns with fixed evaluator;
- protects visible regions and silhouettes;
- likely improves SSIM at same vertex budget.

#### Cons

- needs careful tuning;
- approximate visibility can be wrong without z-buffering;
- too conservative if weights are excessive.

#### Recommendation

High-priority improvement once a local evaluator exists.

---

### Solution 5: Offline evaluator-guided parameter search

#### Idea

Build a local clone of the challenge evaluator:

- six cameras;
- 1024×1024 resolution;
- flat face-normal map;
- depth map;
- foreground-only SSIM;
- same constants.

Then tune simplification parameters offline:

$$
\epsilon_n,
\epsilon_d,
q_{\min},
\theta_{\max},
\lambda_N,
\lambda_S,
\lambda_H,
\lambda_Q.
$$

Use grid search or Bayesian optimization over constants on sample/private meshes. The submitted solver then hardcodes good constants.

#### Pros

- essential for avoiding blind submissions;
- directly optimizes the hidden threshold behavior;
- helps identify whether normal or depth loss dominates.

#### Cons

- the exact evaluator may have subtle rasterization differences;
- expensive at 1024² × 6 views;
- not feasible inside submitted code under 21 seconds.

#### Recommendation

Mandatory. Build this in Python/C++ outside Kattis. Do not put full SSIM rendering inside the submitted solver initially.

---

### Solution 6: Patch extraction and retriangulation

#### Idea

Instead of only collapsing edges, extract maximal near-planar connected face patches, keep patch boundaries, and retriangulate each patch with fewer vertices.

#### Pros

- potentially much stronger than local edge collapse on planar CAD-like regions;
- can remove most internal vertices from planar surfaces;
- evaluator may see no difference if boundaries and planes are preserved.

#### Cons

- hard to implement robustly under contest time;
- triangulating arbitrary polygonal patches with holes is non-trivial;
- must preserve manifold connectivity exactly;
- output triangle quality must be controlled.

#### Recommendation

Consider as a second-stage optimization if test meshes are strongly CAD-like. Use local planar edge collapse first because it is simpler and safer.

---

### Solution 7: Vertex clustering / voxel simplification

#### Idea

Cluster spatially close vertices and replace by representatives.

#### Pros

- very fast;
- easy to implement;
- useful for emergency low-effort simplification.

#### Cons

- likely breaks manifoldness;
- poor silhouette preservation;
- poor normal-map preservation;
- weak Hausdorff guarantees;
- bad for sharp features.

#### Recommendation

Not a primary solution. Possibly useful as an offline diagnostic or for tiny, very smooth meshes only.

---

### Solution 8: Neural or learned simplification

#### Idea

Use a trained model to predict important vertices or collapse priorities.

#### Pros

- interesting research direction;
- could learn saliency if many training pairs existed.

#### Cons

- no training distribution guaranteed;
- strict manifold output is hard;
- C++ contest runtime is hostile to neural inference;
- evaluator is explicit and classical;
- the largest gains likely come from classical geometry.

#### Recommendation

Do not prioritize.

---

## 5. Preliminary Roadmap

### Phase 0: Shared understanding and invariants

Deliverables:

- read the problem statement carefully;
- read Kulkarni--Narayanan Sections 3--7 as the implementation base;
- agree that the primary solver is C++;
- treat Python as offline tooling only.

Core invariants:

$$
\forall e: \operatorname{incidence}(e)=2,
$$

$$
\forall f: A_f>\epsilon_A,
$$

$$
|V'|\le |V|,
$$

$$
r_{\mathrm{cluster}}\le 0.05\cdot\mathrm{Diagonal}.
$$

### Phase 1: I/O and validation

Implement in C++:

1. fast parser for modified OBJ format;
2. compact vertex and face arrays;
3. edge hashing:

   $$
   e=(\min(i,j),\max(i,j));
   $$

4. face area computation;
5. face normal computation;
6. edge incidence validation;
7. Euler characteristic computation;
8. output writer with controlled precision.

The Python baseline shows the expected format and uses 10 significant digits; C++ output should avoid excessive decimals because the problem has a 100 MiB output limit.

### Phase 2: Offline evaluator clone

Implement local evaluator before serious tuning.

Minimum evaluator:

```text
for each of six camera directions:
    transform vertices into camera coordinates
    project triangles
    rasterize nearest triangle per pixel
    write flat normal map
    write perspective-correct depth map
    compute foreground-only SSIM
average six views
```

Start with a lower resolution, e.g. 256 or 512, for speed. Then verify at 1024.

Key functions:

$$
I_N(p)=(n_f+[1,1,1])\cdot127.5,
$$

$$
z_P=\frac{1}{w_0/z_0+w_1/z_1+w_2/z_2},
$$

$$
\mathrm{SSIM}(X,Y)=
\frac{(2\mu_X\mu_Y+c_1)(2\sigma_{XY}+c_2)}
{(\mu_X^2+\mu_Y^2+c_1)(\sigma_X^2+\sigma_Y^2+c_2)}.
$$

This evaluator is the difference between guessing and engineering.

### Phase 3: Mesh connectivity

Implement a memory-conscious edge-collapse data structure.

Avoid object-heavy half-edge classes. Use arrays:

```cpp
struct Vec3 { double x, y, z; };
struct Face { int a, b, c; bool alive; };
struct EdgeInfo { int u, v; int f0, f1; };
struct VertexInfo { bool alive; int parent; double radius; Quadric Q; };
```

Need efficient local queries:

$$
N(v),
\qquad
F(v),
\qquad
F(e).
$$

Possible implementation choices:

1. Full mutable adjacency sets. Easier, more memory.
2. Lazy invalidation with periodic rebuilds. Harder, less memory.
3. Hybrid: maintain vertex neighbor sets for alive vertices and rebuild local connectivity after collapse.

For 1.1M vertices and 2.1M faces under 2GB, compactness matters.

### Phase 4: Planar collapse pass

Implement near-coplanar edge detection:

$$
1-|n_f^Tn_g|<\epsilon_n,
$$

$$
|d_f-d_g|<\epsilon_d.
$$

Collapse low-risk planar edges first.

Suggested initial constants:

$$
\epsilon_n\in[10^{-8},10^{-5}],
\qquad
\epsilon_d\in[10^{-8},10^{-5}].
$$

Tune by offline evaluator.

Placement candidates:

$$
\{v_i,v_j,(v_i+v_j)/2\}.
$$

Validity filters:

- common-neighbor count equals 2;
- no degenerate affected faces;
- no normal flips;
- cluster radius below Hausdorff bound;
- triangle quality above threshold.

### Phase 5: Endpoint-only QEM

Implement per-vertex quadrics:

$$
Q_i=\sum_{f\sim i}K_f.
$$

For edge $e=(i,j)$:

$$
Q_e=Q_i+Q_j.
$$

Evaluate endpoint candidates:

$$
C_i=\bar v_i^TQ_e\bar v_i,
\qquad
C_j=\bar v_j^TQ_e\bar v_j.
$$

Pick the lower-cost valid endpoint.

Collapse operation:

1. choose survivor;
2. mark removed vertex dead;
3. remove the two incident faces of collapsed edge;
4. replace removed vertex by survivor in adjacent faces;
5. remove degenerate duplicate faces if any;
6. update survivor quadric:

   $$
   Q_s\leftarrow Q_i+Q_j;
   $$

7. update cluster radius;
8. recompute local face normals and edge costs.

### Phase 6: Full QEM placement

Add optimal placement:

$$
v^*=-H^{-1}c.
$$

Use Eigen for the solve. If determinant or condition number is bad, fallback.

Candidate set:

$$
\mathcal C_e=\left\{v_i,v_j,\frac{v_i+v_j}{2},v^*\right\}.
$$

Pick:

$$
v'=\arg\min_{v\in \mathcal C_e}C(v),
$$

subject to validity checks.

This should be added only after endpoint QEM works reliably.

### Phase 7: View-aware penalties

Add face weights:

$$
w_f=1+\lambda_{\mathrm{view}}\sum_{k=1}^{6}A_f|n_f^Td_k|\frac{f_xf_y}{z_{f,k}^2}.
$$

Add normal-change cost:

$$
C_N=\sum_{f\in\operatorname{star}(e)}w_f(1-n_f^{old}\cdot n_f^{new}).
$$

Add silhouette cost:

$$
C_S=\sum_k\mathbf{1}[(n_f^Td_k)(n_g^Td_k)<0].
$$

Combined:

$$
C(e)=C_{\mathrm{QEM}}+\lambda_NC_N+\lambda_SC_S+\lambda_QC_Q.
$$

Tune constants offline.

### Phase 8: Stopping policy

The solver needs a stop policy without running full SSIM. Options:

1. Stop when edge queue minimum cost exceeds tuned threshold:

   $$
   \min_e C(e)>T_C.
   $$

2. Stop when vertex ratio reaches tuned target:

   $$
   |V'|/|V|\le \rho.
   $$

3. Use staged targets by test size:

   $$
   \rho=\rho(|V|,|F|).
   $$

4. Use a conservative local damage budget:

   $$
   \sum_{\text{collapses}}\Delta C < B.
   $$

Recommended first version: target ratio + safety threshold. Tune ratios offline.

### Phase 9: Testing and debugging checklist

For every output mesh:

$$
|V'|>0,
\qquad
|V'|\le |V|.
$$

Every face index valid.

Every face area positive:

$$
A_f>\epsilon_A.
$$

Every undirected edge has exactly two incident faces.

No duplicate faces.

No self-collapse faces such as:

$$
(i,i,j),\quad(i,j,i),\quad(j,i,i).
$$

Euler characteristic is plausible:

$$
\chi'=|V'|-|E'|+|F'|.
$$

Offline:

$$
\mathrm{FinalSSIM}\ge 0.9.
$$

Offline approximate Hausdorff / cluster-radius:

$$
r_{\max}\le0.05\cdot\mathrm{Diagonal}.
$$

### Phase 10: Division of work

Suggested roles:

1. **Solver core**  
   C++ parser, connectivity, edge collapse, QEM, output.

2. **Evaluator/tools**  
   Offline renderer, SSIM, visualization, parameter sweeps.

3. **Theory/experiments**  
   Planar-patch detection, view-aware costs, threshold tuning, failure analysis.

### Phase 11: Expected progression of submissions

Submission A:

$$
\text{no-op / validator only}
$$

Purpose: confirm format.

Submission B:

$$
\text{planar collapse only}
$$

Purpose: safe first compression.

Submission C:

$$
\text{endpoint-only QEM + validity filters}
$$

Purpose: first real competitive solver.

Submission D:

$$
\text{QEM + planar + view-aware normal/silhouette penalties}
$$

Purpose: strong competition solver.

Submission E:

$$
\text{full QEM placement + tuned thresholds + local evaluator calibration}
$$

Purpose: final optimization.

---

## 6. Key Risks and Mitigations

### Risk 1: Breaking manifoldness

Cause: bad collapse connectivity, duplicate faces, edge incidence errors.

Mitigation:

$$
|N(a)\cap N(b)|=2
$$

before collapse, then validate local edge incidence after collapse.

### Risk 2: Zero-area faces

Cause: collapsing into a neighbor or creating collinear triangles.

Mitigation:

$$
A_f>\epsilon_A
$$

for all affected faces before committing.

### Risk 3: Normal-map damage

Cause: flat normals make large triangle changes visible.

Mitigation:

$$
n_f^{old}\cdot n_f^{new}>\cos\theta_{\max}
$$

and view-weighted normal-change cost.

### Risk 4: Silhouette damage

Cause: collapsing boundary-of-visibility edges.

Mitigation:

protect edges where:

$$
(n_f^Td_k)(n_g^Td_k)<0.
$$

### Risk 5: Hausdorff failure

Cause: optimal QEM placement moves off original surface.

Mitigation:

start endpoint-only; maintain cluster radius; only allow free QEM placement when local proxy says safe.

### Risk 6: Runtime/memory failure

Cause: object-heavy half-edge, excessive priority queue storage, expensive global checks.

Mitigation:

compact arrays, lazy invalidation, local updates, avoid full evaluator in submission, use Eigen only for tiny linear solves.

---

## 7. Recommended First Implementation Specification

### Data structures

```cpp
struct Vec3 {
    double x, y, z;
};

struct Face {
    int a, b, c;
    bool alive;
    Vec3 normal;
    double area;
};

struct Quadric {
    double q[10]; // symmetric 4x4 packed, or H,c,k separately
};

struct Vertex {
    Vec3 p;
    bool alive;
    int version;
    double radius;
    Quadric Q;
};

struct EdgeCandidate {
    double cost;
    int u, v;
    int version_u, version_v;
};
```

### Collapse validity

For edge $(a,b)$:

1. both vertices alive;
2. edge exists and has two incident faces;
3. common neighbor count equals 2;
4. replacement candidate passes cluster-radius bound;
5. all affected faces remain non-degenerate;
6. all affected face normals avoid flips;
7. local duplicate faces are not introduced.

### Candidate placement order

Initial:

$$
\mathcal C=\{v_a,v_b\}.
$$

Then:

$$
\mathcal C=\{v_a,v_b,(v_a+v_b)/2\}.
$$

Final:

$$
\mathcal C=\{v_a,v_b,(v_a+v_b)/2,v^*_{\mathrm{QEM}}\}.
$$

### Priority queue

Use lazy invalidation:

```text
pop edge
if vertex versions changed: discard
else recompute exact local cost
if still valid: collapse
push updated neighboring edges
```

### Output compaction

At the end:

1. assign new indices to alive vertices;
2. emit compact vertex list;
3. emit alive faces with remapped indices;
4. print with enough precision but not excessive decimals.

Suggested:

```cpp
std::setprecision(10)
```

or controlled fixed/scientific formatting after testing.

---

## 8. References and Suggested Reading

### Challenge and supplied material

[IMC-Problem] Huawei IMC Challenge, *Perception-Aware Lossless Simplification of Million-Vertex 3D Meshes for Mobile Platforms*, released problem statement PDF. Key sections: evaluator cameras, feature maps, constraints, SSIM formula, input/output, runtime/memory limits.

[Baseline] Competition starter scaffold, `baseline.py`. Useful for input/output format only; `simplify()` is empty.

[Kulkarni2025] Purva Kulkarni and Aravind Shankara Narayanan. *A Comprehensive Guide to Mesh Simplification using Edge Collapse*. arXiv:2512.19959, 2025. Main base document for this report. Especially relevant sections: data structures, edge-collapse loop, QEM, Lindstrom--Turk constraints, validity checks, fallback strategies, and attribute-aware extensions.

### Foundational papers

[Cignoni1998] Paolo Cignoni, Claudio Montani, and Roberto Scopigno. *A Comparison of Mesh Simplification Algorithms*. Computers & Graphics, 1998. Useful taxonomy of simplification methods.

[Garland1997] Michael Garland and Paul S. Heckbert. *Surface Simplification Using Quadric Error Metrics*. SIGGRAPH 1997. Core QEM paper.

[Garland1998] Michael Garland and Paul S. Heckbert. *Simplifying Surfaces with Color and Texture using Quadric Error Metrics*. IEEE Visualization 1998. Attribute-aware QEM; less directly relevant here because IMC input has no texture/UV/color.

[Lindstrom1998] Peter Lindstrom and Greg Turk. *Fast and Memory Efficient Polygonal Simplification*. IEEE Visualization 1998. Important for volume, boundary, and triangle-shape constraints.

[Hoppe1993] Hugues Hoppe, Tony DeRose, Tom Duchamp, John McDonald, and Werner Stuetzle. *Mesh Optimization*. SIGGRAPH 1993. Energy-based mesh optimization.

[Hoppe1996] Hugues Hoppe. *Progressive Meshes*. SIGGRAPH 1996. Multiresolution mesh representation and edge-collapse sequences.

[Cohen1996] Jonathan Cohen et al. *Simplification Envelopes*. SIGGRAPH 1996. Bounded-deviation simplification.

[Dey1999] Tamal K. Dey, Herbert Edelsbrunner, Sumanta Guha, and Dmitry Nekhayev. *Topology Preserving Edge Contraction*. Publications on link conditions for topology-safe contraction.

[Schroeder1992] William J. Schroeder, Jonathan A. Zarge, and William E. Lorensen. *Decimation of Triangle Meshes*. SIGGRAPH 1992. Early triangle decimation method.

[Rossignac2002] Jarek Rossignac. *Edgebreaker: Connectivity Compression for Triangle Meshes*. IEEE TVCG. Relevant for compact connectivity thinking.

[Wang2004] Zhou Wang, Alan C. Bovik, Hamid R. Sheikh, and Eero P. Simoncelli. *Image Quality Assessment: From Error Visibility to Structural Similarity*. IEEE Transactions on Image Processing, 2004. Original SSIM paper.

### Textbooks and lecture-style references

[Botsch2010] Mario Botsch, Leif Kobbelt, Mark Pauly, Pierre Alliez, and Bruno Lévy. *Polygon Mesh Processing*. AK Peters/CRC Press, 2010. Read chapters/sections on surface representations, discrete differential geometry, remeshing, simplification, and approximation.

[Munkres2000] James R. Munkres. *Topology*, 2nd edition. Read sections on surfaces and Euler characteristic if topology foundations are weak.

[Hatcher2002] Allen Hatcher. *Algebraic Topology*. Read Section 2.2 for Euler characteristic and homology interpretation. Overkill for implementation, useful for rigorous topology background.

[deBerg2008] Mark de Berg, Otfried Cheong, Marc van Kreveld, and Mark Overmars. *Computational Geometry: Algorithms and Applications*. Useful for geometric data structures, arrangements, triangulations, and proximity queries.

### Practical libraries and documentation

[CGAL-SMS] CGAL Surface Mesh Simplification documentation. Important because it clearly describes edge collapse, cost policies, placement policies, Lindstrom--Turk, Garland--Heckbert, manifold restrictions, and memory trade-offs.

[CGAL-PMP] CGAL Polygon Mesh Processing documentation. Useful for mesh repair, degeneracy handling, self-intersection checks, and geometric predicates.

[Open3D-Mesh] Open3D mesh tutorial. Useful for understanding manifold checks, watertightness, orientability, self-intersection, vertex clustering, and quadric decimation.

[libigl] libigl tutorial. Useful for cotangent Laplacian, curvature, mass matrices, and discrete differential geometry operators.

[PyTorch3D] PyTorch3D mesh rasterizer documentation. Useful for offline evaluator concepts: nearest face per pixel, z-buffer, barycentric coordinates.

---

## 9. Final Recommendation

The winning direction is classical geometry processing specialized to the evaluator.

Do not start with neural methods. Do not start with generic vertex clustering. Do not implement every method in the review. Use the review as the base for edge-collapse mechanics, then specialize the cost function and validity checks to the IMC metric.

The first serious solver should be:

$$
\boxed{
\text{planar collapse}
\rightarrow
\text{endpoint-only QEM}
\rightarrow
\text{strict validity filters}
\rightarrow
\text{offline SSIM tuning}
}
$$

The strong final solver should be:

$$
\boxed{
\text{planar/coplanar simplification}
+
\text{QEM with fallback placement}
+
\text{view-aware projected-area weighting}
+
\text{silhouette protection}
+
\text{cluster-radius Hausdorff guard}
+
\text{normal/depth proxy penalties}
}
$$

The central design principle is simple:

$$
\textbf{Remove vertices where the six rendered flat-normal/depth maps do not change.}
$$

Everything else is implementation detail.
