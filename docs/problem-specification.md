# Problem Formulation Dense Extract

Source problem: **Problem B — Perception-Aware Lossless Simplification of Million-Vertex 3D Meshes for Mobile Platforms**.

Purpose: implementation-oriented extraction of every relevant constraint, constant, evaluator property, geometric assumption, input/output rule, scoring formula, and data-size bound.

---

## 1. Task in One Line

- Input: one closed triangular 3D mesh $M$.
- Output: a simplified triangular mesh $M'$.
- Primary objective: minimize the number of output vertices $V'$.
- Hard validity: output must remain a valid closed watertight 2-manifold triangular mesh.
- Geometric constraint: symmetric Hausdorff distance from $M$ to $M'$ must be at most $5\%$ of the original AABB diagonal.
- Perceptual validity threshold: final multi-view SSIM must satisfy $\mathrm{FinalSSIM} \ge 0.9$.
- Score per valid test case: compression rate based only on vertex count.

---

## 2. Input Mesh Guarantees

- Input format: modified OBJ-like text format.
- Input read from `stdin`.
- First line: two integers:
  - $V$: number of vertices.
  - $F$: number of triangular faces.
- Global input bounds:
  - $1 \le V \le 1.1 \cdot 10^6$.
  - $1 \le F \le 2.1 \cdot 10^6$.
- Vertex lines:
  - Exactly $V$ lines after the header.
  - Each vertex line begins with literal character `v`.
  - Format: `v x y z`.
  - Coordinates are real numbers.
  - Coordinate bounds: $-1 \le x,y,z \le 1$.
  - Coordinates have at most 15 digits after the decimal point.
- Face lines:
  - Exactly $F$ lines after the vertex block.
  - Each face line begins with literal character `f`.
  - Format: `f v1 v2 v3`.
  - Face indices are 1-indexed.
  - Index bounds: $1 \le v_1,v_2,v_3 \le V$.
  - Each face is triangular.
- Input mesh geometric normalization:
  - Mesh is pre-normalized.
  - Axis-aligned bounding box is centered at the origin.
  - $x_{\min}=-x_{\max}$.
  - $y_{\min}=-y_{\max}$.
  - $z_{\min}=-z_{\max}$.
  - Every vertex lies inside or on the unit sphere:
    $$
    \sqrt{x^2+y^2+z^2}\le 1.
    $$
- Input mesh topological guarantees:
  - Closed.
  - Watertight.
  - Connected surface.
  - 2-manifold.
  - Every edge is shared by exactly two faces.
  - Every face is non-degenerate.
  - For every input face, its three vertices are distinct.
  - For every input face, the three vertices span positive area.
  - No duplicate vertices.
  - No duplicate faces.
- No per-vertex normals are provided.
- Evaluator uses face normals, not input normals.

---

## 3. Output Mesh Requirements

- Output written to `stdout`.
- Output uses the same modified OBJ-like format as input:
  - First line: `V' F'`.
  - Then $V'$ vertex lines: `v x y z`.
  - Then $F'$ triangular face lines: `f v1 v2 v3`.
- Output mesh must be triangular.
- Output vertex count constraint:
  - $1 \le V' \le V$.
  - $V'=0$ is rejected.
  - $V'>V$ is rejected.
- Output face-index validity:
  - All output face indices must be within the output vertex array range.
  - Using an index outside $[1,V']$ is invalid.
- Output topology requirements:
  - Output mesh must be manifold.
  - Every edge must be shared by exactly two triangular faces.
  - Output must be closed/watertight as a 2-manifold.
- Output geometry requirements:
  - No zero-area triangular faces.
  - All triangular faces must have positive area.
  - Degenerate faces are invalid.
- Output size constraint:
  - Total output size must be at most 100 MiB.
  - This makes excessive decimal printing dangerous on large meshes.
- No explicit output face-count upper bound is stated beyond format validity, manifold validity, and 100 MiB output size.
- No explicit requirement is stated that $F' \le F$.
- If any hard output constraint is violated, the submission receives `Wrong Answer` for that case.

---

## 4. Test-Case Size Bounds

| Test case | Vertex bound $V \le$ | Face bound $F \le$ | Notes |
|---:|---:|---:|---|
| 1 | 10 | 15 | sample |
| 2 | 5,000 | 10,000 | small |
| 3 | 25,000 | 50,000 | medium-small |
| 4 | 40,000 | 80,000 | medium |
| 5 | 50,000 | 100,000 | medium-large |
| 6 | 400,000 | 800,000 | large |
| 7 | 1,100,000 | 2,100,000 | maximum |

Additional global statement:

- Across all test cases, vertex count is at most $1.1\cdot10^6$.
- Across all test cases, face count is at most $2.1\cdot10^6$.

Implementation implication:

- Need algorithms that can handle up to 1.1M vertices and 2.1M faces.
- Need streaming/fast I/O or memory-conscious parsing.
- Need avoid quadratic operations over vertices/faces.
- Need output compactly enough to stay below 100 MiB.

---

## 5. Evaluator Overview

- The evaluator compares original mesh $M$ and simplified mesh $M'$ through rendered feature maps.
- Rendering is offline and standardized.
- The evaluator uses virtual cameras around the origin.
- The evaluator renders the input coordinates directly.
- The evaluator performs no further scaling.
- The evaluator performs no further recentering.
- The fixed camera setup always frames the model identically because input is pre-normalized.
- Per view, the evaluator renders two feature maps:
  - Normal map.
  - Depth map.
- SSIM is computed between original and simplified normal/depth maps.
- Final score averages over six axial views.

---

## 6. Camera Placement

- Number of views: 6.
- View index:
  $$
  k \in \{1,2,3,4,5,6\}.
  $$
- Camera positions are the six positive/negative coordinate-axis directions:
  $$
  E_k \in \{(\pm D,0,0),(0,\pm D,0),(0,0,\pm D)\}.
  $$
- Observation distance:
  $$
  D=2.5.
  $$
- Cameras face the mesh origin.
- Views correspond to:
  - $+X$ looking toward origin.
  - $-X$ looking toward origin.
  - $+Y$ looking toward origin.
  - $-Y$ looking toward origin.
  - $+Z$ looking toward origin.
  - $-Z$ looking toward origin.
- Mesh coordinates are already centered and scaled; evaluator renders them directly at distance $D$.

---

## 7. Fixed Camera and Rendering Constants

| Quantity | Symbol | Value |
|---|---:|---:|
| Observation distance | $D$ | $2.5$ model units |
| Focal length x | $f_x$ | $800.0$ px |
| Focal length y | $f_y$ | $800.0$ px |
| Feature map width | $W$ | $1024$ px |
| Feature map height | $H$ | $1024$ px |
| Principal point x | $c_u$ | $512$ px |
| Principal point y | $c_v$ | $512$ px |
| Background depth | $z_b$ | $255$ |
| Background normal vector | $\mathbf n_b$ | $(0,0,0)$ |
| Background normal RGB | | $(127.5,127.5,127.5)$ |

---

## 8. Camera-Space Projection

- Projection uses camera-space coordinates.
- Camera-space convention:
  - Camera placed at the origin.
  - Camera faces the negative $Z$ axis.
  - OpenGL-style convention.
- For a camera-space vertex $V(x,y,z)$, projected screen coordinate $p(u,v)$ is:
  $$
  u=f_x\frac{x}{z}+c_u,
  \qquad
  v=f_y\frac{y}{z}+c_v.
  $$
- Constants:
  - $f_x=f_y=800.0$.
  - $c_u=c_v=512$.
- $(u,v)$ is initially floating-point.
- The projected coordinate is later rasterized to discrete pixel grid locations.
- Pixel sampling uses pixel centers:
  $$
  (u+0.5,\ v+0.5).
  $$

Implementation note:

- The problem statement specifies the camera-space projection formula, but not the explicit world-to-camera rotation matrices for the six axial cameras. Any local evaluator must reproduce the six axis-aligned camera transforms consistently with cameras facing the origin.

---

## 9. Rasterization and Visibility

- For each pixel center, the evaluator determines whether the pixel is covered by any projected triangle.
- Each pixel is sampled once at its center.
- For a covered pixel, the selected triangle is the nearest triangle whose projection contains that pixel center.
- For that nearest triangle:
  - Normal map gets the triangle's flat face normal.
  - Depth map gets perspective-correct interpolated depth.
- Pixels covered by no triangle receive fixed background attributes.
- Occlusion therefore matters.
- Silhouette/contour agreement matters because background-vs-foreground changes affect SSIM foreground windows.

---

## 10. Normal Map Definition

- Purpose: light-shadow / surface undulation visual consistency.
- Shading model: flat shading.
- Each triangular face has one unit normal.
- Every pixel covered by a face gets that same face normal.
- Normals are not interpolated across a triangle.
- There are no per-vertex normals in the input.
- Normal vector components lie in $[-1,1]$.
- Normal vector at pixel $p$:
  $$
  \mathbf n_p = (n_x,n_y,n_z).
  $$
- Normal-to-RGB mapping:
  $$
  I_N(p)=\left(\mathbf n_p + [1,1,1]^T\right)\times127.5.
  $$
- Componentwise interpretation:
  $$
  R=(n_x+1)127.5,
  \qquad
  G=(n_y+1)127.5,
  \qquad
  B=(n_z+1)127.5.
  $$
- Normal value range after mapping: $[0,255]$ per RGB channel.
- Background normal vector:
  $$
  \mathbf n_b=(0,0,0).
  $$
- Background normal RGB:
  $$
  (127.5,127.5,127.5).
  $$

Implementation implications:

- Preserving face orientation and normals is directly rewarded.
- Large triangles are allowed only if their flat-normal image remains close to original.
- Normal discontinuities matter; flat-shaded facets are what the metric sees.
- Vertex normal smoothing is irrelevant unless it changes geometry and therefore face normals.

---

## 11. Depth Map Definition

- Purpose: contour morphology, volume shrinkage, occlusion consistency, silhouette consistency.
- Depth map records depth value $z$ along camera viewing axis for each visible pixel.
- For a pixel $p$ inside a triangle, depth is obtained from the three vertex depths:
  $$
  I_D(p)=\operatorname{Interpolate}(z_0,z_1,z_2).
  $$
- Vertex depths:
  - $z_0,z_1,z_2$ are the camera-space $Z$-axis depth coordinates of the triangle vertices.
- Depth interpolation is perspective-correct.
- The quantity linear in screen space is reciprocal depth $1/z$, not $z$.
- Background depth:
  $$
  z_b=255.
  $$

Implementation implications:

- Silhouette and foreground/background consistency are critical.
- Occlusion order must be preserved.
- Avoid shrinkage that changes depth contours even if Hausdorff passes.
- Depth changes can reduce SSIM even when topology is valid.

---

## 12. Barycentric Interpolation

- For a pixel $P$ inside a projected triangle, barycentric weights $(w_0,w_1,w_2)$ satisfy:
  $$
  P=w_0p_0+w_1p_1+w_2p_2.
  $$
- Projected triangle vertices:
  - $p_0,p_1,p_2$.
- Weight constraints:
  $$
  w_0+w_1+w_2=1.
  $$
- Perspective-correct depth:
  $$
  z_P=\frac{1}{\dfrac{w_0}{z_0}+\dfrac{w_1}{z_1}+\dfrac{w_2}{z_2}}.
  $$
- Face normal is not interpolated.
- Face normal is written directly and constantly over the visible triangle.

---

## 13. Feature Maps

- Feature maps are rendered 2D images used as SSIM inputs.
- Feature map types:
  - Normal map: RGB, surface normal direction mapped to color.
  - Depth map: grayscale/scalar depth image.
- Feature map resolution:
  $$
  1024\times1024.
  $$
- Feature map principal point:
  $$
  (c_u,c_v)=(512,512).
  $$
- Per view, the evaluator generates:
  - $I_{N,i}$: original normal map.
  - $I_{D,i}$: original depth map.
  - $I'_{N,i}$: simplified normal map.
  - $I'_{D,i}$: simplified depth map.
- Total rendered maps per comparison:
  - 6 original normal maps.
  - 6 original depth maps.
  - 6 simplified normal maps.
  - 6 simplified depth maps.
- Original maps may be cached by the evaluator, but participant does not control this.

---

## 14. Hard Mesh Validity Constraints

A submitted mesh is invalid / `Wrong Answer` if any of these fail:

- Vertex count:
  $$
  1\le V'\le V.
  $$
- Mesh is manifold.
- Every edge is shared by exactly two triangular faces.
- Mesh is closed/watertight as a 2-manifold.
- Every triangular face has positive area.
- No zero-area faces.
- All face indices are valid output-vertex indices.
- Output file size is at most 100 MiB.

---

## 15. Geometric Deviation Constraint

- Required:
  $$
  d_H(M,M')\le 5\%\times\mathrm{Diagonal}.
  $$
- Equivalent numeric factor:
  $$
  d_H(M,M')\le 0.05\cdot\mathrm{Diagonal}.
  $$
- $d_H(M,M')$ is the symmetric Hausdorff distance.
- Symmetric Hausdorff:
  $$
  d_H(M,M')=\max\left(\vec d(M,M'),\vec d(M',M)\right).
  $$
- One-way distance:
  $$
  \vec d(A,B)=\max_{a\in A}\min_{b\in B}\lVert a-b\rVert.
  $$
- Interpretation of $\vec d(M,M')$:
  - Every point/vertex/surface sample of the original must remain close to the simplification.
  - Prevents loss of original coverage.
- Interpretation of $\vec d(M',M)$:
  - Simplified mesh must stay close to original surface.
  - Prevents output vertices/surface from drifting away.
- The statement says the first direction requires every original vertex to remain covered by simplification.
- The statement says the second direction forbids simplified vertices from straying away from original surface.

AABB diagonal definition:

- Original bounds:
  $$
  x_{\min},y_{\min},z_{\min},x_{\max},y_{\max},z_{\max}.
  $$
- AABB edge lengths:
  $$
  L_x=x_{\max}-x_{\min},
  \qquad
  L_y=y_{\max}-y_{\min},
  \qquad
  L_z=z_{\max}-z_{\min}.
  $$
- Diagonal:
  $$
  \mathrm{Diagonal}=\sqrt{L_x^2+L_y^2+L_z^2}.
  $$
- Since input AABB is centered at origin:
  $$
  L_x=2x_{\max},\quad L_y=2y_{\max},\quad L_z=2z_{\max}
  $$
  when maxima are nonnegative.
- Since every vertex lies in the unit sphere, a loose upper bound is:
  $$
  \mathrm{Diagonal}\le 2\sqrt{3},
  $$
  but the actual tolerance uses the actual original AABB diagonal.

Implementation implications:

- Need preserve geometry within a global 5%-of-AABB diagonal tolerance.
- Passing Hausdorff does not guarantee scoring; SSIM can still fail.
- Failing Hausdorff is a hard invalidity issue, not just low score.

---

## 16. Optimization Objective

- Primary optimization objective:
  $$
  \min V'.
  $$
- Equivalent ranking goal:
  $$
  \max \left(100.0-100.0\frac{a_{M'}}{a_M}\right).
  $$
- $a_M$ is original vertex count.
- $a_{M'}$ is simplified vertex count.
- Compression rate:
  $$
  \mathrm{CompressionRate}=100.0-100.0\times\frac{a_{M'}}{a_M}.
  $$
- If $V'=V$, compression rate is 0.
- If $V'=0$ were allowed, compression would be 100, but $V'=0$ is invalid.
- Score is based on vertex count, not face count.
- Face count only matters indirectly through validity, rendering quality, output size, and runtime.

---

## 17. Perceptual Validity Threshold

- Required perceptual threshold:
  $$
  \mathrm{FinalSSIM}\ge0.9.
  $$
- If $\mathrm{FinalSSIM}<0.9$:
  - Test-case score is 0.
  - Submission is excluded from ranking for that case.
- If $\mathrm{FinalSSIM}\ge0.9$:
  - Test-case score is the compression rate.
- Validity in scoring sense requires both:
  - Hard constraints pass.
  - FinalSSIM threshold passes.

---

## 18. FinalSSIM Formula

- Final perceptual score:
  $$
  \mathrm{FinalSSIM}
  =\operatorname{Average}_{i=1}^{6}
  \left(
  \omega_N\operatorname{SSIM}(I_{N,i},I'_{N,i})
  +
  \omega_D\operatorname{SSIM}(I_{D,i},I'_{D,i})
  \right).
  $$
- View index:
  $$
  i\in\{1,2,3,4,5,6\}.
  $$
- Original maps:
  - $I_{N,i}$: original normal map at view $i$.
  - $I_{D,i}$: original depth map at view $i$.
- Simplified maps:
  - $I'_{N,i}$: simplified normal map at view $i$.
  - $I'_{D,i}$: simplified depth map at view $i$.
- Weights:
  $$
  \omega_N=0.5,
  \qquad
  \omega_D=0.5.
  $$
- Average is arithmetic mean over six views.
- SSIM output range:
  $$
  \operatorname{SSIM}(\cdot,\cdot)\in[0,1].
  $$
- $1$ means full visual consistency.

Expanded average:

$$
\mathrm{FinalSSIM}
=\frac{1}{6}\sum_{i=1}^{6}
\left(
0.5\operatorname{SSIM}(I_{N,i},I'_{N,i})
+0.5\operatorname{SSIM}(I_{D,i},I'_{D,i})
\right).
$$

---

## 19. SSIM Formula

For images $X$ and $Y$:

$$
\operatorname{SSIM}(X,Y)
=
\frac{(2\mu_X\mu_Y+c_1)(2\sigma_{XY}+c_2)}
{(\mu_X^2+\mu_Y^2+c_1)(\sigma_X^2+\sigma_Y^2+c_2)}.
$$

Parameters:

- Local window size:
  $$
  11\times11.
  $$
- $\mu_X$: mean pixel value in local window of $X$.
- $\mu_Y$: mean pixel value in local window of $Y$.
- $\sigma_X$: local pixel standard deviation or variance term as given by SSIM notation in statement.
- $\sigma_Y$: local pixel standard deviation or variance term as given by SSIM notation in statement.
- $\sigma_{XY}$: local cross-covariance between $X$ and $Y$.
- Dynamic range:
  $$
  L=255.
  $$
- Constants:
  $$
  k_1=0.01,
  \qquad
  k_2=0.03.
  $$
- Stabilization constants:
  $$
  c_1=(k_1L)^2=(0.01\cdot255)^2=2.55^2=6.5025.
  $$
  $$
  c_2=(k_2L)^2=(0.03\cdot255)^2=7.65^2=58.5225.
  $$

Foreground-only averaging:

- Final image SSIM is the mean of per-window SSIM values.
- Averaging is over rendered foreground only.
- A window is included if the original rendering and/or simplified rendering is non-background at the window center pixel.
- A window is excluded if both original and simplified renderings are background at the window center pixel.
- The same foreground-only averaging rule applies to:
  - Each channel of the normal map.
  - The depth map.
- For RGB normal maps:
  - SSIM is calculated separately on the three color channels.
  - The three channel SSIM values are averaged for the final normal-map SSIM.

Implementation implications:

- Background-only empty regions do not dilute errors.
- Silhouette errors matter because windows centered on foreground in either rendering are included.
- Normal-map errors and depth-map errors have equal weight.
- Normal RGB channels have equal internal contribution through channel averaging.

---

## 20. Ranking and Final Score

Per test case:

- First, hard constraints must pass.
- Then, perceptual threshold must pass:
  $$
  \mathrm{FinalSSIM}\ge0.9.
  $$
- If $\mathrm{FinalSSIM}<0.9$, score for that test case is 0.
- If valid, score for that test case is:
  $$
  100.0-100.0\times\frac{a_{M'}}{a_M}.
  $$
- Ranking priority among valid submissions is determined by simplified vertex count.
- Lower $V'$ is better.
- Face count is not directly part of the ranking formula.

Across all test cases:

- Final score is the average of test-case scores.

Sample scoring example:

- Original sample vertex count: $a_M=9$.
- Simplified sample vertex count: $a_{M'}=8$.
- Compression rate:
  $$
  100.0-100.0\times\frac{8}{9}\approx11.11.
  $$
- Sample has $\mathrm{FinalSSIM}=1$ because the simplification removes a redundant vertex without changing face normal and depth.
- Sample case awards no points; it is only for debugging.

---

## 21. Environment / Library / Language Notes

- Large input: provided C++ and Python fast I/O templates exist in attachments.
- Participants may modify or ignore provided templates.
- C++ Eigen library is available.
- Include path for Eigen:
  ```cpp
  #include "Eigen/Dense"
  ```
- Eigen version:
  $$
  5.0.0.
  $$
- Eigen files are placed in the same directory as the solution when compiled.
- Participants do not submit Eigen files.

---

## 22. Useful Implementation-Level Consequences

Hard constraints to guard before printing:

- Output $V'$ must satisfy $1\le V'\le V$.
- Output indices must be remapped correctly after deletions/collapses.
- No unused/index-invalid face references.
- No triangle with repeated vertex index.
- No triangle with zero or near-zero geometric area.
- Every undirected output edge must occur exactly twice.
- Surface must remain closed; boundary edges cause invalidity.
- Non-manifold edges with count $\ne2$ cause invalidity.
- Output text must stay under 100 MiB.

Metric-sensitive properties:

- Preserve six axial silhouettes.
- Preserve visible depth from $\pm X,\pm Y,\pm Z$.
- Preserve face normals in visible regions.
- Preserve occlusion ordering.
- Avoid moving vertices enough to alter projected boundaries.
- Avoid collapses that flatten high-normal-variation regions visible to cameras.
- Since normal map is flat-shaded, triangle-level normals matter more directly than smooth vertex normals.
- Since the evaluator uses 1024×1024 maps and $f=800$, subpixel/small-pixel changes may still affect SSIM near contours and high-frequency normal regions.
- Since foreground-only averaging excludes mutual background, errors concentrate over the rendered object, not the full image.

Memory/time-sensitive properties:

- Maximum mesh size is 1.1M vertices and 2.1M faces.
- Building full dense pairwise geometry structures is infeasible.
- Edge-based structures should be linear or near-linear in $F$.
- Rendering a faithful local evaluator at full resolution requires 6 views × 1024×1024 pixels × normal/depth buffers.
- Local evaluation approximations should account for flat normals, depth, silhouettes, and six axial views.

Scoring strategy implications:

- The SSIM threshold is binary for scoring: below 0.9 gives 0 for the case.
- Once above 0.9, only vertex compression affects score.
- Therefore the useful optimization target is aggressive vertex reduction under a safety margin above $0.9$.
- Directly optimizing face count does not improve ranking unless it enables fewer vertices, faster runtime, smaller output, or better SSIM.

---

## 23. Complete Parameter List

| Parameter / Quantity | Value / Formula |
|---|---:|
| Max input vertices | $1.1\cdot10^6$ |
| Max input faces | $2.1\cdot10^6$ |
| Coordinate bounds | $-1\le x,y,z\le1$ |
| Coordinate decimal precision in input | at most 15 digits after decimal |
| Unit sphere bound | $\sqrt{x^2+y^2+z^2}\le1$ |
| AABB centering | $x_{\min}=-x_{\max}$, same for $y,z$ |
| Output size limit | 100 MiB |
| Output vertex count | $1\le V'\le V$ |
| Number of camera views | 6 |
| Camera positions | $(\pm D,0,0),(0,\pm D,0),(0,0,\pm D)$ |
| Observation distance | $D=2.5$ |
| Focal length | $f_x=f_y=800.0$ px |
| Feature resolution | $1024\times1024$ |
| Principal point | $(c_u,c_v)=(512,512)$ |
| Projection | $u=f_xx/z+c_u$, $v=f_yy/z+c_v$ |
| Pixel sample position | center $(u+0.5,v+0.5)$ |
| Normal shading | flat face normal |
| Normal interpolation | none |
| Normal RGB mapping | $(\mathbf n+[1,1,1]^T)127.5$ |
| Normal background vector | $(0,0,0)$ |
| Normal background RGB | $(127.5,127.5,127.5)$ |
| Depth interpolation | perspective-correct |
| Perspective-correct depth | $z_P=1/(w_0/z_0+w_1/z_1+w_2/z_2)$ |
| Background depth | $255$ |
| Hausdorff threshold | $d_H(M,M')\le0.05\cdot\mathrm{Diagonal}$ |
| AABB diagonal | $\sqrt{L_x^2+L_y^2+L_z^2}$ |
| FinalSSIM threshold | $\ge0.9$ |
| Normal-map weight | $\omega_N=0.5$ |
| Depth-map weight | $\omega_D=0.5$ |
| SSIM local window | $11\times11$ |
| SSIM dynamic range | $L=255$ |
| SSIM $k_1$ | $0.01$ |
| SSIM $k_2$ | $0.03$ |
| SSIM $c_1$ | $6.5025$ |
| SSIM $c_2$ | $58.5225$ |
| Compression rate | $100-100a_{M'}/a_M$ |
| Final score | average of test-case scores |
| Eigen version | 5.0.0 |

---

## 24. Minimal Validity Checklist for an Implementation

Before output:

- [ ] $V'\ge1$.
- [ ] $V'\le V$.
- [ ] Output file size likely below 100 MiB.
- [ ] All face indices in $[1,V']$.
- [ ] All faces have three distinct vertex indices.
- [ ] All faces have positive geometric area.
- [ ] No edge has incidence count 1.
- [ ] No edge has incidence count greater than 2.
- [ ] Every undirected edge has incidence count exactly 2.
- [ ] Output surface remains closed/watertight.
- [ ] Output surface remains a 2-manifold.
- [ ] Approximate or exact Hausdorff check satisfies $d_H\le0.05\cdot\mathrm{Diagonal}$.
- [ ] Estimated or simulated FinalSSIM stays safely above 0.9.
- [ ] Vertex count is reduced as aggressively as possible after the above constraints.

---

## 25. Local Evaluator Checklist

To approximate official scoring locally, reproduce:

- [ ] Six cameras at $\pm X,\pm Y,\pm Z$, distance $2.5$, facing origin.
- [ ] Camera-space convention: camera at origin, looking down negative $Z$.
- [ ] Projection with $f_x=f_y=800$, $c_u=c_v=512$.
- [ ] Resolution $1024\times1024$.
- [ ] Pixel-center sampling.
- [ ] Triangle rasterization by projected containment.
- [ ] Nearest visible triangle selection.
- [ ] Flat face normals.
- [ ] Normal RGB mapping to $[0,255]$.
- [ ] Perspective-correct depth interpolation using reciprocal depth.
- [ ] Background normal $(127.5,127.5,127.5)$.
- [ ] Background depth $255$.
- [ ] SSIM with $11\times11$ windows.
- [ ] Constants $k_1=0.01$, $k_2=0.03$, $L=255$.
- [ ] Foreground-only SSIM averaging.
- [ ] Exclude windows whose center pixel is background in both original and simplified renderings.
- [ ] Compute normal SSIM per RGB channel and average channels.
- [ ] Combine normal/depth with weights $0.5/0.5$.
- [ ] Average over six views.

---

## 26. What the Metric Actually Rewards

- Fewer vertices, as long as validity and FinalSSIM threshold hold.
- Preserved visible silhouettes under six orthographic-like axial observations with perspective projection.
- Preserved visible depth fields.
- Preserved flat-shaded face-normal fields.
- Preserved occlusion relations.
- Preserved local SSIM statistics in foreground regions.
- Topologically valid closed surfaces.

What it does not directly reward:

- Lower face count by itself.
- Smooth vertex normals.
- Texture fidelity, because no texture is part of the evaluator.
- Color/material fidelity, because rendered features are normal and depth maps only.
- Geometry hidden from all six axial views except through Hausdorff/topology constraints.

