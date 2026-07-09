# Mathematical Formulation

## Mathematical problem statement

$$
M_0=(\mathcal X_0,\mathcal T_0)
$$

$$
\widehat M=(\widehat{\mathcal X},\widehat{\mathcal T})
$$

$$
\boxed{
\begin{aligned}
\max_{\widehat M}
\quad
&
O(M_0,\widehat M)
=
100\left(1-\frac{\widehat N}{N_0}\right)
\\[2mm]
\mathrm{s.t.}
\quad
&
C_1^{\mathrm{vertex}}(\widehat M;M_0)=1
\\
&
C_2^{\mathrm{indices}}(\widehat M)=1
\\
&
C_3^{\mathrm{area}}(\widehat M)=1
\\
&
C_4^{\mathrm{manifold}}(\widehat M)=1
\\
&
C_5^{\mathrm{hausdorff}}(M_0,\widehat M)=1
\\
&
C_6^{\mathrm{ssim}}(M_0,\widehat M)=1
\\
&
C_7^{\mathrm{bytes}}(\widehat M)=1
\end{aligned}
}
$$

---

## Given input properties

These are guaranteed for $M_0$. They are not optimization constraints.

$$
G_1^{\mathrm{size}}(M_0):
\qquad
1\le N_0\le 1.1\cdot 10^6,
\qquad
1\le K_0\le 2.1\cdot 10^6
$$

$$
G_2^{\mathrm{coordinates}}(M_0):
\qquad
\forall x_i\in\mathcal X_0,
\quad
x_i\in[-1,1]^3
$$

$$
G_3^{\mathrm{unit\ sphere}}(M_0):
\qquad
\forall x_i\in\mathcal X_0,
\quad
\|x_i\|_2\le 1
$$

$$
G_4^{\mathrm{centered\ AABB}}(M_0):
\qquad
x_{\min}=-x_{\max},
\quad
y_{\min}=-y_{\max},
\quad
z_{\min}=-z_{\max}
$$

$$
G_5^{\mathrm{input\ indices}}(M_0):
\qquad
\forall \tau=(i,j,k)\in\mathcal T_0,
\quad
i,j,k\in[N_0]
$$

$$
G_6^{\mathrm{input\ area}}(M_0):
\qquad
\forall \tau=(i,j,k)\in\mathcal T_0,
\quad
A_0(\tau)>0
$$

$$
G_7^{\mathrm{input\ edge\ incidence}}(M_0):
\qquad
\forall e\in E(\mathcal T_0),
\quad
\deg_{\mathcal T_0}(e)=2
$$

$$
G_8^{\mathrm{input\ topology}}(M_0):
\qquad
M_0\text{ is connected, closed, watertight, and a triangular }2\text{-manifold}
$$

$$
G_9^{\mathrm{input\ uniqueness}}(M_0):
\qquad
M_0\text{ has no duplicate vertices and no duplicate faces}
$$

---

## Required output constraints

These are constraints our algorithm must satisfy for $\widehat M$.

$$
C_1^{\mathrm{vertex}}(\widehat M;M_0)=1
\iff
1\le \widehat N\le N_0
$$

$$
C_2^{\mathrm{indices}}(\widehat M)=1
\iff
\forall \widehat\tau=(a,b,c)\in\widehat{\mathcal T},
\quad
a,b,c\in[\widehat N]
$$

$$
C_3^{\mathrm{area}}(\widehat M)=1
\iff
\forall \widehat\tau=(a,b,c)\in\widehat{\mathcal T},
\quad
\widehat A(\widehat\tau)>0
$$

$$
C_4^{\mathrm{manifold}}(\widehat M)=1
\iff
\forall e\in E(\widehat{\mathcal T}),
\quad
\deg_{\widehat{\mathcal T}}(e)=2
$$

$$
C_5^{\mathrm{hausdorff}}(M_0,\widehat M)=1
\iff
d_H(S_0,\widehat S)
\le
0.05\,D_{\mathrm{AABB}}(M_0)
$$

$$
C_6^{\mathrm{ssim}}(M_0,\widehat M)=1
\iff
\mathrm{FinalSSIM}(M_0,\widehat M)\ge 0.9
$$

$$
C_7^{\mathrm{bytes}}(\widehat M)=1
\iff
\mathrm{OutputBytes}(\widehat M)\le100\ \mathrm{MiB}
$$

The duplicated-looking condition is intentional:

$$
G_7^{\mathrm{input\ edge\ incidence}}(M_0)
$$

is a given property of the input mesh, while

$$
C_4^{\mathrm{manifold}}(\widehat M)
$$

is a required property of our output mesh.

---

## Constraint expansions

$$
A_0(i,j,k)
=
\frac12
\|(x_j-x_i)\times(x_k-x_i)\|_2
$$

$$
\widehat A(a,b,c)
=
\frac12
\|(\widehat x_b-\widehat x_a)\times(\widehat x_c-\widehat x_a)\|_2
$$

$$
E(\mathcal T)
=
\left\{
\{i,j\},\{j,k\},\{k,i\}
\mid
(i,j,k)\in\mathcal T
\right\}
$$

$$
\deg_{\mathcal T}(e)
=
\left|
\{\tau\in\mathcal T:e\subset\tau\}
\right|
$$

$$
S_0
=
\bigcup_{(i,j,k)\in\mathcal T_0}
\left\{
\alpha x_i+\beta x_j+\gamma x_k
\mid
\alpha,\beta,\gamma\ge0,
\alpha+\beta+\gamma=1
\right\}
$$

$$
\widehat S
=
\bigcup_{(a,b,c)\in\widehat{\mathcal T}}
\left\{
\alpha \widehat x_a+\beta \widehat x_b+\gamma \widehat x_c
\mid
\alpha,\beta,\gamma\ge0,
\alpha+\beta+\gamma=1
\right\}
$$

$$
d_H(S_0,\widehat S)
=
\max
\left(
\vec d(S_0,\widehat S),
\vec d(\widehat S,S_0)
\right)
$$

$$
\vec d(A,B)
=
\max_{p\in A}
\min_{q\in B}
\|p-q\|_2
$$

$$
D_{\mathrm{AABB}}(M_0)
=
\sqrt{L_x^2+L_y^2+L_z^2}
$$

$$
L_x=x_{\max}-x_{\min},
\qquad
L_y=y_{\max}-y_{\min},
\qquad
L_z=z_{\max}-z_{\min}
$$

---

## SSIM expansion

$$
\mathrm{FinalSSIM}(M_0,\widehat M)
=
\frac16
\sum_{r=1}^{6}
\left[
\frac12
\operatorname{SSIM}
\left(
I^N_r(M_0),I^N_r(\widehat M)
\right)
+
\frac12
\operatorname{SSIM}
\left(
I^D_r(M_0),I^D_r(\widehat M)
\right)
\right]
$$

$$
r\in\{1,2,3,4,5,6\}
$$

$$
E^{\mathrm{cam}}_r
\in
\{(\pm 2.5,0,0),(0,\pm 2.5,0),(0,0,\pm 2.5)\}
$$

$$
\pi(q)
=
\begin{pmatrix}
800\,q_x/q_z+512\\
800\,q_y/q_z+512
\end{pmatrix}
$$

$$
I^N_r(p;M)
=
\begin{cases}
127.5(n_{\tau^*(p)}+\mathbf 1),
&
\mathcal T_r(p;M)\neq\varnothing
\\
127.5\mathbf 1,
&
\mathcal T_r(p;M)=\varnothing
\end{cases}
$$

$$
I^D_r(p;M)
=
\begin{cases}
Z_{\tau^*(p)}(p),
&
\mathcal T_r(p;M)\neq\varnothing
\\
255,
&
\mathcal T_r(p;M)=\varnothing
\end{cases}
$$

$$
n_{(i,j,k)}
=
\frac{(x_j-x_i)\times(x_k-x_i)}
{\|(x_j-x_i)\times(x_k-x_i)\|_2}
$$

$$
Z_{(i,j,k)}(p)
=
\frac{1}{
\lambda_i/z_i+
\lambda_j/z_j+
\lambda_k/z_k
}
$$

$$
p
=
\lambda_i\pi(x_i)
+
\lambda_j\pi(x_j)
+
\lambda_k\pi(x_k)
$$

$$
\lambda_i+\lambda_j+\lambda_k=1,
\qquad
\lambda_i,\lambda_j,\lambda_k\ge0
$$

$$
\tau^*(p)
=
\arg\min_{\tau\in\mathcal T_r(p;M)}
Z_\tau(p)
$$

$$
\operatorname{SSIM}(A,B)
=
\frac{
(2\mu_A\mu_B+c_1)(2\sigma_{AB}+c_2)
}{
(\mu_A^2+\mu_B^2+c_1)(\sigma_A^2+\sigma_B^2+c_2)
}
$$

$$
c_1=(0.01\cdot255)^2,
\qquad
c_2=(0.03\cdot255)^2
$$

The evaluator uses six axial cameras, flat face normals, perspective-correct depth, foreground-only SSIM averaging, and equal normal/depth weights.

---

## Variable dictionary

| Symbol | Meaning |
|---|---|
| $M_0$ | fixed original input mesh |
| $\widehat M$ | submitted simplified mesh |
| $\mathcal X_0$ | original vertex set |
| $\widehat{\mathcal X}$ | output vertex set |
| $\mathcal T_0$ | original triangle/face set |
| $\widehat{\mathcal T}$ | output triangle/face set |
| $x_i$ | original vertex $i$ |
| $\widehat x_i$ | output vertex $i$ |
| $N_0$ | number of original vertices |
| $\widehat N$ | number of output vertices |
| $K_0$ | number of original triangles |
| $\widehat K$ | number of output triangles |
| $\tau$ | triangle/face index triple |
| $e$ | undirected mesh edge |
| $E(\mathcal T)$ | edge set induced by triangle set $\mathcal T$ |
| $\deg_{\mathcal T}(e)$ | number of triangles incident to edge $e$ |
| $S_0$ | continuous surface of original mesh |
| $\widehat S$ | continuous surface of output mesh |
| $O$ | compression objective |
| $G_i$ | guaranteed input property |
| $C_i$ | required output/evaluation constraint |
| $D_{\mathrm{AABB}}$ | original AABB diagonal |
| $d_H$ | symmetric Hausdorff distance |
| $I^N_r$ | normal map under camera $r$ |
| $I^D_r$ | depth map under camera $r$ |
| $r$ | camera/view index |
| $p$ | pixel center |
| $\pi$ | perspective projection |
| $\mathcal T_r(p;M)$ | triangles of $M$ covering pixel $p$ in view $r$ |
| $\tau^*(p)$ | visible triangle after z-buffering |
| $Z_\tau(p)$ | perspective-correct depth of triangle $\tau$ at pixel $p$ |
| $\lambda_i,\lambda_j,\lambda_k$ | barycentric weights |
| $\mu_A,\mu_B$ | local SSIM window means |
| $\sigma_A^2,\sigma_B^2$ | local SSIM window variances |
| $\sigma_{AB}$ | local SSIM covariance |
| $c_1,c_2$ | SSIM stabilization constants |

This is the clean separation: $G_i$ are facts about $M_0$; $C_i$ are obligations for $\widehat M$.
