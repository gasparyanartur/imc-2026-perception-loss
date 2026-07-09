# **A Comprehensive Guide to Mesh Simplification using Edge Collapse** 

*Note to Coding Assistant: This is an overview paper on mesh simplification techniques. We have included it for reference and brain-storming new solutions. Do not rely on the provided code, as it has inefficiencies and is incomplete.*

> Purva Kulkarni © Aravind Shankara Narayanan Independent Researcher Independent Researcher 

**Figure 1** . Mesh simplification using edge collapse. 

## Abstract 

_Mesh simplification is the process of reducing the number of vertices, edges and triangles in a three-dimensional (3D) mesh while preserving the overall shape and salient features of the mesh. A popular strategy for this is edge collapse, where an edge connecting two vertices is merged into a single vertex. The edge to collapse is chosen based on a cost function that estimates the error introduced by this collapse. This paper presents a comprehensive, implementation-oriented guide to edge collapse for practitioners and researchers seeking both theoretical grounding and practical insight. We review and derive the underlying mathematics and provide reference implementations for foundational cost functions including Quadric Error Metrics (QEM) and Lindstrom-Turk’s geometric criteria. We also explain the mathematics behind attribute-aware edge collapse in QEM variants and Hoppe’s energy-based method used in progressive meshes. In addition to cost functions, we outline the complete edge collapse algorithm, including the specific sequence of operations and the data structures that are commonly used. To create a robust system, we also cover the necessary programmatic safeguards that prevent issues like mesh degeneracies, inverted normals, and improper handling of boundary conditions. The goal of this work is not only to consolidate established methods but also to bridge the gap between theory and practice, offering a clear, step-by-step guide for implementing mesh simplification pipelines based on edge collapse._ 

1 

_1 INTRODUCTION_ 

## **1. Introduction** 

Triangles are the most commonly used drawing primitive in computer graphics. They are natively supported by almost all graphics libraries and hardware systems, making triangular meshes the dominant representation in 3D modeling. Modern graphics systems are capable of rendering models composed of millions of triangles, thanks to decades of hardware advancements. However, with Moore’s Law plateauing and the geometric complexity of meshes increasing rapidly, relying on brute-force parallel processing is no longer viable. This makes mesh simplification techniques more essential than ever for achieving real-time performance and scalability in interactive and large-scale applications. Mesh simplification forms the basis of level of detail (LOD) systems to ease GPU workload, accelerates collision detection in games, and enables faster coarse approximations in FEA simulations. 

Among the various mesh simplification techniques available, edge collapse is most widely adopted in practice. This strategy is implemented in many major graphics libraries and tools like _CGAL_ , _QSlim_ , and _meshoptimizer_ . An edge collapse operation merges the two endpoints of an edge into a single new vertex, effectively removing the edge and the two triangles that shared it. Repeating this operation iteratively leads to a simplified mesh that maintains the overall structure of the original. Cost functions help determine which edge to collapse and where to place the resulting vertex in order to best preserve the model’s visual and geometric details. 

**Figure 2** . Edge collapse 

While mesh simplification is a well-studied topic, newcomers to the field often face a steep learning curve when engaging with foundational papers. Many of these works emphasize final equations or high-level algorithmic descriptions, offering little insight into the underlying geometric reasoning or the practical implementation details. As a result, readers may struggle to build an intuitive understanding of how and why edge collapse-based simplification works, or how to translate theory into working code. 

This paper aims to bridge that gap by offering a detailed, implementation-aware analysis of edge collapse-based mesh simplification on a manifold mesh. Our contributions are as follows: 

2 

_2 FAMILIES OF MESH SIMPLIFICATION TECHNIQUES_ 

- We present a complete, end-to-end simplification pipeline that includes well-chosen data structures for representing mesh connectivity, deep analysis of cost functions presented in foundational papers in this space, and the edge collapse algorithm that binds both of these. 

- Unlike many prior works that present only the final cost metrics or optimization functions, we derive and explain them along with the geometric meaning behind these formulations, allowing readers to understand the rationale behind each step. 

- Our goal is two-fold: to serve as a conceptual guide for learners who want to understand the inner workings of simplification algorithms, and to act as a practical reference for developers looking to implement their own systems. 

In this paper, we first categorize and review different families of mesh simplification algorithms. Since the efficiency of edge-collapse operations depends on fast access to mesh connectivity and rapid local updates, int the following section, we discusse data structures that can be employed to store and manage the mesh connectivity information. We then present a comprehensive edge-collapse algorithm, including detailed programmatic checks to prevent mesh degeneracies. Our most extensive section examines cost computation strategies, explaining the mathematical formulations from foundational papers alongside practical implementations. In the next section, we cover advance edge collapse techinques that account for per-vertex attributes. Finally, we provide supplemental mathematical results and proofs that support these techniques. 

## **2. Families of mesh simplification techniques** 

Mesh simplification techniques vary widely, but most can be grouped by the strategy they use to reduce geometric complexity while maintaining topology as presented in [Cignoni et al. 1998]. We have supplemented this list with recent advances in the field that leverage modern techniques such as machine learning and neural networks. 

An early strategy for mesh decimation focused on detecting coplanar or nearly coplanar surface patches and merging them into larger polygonal regions as presented in [DeHaemer Jr and Zyda 1991] and [Hinker and Hansen 1993]. These regions are subsequently re-triangulated to produce a mesh with fewer faces. Despite its simplicity, the method often degraded geometric detail and introduced topological inconsistencies. 

Another method, known as vertex clustering, groups nearby vertices based on spatial proximity and replaces each cluster with a single representative vertex, followed by local re-triangulation as presented in [Rossignac and Borrel 1993] and improved in [Low and Tan 1997]. While faster, this method was again found to compromise detail and topological accuracy. 

A more refined and topology-sensitive method is iterative local decimation, which incrementally removes vertices, edges, or faces based on localized geometric evaluations. 

3 

_2 FAMILIES OF MESH SIMPLIFICATION TECHNIQUES_ 

These operations are typically guided by cost functions designed to preserve the mesh’s overall structure and appearance [Garland and Heckbert 1997; Lindstrom and Turk 1998; Schroeder et al. 1992]. Extensions such as simplification envelopes [Cohen et al. 1996] presents bounded error control by forcing the resulting simplified mesh to lie between two offset meshes. 

In energy-based optimization methods, such as the one presented in [Hoppe et al. 1993], a global cost function evaluates the overall quality of the mesh. Simplification is carried out through iterative edge-based operations such as collapse, swap, or split that aims at minimizing both the local and global cost function. Although this approach with global optimization promises a better overall structural preservation, it is less commonly used in practice due to its computational complexity. 

A different strategy is retiling, introduced in [Turk 1992], which begins by randomly placing a user-defined reduced number of new vertices on the original surface which are then adjusted based on areas of high curvature. A new reduced triangulation is built on this vertex set. Although effective in reducing triangle count, this method lacks support for per-vertex attributes, making it less suitable for applications like computer-aided design (CAD) or physical simulations where such data is essential. 

Another notable approach to mesh simplification is voxelization, as used by works such as [He et al. 1995] and [He et al. 1996]. Here, the mesh is first sampled into a voxel grid, and a low-pass filter is applied at each grid point to generate a discrete scalar field. A triangulated surface is then extracted from this field using the standard marching cubes algorithm or an adaptive variant of it at an isovalue dictated by the filter. The detail of voxel-based meshes can be adjusted via resolution, but the method sees limited industrial use. It smooths sharp features making itself unsuitable for CAD and is computationally expensive due to volumetric processing, and lacks explicit geometric error control, making output quality difficult to guarantee. 

Recent work has explored neural methods that either simplify meshes directly or offer implicit representations enabling level-of-detail control. [Potamias et al. 2022] employs a differentiable neural network to select a subset of input vertices using a sparse attention mechanism and re-triangulate the selected vertices, producing simplified meshes in a data-driven, generalizable manner without per-mesh retraining. [Chen et al. 2023] generates a coarse base mesh using QEM, followed by neural remeshing through face splits. A per-face latent feature representation is transmitted and decoded on the client-side to reconstruct finer meshes. This approach implicitly generates simplified representations across multiple LODs. [Park et al. 2019] learns a signed distance field (SDF) representation from a voxelized representation of mesh. [Takikawa et al. 2021] extends it by creating multiscale SDFs giving real-time rendering at various LODs via ray marching. Although simplified triangle meshes can be extracted using methods like marching cubes, this undermines the efficiency of its implicit representation. 

4 

_3 DATA STRUCTURES REPRESENTING MESH CONNECTIVITY_ 

## **3. Data structures representing mesh connectivity** 

Mesh connectivity data structures are designed to efficiently organize and manage the relationships between elements of a mesh such as which faces share an edge, which edges are connected to a vertex, or which vertices make up a face. They allow algorithms to rapidly traverse and manipulate the mesh’s topology. Below are two data structures commonly used to represent mesh connectivity, along with an evaluation of their suitability for supporting edge collapse operations. 

The _Corner Table_ data structure introduced in [Rossignac 2002] is a compact mesh representation where each triangle’s three "corners" (vertex-triangle associations) are stored in a list. For edge collapse, it efficiently manages the edge-collapse updates and supports fast querying on the mesh. 

The _Half-Edge_ data structure presented in [McGuire 2000], is widely used due to its intuitive design and broad support across mesh libraries. In this structure, each mesh edge is represented by a pair of half-edges pointing in opposite directions, each storing connectivity to associated elements such as vertices, faces, and neighboring edges. While not the most memory-efficient option, it enables fast mesh queries and local updates, making it ideal for operations like edge collapse. 

In the code listing below, we present the interfaces that a typical connectivity data structure would support. 

**class** IVertex { **virtual** vec3 GetPosition() = 0; **virtual** vecn GetAttributes() = 0; }; **class** IEdge { **virtual** vector<IVertex*> GetVertices() = 0; }; **class** IFace { **virtual** vector<IVertex*> GetVertices() = 0; **virtual** vector<IEdge*> GetEdges() = 0; }; **class** IMesh { **virtual** vector<Face*> GetConnectedFaces(Edge* edge) = 0; **virtual** vector<Face*> GetConnectedFaces(Vertex* vertex) = 0; **virtual** vector<Edge*> GetConnectedEdges(Vertex* vertex) = 0; **virtual** vector<Vertex*> GetConnectedVertices(Vertex* vertex) = 0; }; 

5 

## _4 EDGE COLLAPSE-BASED SIMPLIFICATION ALGORITHM_ 

The queries listed in table. 1 are necessary for the edge collapse-based mesh simplification algorithm, so they must be handled efficiently by the chosen mesh connectivity data structure. As Table 1 illustrates, both the Half-Edge and Corner Table structures are adept at handling these queries with optimal time complexities, making them well-suited for edge-collapse based mesh simplification. 

|**Mesh Query**|**Time complexity** (half-edge / corner table)|
|---|---|
|Get all triangles connected to vertex𝑣|𝑂(degree(𝑣))|
|Get all edges connected to vertex𝑣|𝑂(degree(𝑣))|
|Get all vertices connected to vertex𝑣|𝑂(degree(𝑣))|
|Get all edges connected to vertex𝑣|𝑂(degree(𝑣))|



**Table 1** . Mesh query operations and their time complexities using different data structures 

## **4. Edge collapse-based simplification algorithm** 

Edge collapse-based simplification iteratively reduces the number of triangles in a mesh while preserving its overall shape and features. The core algorithm remains largely consistent across different implementations, with key differences lying in the cost metric and vertex placement strategies. The algorithm typically involves the following steps: 

## **Cost assignment and optimal vertex placement calculation** 

1. A cost is computed for each edge in the mesh to estimate the geometric error introduced by collapsing it. Simultaneously, the optimal position for the resulting merged vertex is determined. This step is critical, as it is where most edge collapse based simplification strategies diverge. 

2. The computed cost, along with the edge and its optimal replacement vertex, is stored in a priority queue. 

## **Iterative edge collapse** 

A target triangle count is either defined internally by the program or specified externally by the client code. Then, the following steps are repeated until the target triangle count is reached: 

1. Select the edge with the lowest collapse cost from the priority queue. 

2. Perform validity checks to ensure that collapsing that edge preserves the mesh’s manifoldness. (The three validity checks we employ are explained below.) 

6 

_4 EDGE COLLAPSE-BASED SIMPLIFICATION ALGORITHM_ 

3. If the edge passes all validity checks, collapse it by replacing the edge with the computed vertex and removing the two adjacent triangles. 

4. Since the collapse locally alters the mesh, recompute the costs of all the edges connected to the collapsed edge, and update the corresponding entries in the priority queue to maintain accuracy for the next iteration. 

5. Update the mesh’s connectivity data structure to reflect the changes made by the collapse. 

## **Edge collapse validity checks** 

Checks 1 and 2 follow the criteria established in [Hoppe et al. 1993], while check 3 is derived empirically. These checks are crucial for avoiding degeneracies that may result in invalid or non-manifold mesh structures. a[Ya)] 

**Figure 3** . Flipped red triangle, causing local mesh degeneracy 

1. **Triangle flip check** : Ensure that collapsing the edge does not invert the orientation of any surrounding triangles. An edge collapse that results in such an inversion is shown in Figure 3. 

**==> picture [340 x 68] intentionally omitted <==**

**----- Start of picture text -----**<br>
bool AreFacesFlipped( const IFace* old_face, const IFace*<br>new_face)<br>{<br>return dot(util::ComputeNormal(old_face), util::ComputeNormal<br>(new_face)) < 0.0;<br>}<br>**----- End of picture text -----**<br>


**Figure 4** . Non-manifold triangle formation caused by collapsing the red edge 

7 

_4 EDGE COLLAPSE-BASED SIMPLIFICATION ALGORITHM_ 

2. **Two-neighbor connectivity check** : Verify that exactly one pair of edges is merged on each side of the collapsing edge. This condition holds when the two collapsing vertices share exactly two common neighbors. A connectivity-related nonmanifold triangle formation is illustrated in Figure 4. 

**bool** HasMoreThanTwoNeighbors( **const** IMesh* mesh, **const** IEdge* collapse_edge) { **auto** vertices = collapse_edge->GetVertices(); **auto** conn_verts_v0 = mesh->GetConnectedVertices(vertices[0]); **auto** conn_verts_v1 = mesh->GetConnectedVertices(vertices[1]); set<IVertex*> conn_verts_v1_set(conn_verts_v1.begin(), conn_verts_v1.end()); **int** common_count = 0; **for** ( **auto** vert : conn_verts_v0) **if** (conn_verts_v1_set.contains(vert)) { common_count++; **if** (common_count > 2) **return true** ; } **return false** ; } 

**Figure 5** . Edge collapse merging internal boundaries forming ill-formed local mesh 

3. **Boundary merge check** : If both the other edges of a face besides the collapsing edge lie on mesh boundaries or holes, collapsing the edge can lead to merging of two boundaries causing an ill-formed structure as illustrated in Figure 5. 

**bool** HasMultipleConnectedBoundaries( **const** IMesh* mesh, **const** IEdge* collapse_edge) { **for** ( **auto** face : mesh->GetConnectedFaces(collapse_edge)) { **int** n_edges_on_boundary = 0; **for** ( **auto** edge : face->GetEdges()) { **if** (edge == collapse_edge) **continue** ; 

8 

_5 COST FUNCTIONS_ 

**if** (util::IsBoundaryEdge(edge)) n_edges_on_boundary++; } **if** (n_edges_on_boundary > 1) **return true** ; } **return false** ; } 

## **5. Cost functions** 

In edge collapse-based mesh simplification, an _error metric_ is assigned to each edge that estimates the cost of collapsing it. Edges with the lowest error are prioritized for collapse. Additionally, we need effective strategies to determine the _best new vertex position_ that will replace the collapsed edge while minimizing the geometric distortion. 

The IConstraint class below defines an interface for error metrics. The cost function classes implementing this interface compute the cost ℇ(𝑣) of collapsing an edge for a candidate vertex position 𝑣. Implementations of this class compute the cost as ℇ(𝑣) = 𝑣[𝑇] 𝐻𝑣+ 2𝑐[𝑇] 𝑣+ 𝑘, and store the entities {𝐻, 𝑐, 𝑘} in this equation in m_H, m_c, m_k. These will be used to obtain the optimal vertex placement as well, as detailed in section 7. 

**class** IConstraint { **public** : **virtual void** EvaluateCost( **const** vec3 &v) = 0; **const** mat3& GetH() **const** { **return** m_H; } **const** vec3& GetC() **const** { **return** m_c; } **private** : mat3 m_H; vec3 m_c; **double** m_k; } 

## 5.1. Plane-based quadric error metrics (QEM) 

This method, described in [Garland and Heckbert 1997], defines error as the sum of distances from the new vertex to the planes of surrounding triangles, treating each vertex as their intersection. This captures how much the new vertex deviates from the original geometry, reflecting the introduced distortion. 

Referring to Figure 6, we collapse the edge (𝑣1, 𝑣2) into a new vertex 𝑣, removing the two adjacent triangles(in planes 𝑃1 and 𝑃6) and forming a local geometric approximation. The error is measured as the sum of distances from 𝑣 to the original surrounding planes 𝑃1 through 𝑃10. 

9 

_5.1 Plane-based quadric error metrics (QEM)_ 

_5 COST FUNCTIONS_ 

**Figure 6** . Local structure around edge collapse for QEM 

Let ℙ= {𝑃1, 𝑃2, … , 𝑃𝑚}, the set of planes that surround the edge being collapsed. Let ℇ be the error introduced by the newly added vertex 𝑣, given by: 

**==> picture [98 x 25] intentionally omitted <==**

where (𝑛, 𝑑) represent the unit normal 𝑛 and scalar 𝑑 in the equation 𝑛·𝑟+𝑑= 0 of each plane 𝑃∈ℙ. 

The term 𝑛[𝑇] 𝑣+ 𝑑 gives the signed distance from the vertex to the plane. Squaring it ensures that the error is always non-negative, penalizing both positive and negative deviations equally. 

Expanding, 

**==> picture [232 x 104] intentionally omitted <==**

To minimize the error, we set its gradient to zero and solve for 𝑣: 

**==> picture [95 x 24] intentionally omitted <==**

**Note:** When ℇ takes this form, the matrix 𝐻 is its Hessian matrix. In this specific case, 𝐻 turns out to be positive semidefinite. So, the point that makes ∇ℇ= 0 corresponds to a minimum point rather than a maximum or a saddle point. 

If the matrix 𝐻 is non-invertible (i.e., det(𝐻) = 0), the optimal vertex position cannot be computed this way. In such cases, fallback strategies or alternative constraints are used. For this cost function, a non-invertible 𝐻 indicates that the surface surrounding the edge collapse is flat, as explained below: 

10 

_5.1 Plane-based quadric error metrics (QEM)_ 

_5 COST FUNCTIONS_ 

**==> picture [355 x 169] intentionally omitted <==**

each term of the sum is itself a non-invertible matrix, as all its columns are parallel to 𝑛. So, when 𝐻 is non-invertible, all the normals of the planes forming 𝐻 are parallel. This occurs if the local surface is flat. 

**Note:** The "quadric" in the name of this method is derived from the form this error takes when 𝑣 is represented in homogeneous 4-dimensional coordinates as the vector[In] 1[)][.] ([𝑣] that case, the error ℇ is expressed as follows: 

**==> picture [110 x 29] intentionally omitted <==**

𝑐 where the authors define the 4x4 matrix 𝑐[𝑇] 𝑘[)][ above as the] _[ total error quadric]_ 𝑄= ([𝐻] for this edge. It can further be decomposed as the sum of _fundamental error quadrics_ 𝐾𝑃 for each plane 𝑃∈ℙ: 

**==> picture [184 x 29] intentionally omitted <==**

However, the same authors in [Garland and Heckbert 1998] found this formulation impractical because it requires computationally expensive matrix operations on higherdimensional matrices, like inversion. For this reason, it won’t be discussed further. 

**class** QEM : IConstraint { 

**void** QEM( **const** IMesh* mesh, **const** IEdge* collapse_edge) { **auto** vertices = collapse_edge->GetVertices(); **auto** connected_faces = util::GetUnion( mesh->GetConnectedFaces(vertices[0]), mesh->GetConnectedFaces(vertices[1]) ); 

11 

_5 COST FUNCTIONS_ 

## _5.2 Boundary handling with QEM_ 

m_H = mat3(0); m_c = vec3(0); m_k = 0; **for** ( **auto** face : connected_faces) { **auto** face_normal = util::ComputeNormal(face); **auto** v0 = face->GetVertices()[0]; **double** d = -dot(face_normal, v0->GetPosition()); m_H += outerProduct(face_normal, face_normal); m_c += d * face_normal; k += d * d; } } **double** EvaluateCost( **const** vec3& v) **const** override { **return** transpose(v) * m_H * v + 2 * dot(m_c, v) + m_k; } } 

## 5.2. Boundary handling with QEM 

The standard QEM method struggles with boundary edges - those with only one adjacent face. As noted in [Garland and Heckbert 1998], a modified QEM was proposed to address this and preserve boundary edges. 

**Figure 7** . When a red edge is collapsed, QEM places the new vertex at the intersection of adjacent planes. This can cause the new vertex to be located away from the boundary 

Figure 7 illustrates the boundary-related limitations of the standard QEM approach. Consider the red boundary edge between 𝑣1 and 𝑣2, selected for collapse under two distinct surrounding geometries. The new vertex 𝑣 is computed as the intersection of the adjacent planes 𝑝1, 𝑝2 and 𝑝3 because the distance of that point from all these planes is zero - resulting in the minimum possible quadric error. Depending on their configuration, this intersection may lie above or below the original boundary. 

In conventional QEM, no explicit constraint anchors the new vertex to the boundary. Consequently, collapsing a boundary edge tends to displace the vertex away from the boundary, a deviation that compounds as more boundary edges are collapsed. This 

12 

_5.2 Boundary handling with QEM_ 

_5 COST FUNCTIONS_ 

progressive drift results in noticeable degradation of mesh quality, as seen in Figure 8. 

**Figure 8** . Comparison of QEM with and without boundary constraints 

**Figure 9** . Imaginary plane added for boundary handling 

To counteract this effect, [Garland and Heckbert 1998] introduces an imaginary plane 𝑝[′] as shown in Figure 9 in addition to the actual planes adjacent to the collapsing edge. 𝑝[′] is defined as the plane perpendicular to the mesh plane containing edge (𝑣1, 𝑣2). A new term, 𝑑[2] (𝑣, 𝑝[′] ), representing the squared distance between the vertex and 𝑝[′] , is incorporated into the error metric. As 𝑣 moves away from the boundary, this term increases, exerting a corrective pull toward the boundary. To strengthen this constraint, the quadric for 𝑝[′] is scaled by a large constant before being added to the quadrics of the edge endpoints. 

**Figure 10** . A mesh with vertex colors before and after simpification by enhanced QEM 

The method is further extended to treat edges separating faces with different attribute values (e.g., material indices) as boundaries. This ensures that such attribute boundaries 

13 

_5.3 Volume preservation constraint_ 

_5 COST FUNCTIONS_ 

are preserved during simplification, concentrating edges and faces along these divisions for improved alignment. An example of this extension is shown in Figure 10. 

**class** BoundaryQEM : QEM { **void** BoundaryQEM( **const** IMesh* mesh, **const** IEdge* collapse_edge) : QEM(mesh, collapse_edge) { **static const double** BOUNDARY_QUADRIC_SCALE = 10.0; _//_ scale factor for quadric of imaginary boundary planes **auto** vertices = collapse_edge->GetVertices(); **auto** connected_faces = GetUnion( mesh->GetConnectedFaces(vertices[0]), mesh->GetConnectedFaces(vertices[1]) ); 

**for** ( **auto** face : connected_faces) { **auto** face_normal = util::ComputeNormal(face); 

**for** ( **const auto** face_edge : face.GetEdges()) { **if** (!util::IsBoundaryEdge(face_edge)) **continue** ; 

**auto** edge_verts = face_edge->GetVertices(); vec3 edge_pos[2] = { edge_verts[0]->GetPosition(), edge_verts[1]->GetPosition() }; 

vec3 perp_to_face = normalize(cross(face_normal, edge_pos [1] - edge_pos[0])); 

**double** d = -dot(perp_to_face, edge_pos[0]); 

m_H += BOUNDARY_QUADRIC_SCALE * outerProduct(perp_to_face, perp_to_face); 

m_c += BOUNDARY_QUADRIC_SCALE * d * perp_to_face; k += BOUNDARY_QUADRIC_SCALE * d * d; } } } } 

## 5.3. Volume preservation constraint 

This constraint, introduced in [Lindstrom and Turk 1998] helps preserve the mesh volume. If the new vertex replacing the collapsed edge isn’t chosen carefully, it can distort the model. For instance, using the edge midpoint as the new vertex might increase the volume in concave areas or decrease it in convex ones. The goal of this constraint is to preserve volume locally at each collapse, thereby minimizing the overall volume change 

14 

_5.3 Volume preservation constraint_ 

_5 COST FUNCTIONS_ 

across the whole model. 

Neither boundary nor volume preservation guarantee geometric integrity; boundaries may deform, and surfaces can lose detail. However, these constraints serve as useful heuristics. Preserving simple, quantifiable properties like area and volume helps reduce extreme distortions, even if local features like sharp edges or curves are lost. While these constraints don’t capture fine geometric details, they provide an efficient way to maintain overall structure, balancing accuracy and performance without the complexity of exact boundary or volume preservation. 

When an edge 𝑒 is collapsed, it sweeps out a tetrahedral volume as illustrated in Figure 11, due to each triangle 𝑡 being replaced by a new triangle 𝑡[′] . 

**Figure 11** . Volume preservation - sweeping a tetrahedron 

Let 𝑡= [𝑣1, 𝑣2, 𝑣3], 𝑡[′] = [𝑣, 𝑣2, 𝑣3], and the volume swept by 𝑡 as 𝑣1 moves linearly to 𝑣 be 𝑉(𝑣, 𝑣1, 𝑣2, 𝑣3). 𝑉 is positive if 𝑣 is above the plane of 𝑡 and negative otherwise. 

Thus, to preserve the local volume at the site of an edge collapse, the sum of volumes of tetrahedra swept with all triangles 𝑇= {𝑡1, 𝑡2, ..., 𝑡𝑛} connected to edge 𝑒 are considered. The change in volume is given by (the superscript 𝑡 indicates the vertices belonging to triangle 𝑡): 

**==> picture [128 x 80] intentionally omitted <==**

Solving for ℇ= 0 and expanding the determinant along the fourth row, we get, 

**==> picture [375 x 45] intentionally omitted <==**

15 

_5.3 Volume preservation constraint 5 COST FUNCTIONS_ 

Representing the determinants that include 𝑣 as scalar triple products, we get, 

**==> picture [369 x 71] intentionally omitted <==**

**==> picture [323 x 90] intentionally omitted <==**

where 𝑛[𝑡] is the normal of the plane containing ⧍[(] 𝑣1[𝑡][, 𝑣] 2[𝑡][, 𝑣] 3[𝑡] ) with magnitude equal to triangle area. 

Substituting the above simplified term in Equation 1, we get, 

**==> picture [184 x 47] intentionally omitted <==**

The above equation is of the form 𝑣· 𝑁= 𝐷 which defines a plane. This implies that the vector 𝑣 is restricted to lie on a plane. So, any point on that plane will satisfy the equation above, implying that volume preservation alone is not enough to fully determine 𝑣: we need 2 other constraint equations to do so. 

**class** VolumePres : IConstraint { **void** VolumePres( **const** IMesh* mesh, **const** IEdge* collapse_edge) { m_H = mat3(0); m_c = vec3(0); m_k = 0; **auto** vertices = collapse_edge->GetVertices(); **auto** connected_faces = GetUnion( mesh->GetConnectedFaces(vertices[0]), mesh->GetConnectedFaces(vertices[1]) ); **for** ( **auto** face : connected_faces) { vec3 face_normal = util::ComputeNormal(face); **auto** fv = face->GetVertices(); vec3 positions[3] = { fv[0]->GetPosition(), fv[1]->GetPosition (), fv[2]->GetPosition() }; 

16 

_5.4 Volume optimization constraint_ 

_5 COST FUNCTIONS_ 

mat3 D(0); D[0] = positions[0]; D[1] = positions[1]; D[2] = positions[2]; **float** det = determinant(D); m_H[0] += face_normal; m_c[0] += det; } } **double** EvaluateCost( **const** vec3& v) **const** override { **return** 0; } } 

## 5.4. Volume optimization constraint 

Volume optimization finds the best new vertex position by minimizing the total unsigned volume change during an edge collapse. It fully determines the position, i.e., no extra constraints needed. 

By contrast, volume preservation only ensures that the total volume added and removed balances out to zero. And, as we know, it forces the vertex to lie on a plane but doesn’t tell us exactly where on that plane to place it, so it leaves some freedom. Moreover, it can lead to local distortions if large volumes are added and subtracted in different areas. 

From the volume preservation constraint formulation, we know that the change of volume induced by an edge collapse is: 

**==> picture [106 x 22] intentionally omitted <==**

where: 

**==> picture [86 x 13] intentionally omitted <==**

- 𝑡[′] = ⧍(𝑣, 𝑣2, 𝑣3) 

- 𝑉(𝑣, 𝑣1, 𝑣2, 𝑣3) is the volume swept out by 𝑡 when 𝑣1 moves in a linear path to 𝑣 

If the vertex 𝑣 is above the plane of a triangle 𝑡, the signed volume 𝑉 of the tetrahedron is positive. If below, it’s negative. But for optimization, we care about how much the volume changes, not the direction. So, we use the unsigned volume change. To get unsigned volume, we could use |𝑉| or 𝑉[2] . We use 𝑉[2] because it is differentiable everywhere. This matters because optimization algorithms rely on gradients and |𝑉| has a kink at zero where the gradient is undefined. 

17 

_5.4 Volume optimization constraint_ 

_5 COST FUNCTIONS_ 

So we express ℇ as the sum of squares of volumes instead. So we get, 

**==> picture [306 x 164] intentionally omitted <==**

and we get, 

**==> picture [48 x 9] intentionally omitted <==**

This constraint uniquely determines 𝑣, except in degenerate cases where det(𝐻) = 0. Just like the case of QEM, this happens in locally flat regions of the geometry, since we know that 𝐻 is defined as: 

**==> picture [66 x 21] intentionally omitted <==**

which has the form used in QEM. So, 𝐻 becomes non-invertible in flat regions, where all 𝑛𝑡 are parallel and the sum reduces to scaled rank-1 terms. In such situations, alternative constraints or fallback strategies are required for vertex placement. 

**class** VolumeOpt : IConstraint { **void** VolumeOpt( **const** IMesh* mesh, **const** IEdge* collapse_edge) { **auto** vertices = collapse_edge->GetVertices(); **auto** connected_faces = GetUnion( mesh->GetConnectedFaces(vertices[0]), mesh->GetConnectedFaces(vertices[1]) ); m_H = mat3(0); m_c = vec3(0); m_k = 0; **for** ( **const auto** face : connected_faces) { vec3 face_normal = util::ComputeNormal(face); **auto** fv = face->GetVertices(); 

18 

_5.5 Boundary preservation constraint_ 

_5 COST FUNCTIONS_ 

vec3 positions[3] = { fv[0]->GetPosition(), fv[1]->GetPosition (), fv[2]->GetPosition() }; mat3 D(0); D[0] = positions[0]; D[1] = positions[1]; D[2] = positions[2]; **float** det = determinant(D); m_H += outerProduct(face_normal, face_normal); m_c += det * face_normal; m_k += det * det; } m_c = -1.0 * m_c; } **double** EvaluateCost( **const** vec3& v) **const** override { **return** transpose(v) * m_H * v + 2.0 * dot(m_c, v) + m_k; } } 

## 5.5. Boundary preservation constraint 

This constraint is discussed in [Lindstrom and Turk 1998]. It helps compute optimal vertex placement and edge collapse error by preserving the area of boundaries. 

**Figure 12** . Boundary preservation example (areas not to scale) 

In Figure 12, the image on the left shows a boundary edge (in red) on a planar hole, while the image on the right shows it being replaced by a new vertex (red dot) after edge collapse. As a result, although the total shaded area is preserved (red area loss offset by blue area gain), the boundary’s shape and structure are visibly altered. Thus, the constraint preserves area, not the boundary itself. 

As per Figure 13, let edge (𝑣1, 𝑣2) be collapsed into vertex 𝑣 and let ℇ be the net area change. Collapsing a boundary edge connects the new vertex 𝑣 to two other boundary 

19 

_Boundary preservation constraint_ 

_5 COST FUNCTIONS_ 

_5.5_ 

**Figure 13** . Boundary preservation - planar boundary case 

edges, forming three triangles. The sum of their signed areas gives yields ℇ. 

**==> picture [238 x 62] intentionally omitted <==**

While planar boundaries help build intuition for area change during edge collapse, real boundaries are often non-planar. However, the same formulation works for nonplanar boundaries, as detailed in the appendix (section 9). 

## **Figure 14** . Boundary preservation - constraint derivation 

Let ℇ be the squared change in area from the edge collapse, and 𝐸= {𝑒1, … , 𝑒𝑛} the set of boundary edges connected to vertex 𝑣 with each edge 𝑒∶[(] 𝑣1[𝑒][, 𝑣] 2[𝑒] ) ∈𝐸. 

Collapsing a boundary edge connects two others, forming three triangles (𝑛= 3) as shown in Figure 14. The squared norm is used for computational simplicity. The error is then defined as the squared magnitude of the total area change induced by vertex 𝑣 as: 

20 

_5.5 Boundary preservation constraint_ 

_5 COST FUNCTIONS_ 

**==> picture [339 x 108] intentionally omitted <==**

**==> picture [175 x 20] intentionally omitted <==**

To simplify the cross-product term, 𝐸1 × 𝑣 can be written as 𝒮𝑣, where 𝒮 is the skewsymmetric matrix for the cross product with 𝐸1: 

**==> picture [173 x 141] intentionally omitted <==**

Simplifying the squared norm gives: 

which has the same form as in QEM, suggesting that the solution should be the same: 𝑣= −𝐻[−1] 𝑐. 

However, for this constraint, 𝐻 is non-invertible as 𝒮 is non-invertible, being a skewsymmetric matrix. Therefore, 𝑣 cannot be fully determined. Below is a demonstration of the constraints that can actually be extracted from setting the gradient to zero. 

**==> picture [405 x 140] intentionally omitted <==**

This gives us, 

21 

_5.5 Boundary preservation constraint_ 

_5 COST FUNCTIONS_ 

**==> picture [251 x 12] intentionally omitted <==**

This is a 3D vector equation in 𝑣, which can be split into 3 scalar equations to solve for its components. We can choose any basis for this, but for convenience, we choose: [𝐾, 𝐸1, 𝐸1 × 𝐾]. 

**Projecting Equation 2 onto** 𝐾 **gives us:** 

- 0= 𝐾· [𝑣(𝐸1 · 𝐸1) −𝐸1(𝐸1 · 𝑣) + 𝐾] 

- = 𝐾·𝑣[(] ‖𝐸1‖[2][) ] −𝐾· 𝐸1(𝐸1 · 𝑣) + 𝐾·𝐾 

**==> picture [336 x 44] intentionally omitted <==**

**Projecting Equation 2 onto** 𝐸1 **gives us:** 

**==> picture [298 x 59] intentionally omitted <==**

This simplifies to 0 = 0, which is a degenerate result. This indicates that the component of 𝑣 parallel to 𝐸1is not determined by this minimization problem, as it does not affect the value of the error function. 

**Projecting Equation 2 onto** 𝐸1 × 𝐾 **gives us:** 

**==> picture [180 x 10] intentionally omitted <==**

**==> picture [327 x 62] intentionally omitted <==**

Thus, the solution space of this optimization lies in the intersection of two planes defined by: 

**==> picture [154 x 27] intentionally omitted <==**

Even with this approach of minimizing ℇ, 𝑣 remains undetermined. Thus, additional constraints are needed alongside the two equations to solve for 𝑣. 

**class** BoundaryPres : IConstraint { **void** BoundaryPres( **const** IMesh* mesh, **const** IEdge* collapse_edge) { 

22 

_5 COST FUNCTIONS_ 

## _5.6 Boundary Optimization Constraint_ 

**if** (!util::IsBoundaryEdge(collapse_edge)) **return** ; m_H = mat3(0); m_c = vec3(0); m_k = 0; **auto** vertices = collapse_edge->GetVertices(); **auto** connected_edges = GetUnion( mesh->GetConnectedEdges(vertices[0]), mesh->GetConnectedEdges(vertices[1]) ); vec3 E1(0); vec3 E2(0); **for** ( **auto** edge : connected_edges) { **if** (!util::IsBoundaryEdge(edge)) **continue** ; **const auto** verts = edge->GetVertices(); vec3 positions[2] = { verts[0]->GetPosition(), verts[1]-> GetPosition() }; E1 += positions[0] - positions[1]; E2 += cross(positions[0], positions[1]); } mat3 skew_sym_mat = util::MakeSkewSymMat(E1); H += transpose(skew_sym_mat) * skew_sym_mat; c += skew_sym_mat * E2; k += dot(E2, E2); } **double** EvaluateCost( **const** vec3& v) **const** override { **return** 0.25 * (transpose(v) * m_H * v) + 0.5 * dot(m_c, v) + 0.25 * m_k; } } 

## 5.6. Boundary Optimization Constraint 

The boundary optimization constraint introduced in [Lindstrom and Turk 1998] minimizes boundary triangle area like boundary preservation, but focuses on unsigned area. The error is expressed as a sum of squared signed areas, giving: 

**==> picture [239 x 16] intentionally omitted <==**

where, 

**==> picture [43 x 9] intentionally omitted <==**

23 

_5.6 Boundary Optimization Constraint_ 

_5 COST FUNCTIONS_ 

**==> picture [214 x 13] intentionally omitted <==**

**==> picture [205 x 14] intentionally omitted <==**

Expanding Equation 3 gives: 

**==> picture [172 x 19] intentionally omitted <==**

**==> picture [298 x 93] intentionally omitted <==**

To simplify the cross-product term, 𝑣× 𝑒1 can be written as 𝒮𝑣, where 𝒮 is the skewsymmetric matrix for the cross product with 𝑒1. Also, the scalar triple product identity can be used to rearrange 2(𝑣× 𝑒1)[𝑇] 𝑒2 = 2(𝑒1 × 𝑒2)[𝑇] 𝑣. 

**==> picture [270 x 89] intentionally omitted <==**

which is of the same form as earlier constraints, so we obtain 𝑣 as 𝑣= −𝐻[−1] 𝑐. 

As in earlier constraints, if det(𝐻) = 0, the constraint becomes degenerate and we need to use other constraints or use fallback strategies for vertex placement. 

**class** BoundaryOpt : IConstraint { **void** BoundaryOpt( **const** Mesh* mesh, **const** IEdge* collapse_edge) { **if** (!util::IsBoundaryEdge(collapse_edge)) **return** ; **auto** vertices = collapse_edge->GetConnectedVertices(); **auto** connected_edges = GetUnion( mesh->GetConnectedEdges(vertices[0]), mesh->GetConnectedEdges(vertices[1]) ); m_H = mat3(0); m_c = vec3(0); m_k = 0; **for** ( **auto** edge : connected_edges) { **if** (!util::IsBoundaryEdge(edge)) **continue** ; 

24 

_5.7 Triangle Shape Optimization Constraint_ 

_5 COST FUNCTIONS_ 

**const auto** verts = edge->GetVertices(); vec3 positions[2] = { verts[0]->GetPosition(), verts[1]-> GetPosition()}; vec3 e1 = positions[0] - positions[1]; vec3 e2 = cross(positions[0], positions[1]); mat3 skew_sym_mat = util::MakeSkewSymMat(E1); m_H += transpose(skew_sym_mat) * skew_sym_mat; m_c += cross(e1, e1); m_k += dot(e2, e2); } } **double** EvaluateCost( **const** vec3& v) **const** override { **return** 0.25 * (transpose(v) * m_H * v) + 0.5 * dot(m_c, v) + 0.25 * m_k; } } 

## 5.7. Triangle Shape Optimization Constraint 

Triangle shape optimization tries to improve triangle quality. Skinny or stretched triangles can cause shading issues, while more even, equilateral triangles make the mesh look and work better. 

**Figure 15** . Edge collapse yielding low-quality (left) vs. high-quality (right) triangle shapes 

As shown in Figure 15, the vertex placement on the left side leads to a cleaner triangle structure thanks to its more regular, evenly shaped triangles. The placement on the right side contains long, stretched triangles, which make the mesh look less tidy and visually less appealing. 

Before analyzing this further, an important triangle shape quality metric needs to be introduced: the **area-to-perimeter ratio** . Regular triangles (like equilateral ones) have 

25 

_5.7 Triangle Shape Optimization Constraint_ 

_5 COST FUNCTIONS_ 

a higher area-to-perimeter ratio, which means they’re more compact and less stretched. 

In the triangle shape preservation constraint, the goal is to maximize the area-toperimeter ratio. We make an assumption that the region around the collapsing edge is nearly flat. This means the total area of the nearby triangles doesn’t change much after the edge collapse. So, to improve their area-to-perimeter ratio, we can focus on reducing the perimeter alone, which is determined by the edge lengths. 

That’s why we minimize the sum of the squared lengths of the edges connected to the new vertex. This pulls the vertex into a position where the edges are more evenly distributed and shorter, leading to more balanced, less skinny triangles. 

We formulate this error ℇ as: 

**==> picture [89 x 14] intentionally omitted <==**

where 𝑣𝑖 refers to each neighboring vertex {𝑣1, … , 𝑣𝑛} connected to 𝑣 in the mesh. 

On expanding this equation for ℇ, we get, 

**==> picture [316 x 101] intentionally omitted <==**

which can be solved in the same way as in other constraints. Here, it leads to this solution: 

**==> picture [191 x 22] intentionally omitted <==**

Thus, optimizing triangle shape will lead to choosing the centroid of the neighboring vertices. 

Skinny triangles are avoided because they cause shading artifacts. Rasterization interpolates per-vertex data (e.g., normals) using barycentric coordinates, defined as: 

**==> picture [287 x 26] intentionally omitted <==**

In skinny triangles, the denominator 𝐴𝑟𝑒𝑎(⧍𝐴𝐵𝐶) becomes very small, making the coordinates numerically unstable. Small floating-point errors in the vertex positions or sub-areas can then cause large interpolation errors, producing shading artifacts. 

**class** TriShapeOpt : IConstraint { 

**void** TriShapeOpt( **const** IMesh* mesh, **const** IEdge* collapse_edge) { **auto** vertices = collapse_edge->GetVertices(); 

26 

## _5.8 Fallback strategies 6 CONSTRAINT SELECTION CRITERIA_ 

**auto** connected_verts = GetUnion( mesh->GetConnectedVertices(vertices[0]), mesh->GetConnectedVertices(vertices[1]) ); m_H = mat3(0); m_c = vec3(0); m_k = 0; **for** ( **const auto** vert : connected_verts) { m_H += mat3(1); _//_ Identity matrix vec3 pos = vert.GetPosition(); m_c += -pos; m_k += dot(pos, pos); } } **double** EvaluateCost( **const** vec3& v) **const** override { **return** transpose(v) * m_H * v + 2.0 * dot(m_c, v) + m_k; } } 

## 5.8. Fallback strategies 

When the matrix used to compute 𝑣 is non-invertible, fallback strategies are needed. A simple and common fallback is to place the new vertex at the midpoint of the edge (𝑣1, 𝑣2) being collapsed: 

**==> picture [51 x 20] intentionally omitted <==**

vec3 GetFallbackVertex(IEdge* collapse_edge) { **auto** verts = collapse_edge->GetVertices(); **return** 0.5 * (verts[0]->GetPosition() + verts[1]->GetPosition()); } 

## **6. Constraint selection criteria** 

To solve for the new vertex 𝑣, a system of linear equations is formed from several constraints as discussed in the earlier sections. Each linear equation is of the form: 

**==> picture [40 x 9] intentionally omitted <==**

Here, each 𝑎 represents the normal of a constraint plane, and 𝑏 is the corresponding offset. Geometrically, we are finding the point 𝑣 that lies at the intersection of all these planes in ℝ[3] . 

27 

_6 CONSTRAINT SELECTION CRITERIA_ 

In theory, only three linearly independent constraints (planes) are needed to uniquely determine a point in 3D. However, the algorithm includes more than three constraints to ensure robustness. That’s because: 

- some constraints may become redundant (linearly dependent). 

- some may become degenerate in flat or symmetric regions. 

The final vertex placement includes only the best three by checking for linear independence and stability using the following criteria while adding constraints one by one: **First Constraint(** 𝐚𝟏 **): Is it valid?** 

𝑎1 ≠0 

**bool** IsFirstConstraintValid( **const** vec3 &A, **const** mat3 &H, **const** vec3 &c) { **return** length(A) > 0; } 

## **Second Constraint(** 𝐚𝟐 **): Is it linearly independent from the first?** 

## (𝑎1 · 𝑎2)[2] < (‖𝑎1‖ ‖𝑎2‖ cos(𝛼))[2] 

This checks if the angle 𝜃 between the first and second constraint normals is sufficiently large, that is, that they are not almost parallel. 𝛼 is a threshold angle used to determine acceptable linear independence. 

**bool** IsSecondConstraintValid( **const** vec3 &A, **float** alpha, **const** mat3 &H, **const** vec3 &c) { **float** lhs = dot(H[0], A); **float** rhs = length(H[0]) * length(A) * cos(alpha); **return** lhs * lhs < rhs * rhs; } 

## **Third Constraint(** 𝐚𝟑 **): Is it not coplanar with the first two?** 

## ((𝑎1 × 𝑎2) · 𝑎3)[2] <[(] ‖𝑎1 × 𝑎2‖ ‖𝑎3‖ sin(𝛼)[)][2] 

This ensures that the third constraint’s normal 𝑎3 does not lie in the plane formed by the normals of the first two constraints, again up to a threshold angle 𝛼. This guarantees that the three planes intersect at a single point in 3D space, defining a unique solution for the new vertex. 

Note that, here we use sin(𝛼) and not cos(𝛼) when computing the dot product. We define 𝛼 as the angle between the plane formed by 𝑎1 and 𝑎2 and the new vector 𝑎3. So as 

28 

_7 VERTEX PLACEMENT_ 

**Figure 16** . Visualization of 3 constraint normals 

Figure 16 suggests, the angle between the vectors 𝑎1 × 𝑎2 and 𝑎3 will be 90° −𝛼. So we have, 

**==> picture [184 x 27] intentionally omitted <==**

**bool** IsThirdConstraintValid( **const** vec3 &A, **float** alpha, **const** mat3 & H, **const** vec3 &c) { vec3 p = cross(H[0], H[1]); **float** lhs = dot(p, A); **float** rhs = length(p) * length(A) * sin(alpha); **return** lhs * lhs < rhs * rhs; } 

## **7. Vertex placement** 

Putting it all together, we revisit the overall edge collapse algorithm and implement all its components. We begin by computing the constraints described above in an order that best suits our use case. Next, we apply the constraint selection criteria to form a set of solvable constraints. Finally, we compute the optimal vertex position based on this set, resorting to our fallback strategy if the system remains unsolvable. The resulting optimal position and its associated cost are then returned for further handling in the priority queue. 

**void** GetCollapseVertex( **const** IMesh* mesh, **const** IEdge &collapse_edge , vec3 &collapse_vertex, **float** &collapse_error) { vector<IConstraint> constraints; 

29 

_7 VERTEX PLACEMENT_ 

IConstraint qem = QEM(mesh, collapse_edge); constraints.add(qem); IConstraint volume_pres = VolumePres(mesh, collapse_edge); constraints.add(volume_pres); IConstraint volume_opt = VolumeOpt(mesh, collapse_edge); constraints.add(volume_opt); IConstraint boundary_qem = BoundaryQEM(mesh, collapse_edge); constraints.add(boundary_qem); IConstraint boundary_pres = BoundaryPres(mesh, collapse_edge); constraints.add(boundary_pres); IConstraint boundary_opt = BoundaryOpt(mesh, collapse_edge); constraints.add(boundary_opt); IConstraint tri_opt = TriangleOpt(mesh, collapse_edge); constraints.add(tri_opt); 

**int** n_eqns = 0; **float** alpha = radians(5.0); _//_ 5 degrees, in radians mat3 H; vec3 c; **for** ( **auto** constraint : constraints) { **if** (n_eqns == 3) **break** ; mat3 constr_H = constraint.GetH(); vec3 constr_c = constraint. GetC(); **for** ( **int** i = 0; i < 3; i++) { vec3 A = constr_H[i]; **float** b = constr_C[i]; **if** (n_eqns == 0 && IsFirstConstraintValid(A, H, c)) { H[0] = A; c[0] = b; n_eqns++; } **else if** (n_eqns == 1 && IsSecondConstraintValid(A, alpha, H, c )) { H[1] = A; c[1] = b; n_eqns++; } **else if** (n_eqns == 2 && IsThirdConstraintValid(A, alpha, H, c) ) { H[2] = A; c[2] = b; n_eqns++; **break** ; } } 

30 

_8 HANDLING MESH ATTRIBUTES_ 

} 

collapse_vertex = determinant(H) == 0.0 ? GetFallbackVertex( collapse_edge) : inverse(H) * c; 

collapse_error = 0; **for** ( **auto** constraint : constraints) { _//_ Note: The ‘EvaluateCost‘ function can be weighted here. _//_ The weights can be user-defined to prioritize _//_ different types of mesh deformation minimization _//_ (e.g., prioritizing volume preservation _//_ over triangle quality) collapse_error += constraint.EvaluateCost(collapse_error); } } 

## **8. Handling mesh attributes** 

The focus of the paper so far has been solely on the geometry and topology of 3D meshes during simplification. However, real-world meshes often include attributes like per-vertex colors and normals or per-face material indices used in rendering. When simplifying such meshes via edge collapse, it’s crucial to preserve attribute consistency at new vertices to avoid visual artifacts like color seams. The following sections explore key methods addressing this challenge. 

## 8.1. Higher-dimensional quadrics for QEM 

[Garland and Heckbert 1998] presented a modification to the QEM method that incorporates vertex attributes in addition to position when computing the new vertex in an edge collapse. This approach uses higher-dimensional quadrics, in which each vertex is expressed as a vector in a higher-dimensional space ℝ[𝑛] , 𝑛> 3, combining its position with all associated attributes. For instance, if a vertex at (𝑥, 𝑦, 𝑧) has only color attributes (𝑅, 𝐺, 𝐵), it is represented as a vector (𝑥, 𝑦, 𝑧, 𝑅, 𝐺, 𝐵) ∈ℝ[6] . 

Even when vertex coordinates are given in more than three dimensions, the QEM error measure (sum of distances of the new point from adjacent planes) can still be used. The distance to each plane is now computed using 𝑛 dimensions but is still with respect to a 2D plane, because three non-collinear points define a plane no matter how many dimensions they are in. 

To compute the distance of a point 𝑣 from a plane, we first determine a vector 𝑢 from 𝑣 to the plane that is perpendicular to its surface. To do this, we define an orthonormal basis in which two basis vectors span the plane and denote these as 𝑒1 and 𝑒2. These two vectors are part of a complete orthonormal basis {𝑒1, 𝑒2, … , 𝑒𝑛} for all of ℝ[𝑛] . The exact values of {𝑒3, … , 𝑒𝑛} need not be known. We only need to know that such a basis exists. 

31 

_8.1 Higher-dimensional quadrics for QEM8 HANDLING MESH ATTRIBUTES_ 

If needed, they can be determined using the Gram-Schmidt process, which begins with 𝑛 linearly independent vectors and iteratively removes components parallel to previously computed basis vectors. 

Let the plane be determined by three points 𝑝, 𝑞, 𝑟∈ℝ[𝑛] . 

**Figure 17** . Local frame of reference in ℝ[𝑛] 

As shown in Figure 17, two orthogonal axes 𝑒1 and 𝑒2 can be defined on the plane spanned by (𝑝, 𝑞, 𝑟): 

**==> picture [263 x 27] intentionally omitted <==**

- Using the Gram-Schmidt process, 𝑒2 can be obtained by taking the vector 𝑟−𝑝, removing its projection onto 𝑒1 (to ensure orthogonality with 𝑒1), and then normalizing. In other words, 

**==> picture [141 x 29] intentionally omitted <==**

Next, a point 𝑝 lying on the plane is chosen, and the vector from 𝑝 to 𝑣, i.e 𝑤= 𝑣−𝑝, is expressed as the sum of its components along the orthonormal basis {𝑒1, 𝑒2, … , 𝑒𝑛}: 

**==> picture [78 x 14] intentionally omitted <==**

Next, the components along the plane i.e (𝑒1, 𝑒2) are removed from 𝑤, giving us a vector 𝑢: 

**==> picture [272 x 11] intentionally omitted <==**

Here, 𝑢 is the perpendicular from 𝑣 to the plane, whose squared length is precisely ℇ: 

32 

_8.2 Energy-based cost computation_ 

_8 HANDLING MESH ATTRIBUTES_ 

ℇ= 𝑢· 𝑢 

**==> picture [346 x 62] intentionally omitted <==**

Expanding this out further using 𝑤= 𝑣−𝑝, we get: 

ℇ= (𝑣−𝑝) · (𝑣−𝑝) −((𝑣−𝑝) · 𝑒1)[2] −((𝑣−𝑝) · 𝑒2)[2] 

= (𝑣· 𝑣+ 𝑝· 𝑝−2𝑝· 𝑣) −(𝑒1 · 𝑣−𝑒1 · 𝑝)[2] −(𝑒2 · 𝑣−𝑒2 · 𝑝)[2] 

**==> picture [441 x 64] intentionally omitted <==**

Note that this expression matches the form of the error used in the original QEM method. This means that, aside from differences in the values and dimensions of the entities {𝐻, 𝑐, 𝑘}, the procedure for calculating the cost and determining the optimal 𝑣 remains unchanged: the optimal 𝑣 is still given by −𝐻[−1] 𝑐. 

Furthermore, once computed, 𝑣 not only represents the optimal vertex position but also encodes the optimal values for all associated scalar attributes. 

## 8.2. Energy-based cost computation 

[Hoppe 1996] introduced progressive meshes - sequences of meshes representing varying levels of detail of an input mesh, each created through successive edge collapse operations. They propose an alternative method to compute the cost of each edge collapse by defining it as the difference in an energy function. The cost reflects how much the energy function of the mesh changes before and after the collapse, with edges causing smaller energy differences considered better candidates for collapse. Their approach also handles discrete face attributes and scalar vertex attributes at each level of detail. 

## _8.2.1. Prerequisites_ 

Before introducing the cost function, we define key geometric entities and setup steps needed to compute the cost of an edge collapse. Figure 18 shows an example. 

## **Original mesh definition:** 

The original mesh (before any edge collapse operations) is denoted as: 

**==> picture [72 x 12] intentionally omitted <==**

- **Vertices:** 𝑉= {𝑣[̂] 1, … , 𝑣𝑛} , where each 𝑣𝑖 ∈ℝ[3] 

33 

_8.2 Energy-based cost computation 8 HANDLING MESH ATTRIBUTES_ 

- **Faces:** 𝐹= {𝑓[̂] 1, … , 𝑓𝑚}, with each 𝑓𝑖 ∈{1, … , 𝑛}[3] 

- **Vertex attributes:** 𝑉[̂] = 𝑣1, … , 𝑣𝑛 , with each 𝑣𝑖 ∈ℝ[𝑑] (for example, 𝑑= 3 for { } 

- RGB colors) 

- **Face attributes:** 𝐹[̂] = 𝑓1, … , 𝑓𝑚 i, with each 𝑓𝑖 ∈ℤ[𝑑][′] (for example, 𝑑[′] = 1 for { } 

- material indices) 

## **Setup for simplification:** 

Before simplification, the following steps are performed: 

## 1. **Surface sampling:** 

Sample a set 𝑋 of 𝑘 points on the surface of 𝑀[̂] : 

𝑋= {𝑥1, … , 𝑥𝑘}, 𝑥𝑖 ∈ℝ[3] 

These points will serve to approximate 𝑀[̂] in the energy functions. 

## 2. **Attribute sampling:** 

For each 𝑥𝑖, compute its scalar attribute 𝑥𝑖 ∈ℝ[𝑑] via barycentric interpolation on its containing face yielding the attribute set 𝑋 corresponds to 𝑋: 

𝑋 = 𝑥1, … , 𝑥𝑘 , 𝑥𝑖 ∈ℝ[𝑑] { } 

## 3. **Sharp-edge sampling:** 

Sample an additional set 𝑋[′] of 𝑘[′] points constrained to lie only on _sharp edges_ on the surface of 𝑀[̂] : 

**==> picture [109 x 14] intentionally omitted <==**

_Sharp edges_ are comprised of: 

- (a) Material boundaries: edges between faces with 𝑓𝑖 ≠𝑓𝑗. 

- (b) Geometric boundaries: Edges connected to only one face. 

**Figure 18** . An example showing the original mesh and sampled point sets 

34 

_8.2 Energy-based cost computation_ 

_8 HANDLING MESH ATTRIBUTES_ 

## _8.2.2. Cost function definitions_ 

Consider the mesh 𝑀[̂] after some edge collapses. Let 𝑀[↑] be the state before collapsing another edge, and 𝑀 the state after edge collapse. The cost for this collapse is formulated as: 

**==> picture [400 x 43] intentionally omitted <==**

## **Distance energy:** 

𝐸𝑑𝑖𝑠𝑡 measures geometric fidelity by penalizing the distance between each sampled point 𝑥𝑖 ∈𝑋 and its projection 𝑝(𝑥𝑖) on the mesh 𝑀. 

**==> picture [248 x 23] intentionally omitted <==**

## **Spring energy:** 

𝐸𝑠𝑝𝑟𝑖𝑛𝑔 regularizes the optimization by treating each edge as a "spring", contributing its squared length to the energy. 

**==> picture [273 x 23] intentionally omitted <==**

Here, the spring constant 𝜅 weights this term. 

## **Scalar attribute energy:** 

𝐸𝑠𝑐𝑎𝑙𝑎𝑟 is similar to 𝐸𝑑𝑖𝑠𝑡, but for scalar attributes. It measures the squared difference between the attributes of each sampled point 𝑥𝑖 and those of its projection 𝑝(𝑥𝑖): 

**==> picture [265 x 27] intentionally omitted <==**

where 𝑐𝑠𝑐𝑎𝑙𝑎𝑟 weights this term. 

## **Discontinuity preservation offset:** 

𝐷𝑑𝑖𝑠𝑐 penalizes collapsing a _sharp edge_ 𝑒 that affects discontinuities tracked by 𝑋[′] : 

**==> picture [251 x 12] intentionally omitted <==**

Here, numProject(𝑋[′] , 𝑒) denotes the number of points in 𝑋[′] projecting onto 𝑒. This term is applied only if the collapse alters the discontinuity connectivity, based on criteria presented in [Hoppe 1996]. To forbid the collapses entirely, set 𝐷𝑑𝑖𝑠𝑐 = ∞. 

## **Cost function evaluation and optimal vertex placement:** 

35 

_8.2 Energy-based cost computation_ 

_8 HANDLING MESH ATTRIBUTES_ 

Recall that 𝑀[↑] is the state of the mesh before collapsing the current edge, and 𝑀 the mesh after the collapse. The new vertex is placed at position 𝑣 with attributes 𝑣. We need to compute 𝑣 and 𝑣 that minimize the cost ℇ. 

As per Equation 4, the terms in ℇ depend on 𝑣 and 𝑣 as follows: 

- 𝐸𝑑𝑖𝑠𝑡(𝑀) and 𝐸𝑠𝑝𝑟𝑖𝑛𝑔(𝑀) depend only on 𝑣. 

- 𝐸𝑠𝑐𝑎𝑙𝑎𝑟(𝑀) depends only on 𝑣. 

- 𝐷𝑑𝑖𝑠𝑐 and the energy terms for 𝑀[↑] are independent of both 𝑣 or 𝑣. So, they only affect the cost ℇ but not the optimization. 

Thus, here are the steps to compute ℇ, 𝑣 and 𝑣 for a given edge collapse: 

1. Minimize ∆𝐸𝑑𝑖𝑠𝑡 + ∆𝐸𝑠𝑝𝑟𝑖𝑛𝑔 over 𝑣. 

2. Minimize ∆𝐸𝑠𝑐𝑎𝑙𝑎𝑟 over 𝑣. 

3. If the discontinuity criteria hold, compute 𝐷𝑑𝑖𝑠𝑐. 

4. Return ℇ= ∆𝐸𝑑𝑖𝑠𝑡 + ∆𝐸𝑠𝑝𝑟𝑖𝑛𝑔 + ∆𝐸𝑠𝑐𝑎𝑙𝑎𝑟 + 𝐷𝑑𝑖𝑠𝑐 along with the optimal 𝑣 and 𝑣. 

## _8.2.3. Optimization process_ 

Minimizing the error ℇ is more complex than previous error functions and requires a staged optimization process. We describe that process in detail below, and the data required for computing and optimizing the error is shown in an example in Figure 19. 

**Figure 19** . An example showing data required for 𝐸𝑑𝑖𝑠𝑡 and 𝐸𝑠𝑝𝑟𝑖𝑛𝑔 

**Minimizing** ∆𝐸𝑠𝑝𝑟𝑖𝑛𝑔 **over** 𝑣 

**==> picture [304 x 58] intentionally omitted <==**

36 

_8.2 Energy-based cost computation 8 HANDLING MESH ATTRIBUTES_ 

We can observe that any edge with unchanged endpoints in 𝑀 and 𝑀[↑] cancels out in ∆𝐸𝑠𝑝𝑟𝑖𝑛𝑔. Since differences occur only near the collapsed edge, only two edge sets contribute nonzero terms: 

1. 𝑁: edges in 𝑀 incident on the new vertex at 𝑣. 

2. 𝑁[↑] : edges in 𝑀[↑] incident on the collapsed edge. 

Thus, we can simplify ∆𝐸𝑠𝑝𝑟𝑖𝑛𝑔 as follows: 

**==> picture [309 x 43] intentionally omitted <==**

where 𝑣 appears explicitly as all edges in 𝑁 are incident on it. 

Since 𝐸𝑠𝑝𝑟𝑖𝑛𝑔 (𝑀↑) is independent of 𝑣, it is ignored in the optimization process and added only to the final cost. 

The spring term 𝐸𝑠𝑝𝑟𝑖𝑛𝑔(𝑀), 

**==> picture [142 x 23] intentionally omitted <==**

is a quadratic in 𝑣 and can be written as 

**==> picture [422 x 130] intentionally omitted <==**

**Minimizing** ∆𝐸𝑑𝑖𝑠𝑡 **over** 𝑣 

**==> picture [194 x 44] intentionally omitted <==**

where the projection of the same sampled point 𝑥𝑖 is 𝑝(𝑥𝑖) in 𝑀 and 𝑝[↑] (𝑥𝑖) in 𝑀[↑] . 

Since 𝑀 and 𝑀[↑] differ only in the vicinity of the one edge being collapsed, most points project identically in both states and cancel out. Thus, only points 𝑌⊆𝑋 projecting onto the neighborhood of the edge contribute: 

**==> picture [282 x 27] intentionally omitted <==**

37 

_8.2 Energy-based cost computation_ 

_8 HANDLING MESH ATTRIBUTES_ 

We define the neighborhood of an edge as the set of faces connected to either endpoint of that edge in 𝑀 or 𝑀[↑] . 

This simplification is based on a _locality assumption_ : the new vertex 𝑣 stays near the collapsed edge, so the projections of distant points in 𝑋 remain unchanged. Although allowing 𝑣 farther away could lower the cost, it would require recomputing over all 𝑋. In practice, restricting 𝑣 and using the simplified ∆𝐸𝑑𝑖𝑠𝑡 works well. 

Since 𝐸𝑑𝑖𝑠𝑡 (𝑀↑) is independent of 𝑣, it is ignored in the optimization process and added only to the final cost. 

Now, although the 𝐸𝑑𝑖𝑠𝑡(𝑀) term, 

**==> picture [122 x 23] intentionally omitted <==**

appears independent of 𝑣, each projection 𝑝(𝑦𝑖) depends on 𝑣. 

For a point 𝑦, the projection 𝑝(𝑦) is 

**==> picture [103 x 17] intentionally omitted <==**

where 𝑝 lies on some face in 𝑀 with vertices (𝑣𝑎, 𝑣𝑏, 𝑣𝑐). We use barycentric coordinates 𝛽= (𝛽𝑎, 𝛽𝑏, 𝛽𝑐) to express 𝑝: 

**==> picture [228 x 13] intentionally omitted <==**

We can define a function 𝛽(𝑦) to denote the projected barycentric coordinates for any point 𝑦: 

**==> picture [196 x 18] intentionally omitted <==**

Now, since both 𝑉 and 𝛽 are face-specific, we extend 𝑉 to the full mesh 𝑀 with 𝑛 vertices {𝑣1, … , 𝑣, … , 𝑣𝑛} and any given 𝛽 to an 𝑛-dimensional vector, zeroing out the entries for all vertices except (𝑣𝑎, 𝑣𝑏, 𝑣𝑐) 

**==> picture [170 x 12] intentionally omitted <==**

Thus, 𝐸𝑑𝑖𝑠𝑡(𝑀) can be rewritten in terms of 𝑣 (as contained within 𝐕): 

**==> picture [332 x 23] intentionally omitted <==**

We can now minimize 𝐸𝑑𝑖𝑠𝑡(𝑀) over 𝑣. Since evaluating it requires projecting each 𝑦𝑖 via an inner minimization in β space, the problem becomes nested: an outer minimization over 𝑣 and inner minimizations over all β(𝑦𝑖). 

We solve the nested minimization iteratively as shown in Figure 20, starting with an initial guess for 𝑣 and alternating between: optimizing 𝑣 with fixed β(𝑦𝑖), then updating β(𝑦𝑖) with fixed 𝑣. This repeats until convergence, i.e., when the values of 𝑣 and each β(𝑦𝑖) don’t change much between iterations. In practice, a small number of iterations is sufficient. 

The inner minimization - over each β(𝑦𝑖) with fixed 𝑣 - is called the _projection subproblem,_ i.e., projecting all points in 𝑌 onto 𝑀. A brute-force method is to try projecting 

38 

_8.2 Energy-based cost computation 8 HANDLING MESH ATTRIBUTES_ 

**Figure 20** . Alternating optimization of 𝑣 and each β(𝑦𝑖) 

every 𝑦𝑖 on every face of 𝑀 and compute β(𝑦𝑖) corresponding to the closest face. But, [Hoppe 1996] adds two speedups to this approach: 

1. Use a spatial partitioning structure to find candidate faces in 𝑂(1) time per point, especially useful early on or after edge collapses in new regions. 

2. If 𝑦𝑖 was projected onto 𝑀[↑] , limit its projection on 𝑀 to the faces neighboring the previous one, leveraging locality. 

The outer minimization - over 𝑣 while keeping all β(𝑦𝑖) constant - is now solved by rewriting 𝐸𝑑𝑖𝑠𝑡(𝑀) using that constancy. 

**==> picture [342 x 57] intentionally omitted <==**

where β𝑣(𝑦𝑖) represents 𝑣[′] 𝑠 component in β(𝑦𝑖). So we have, 

**==> picture [412 x 148] intentionally omitted <==**

This yields the same quadratic error form as for 𝐸𝑠𝑝𝑟𝑖𝑛𝑔(𝑀), allowing us to jointly optimize both by summing their 𝐻, 𝑐, 𝑘 coefficients and solving for 𝑣. 

**Minimizing** ∆𝐸𝑠𝑐𝑎𝑙𝑎𝑟 **over** 𝑣 

39 

_8.2 Energy-based cost computation 8 HANDLING MESH ATTRIBUTES_ 

The cost function ∆𝐸𝑠𝑐𝑎𝑙𝑎𝑟 is analogous to 𝐸𝑑𝑖𝑠𝑡 and shares its locality assumption, giving the expression 

**==> picture [303 x 45] intentionally omitted <==**

Here, 𝑌 ⊆ 𝑋 is a local subset of sample attribute vectors. We define this neighborhood as the set of all sample points on the faces adjacent to the edge being collapsed, consistent with the approach used for 𝐸𝑑𝑖𝑠𝑡. 

Since 𝐸𝑠𝑐𝑎𝑙𝑎𝑟 (𝑀↑) is independent of 𝑣, it is ignored in the optimization process and added only to the final cost. 

The optimization is simplified by reusing the barycentric coordinate sets computation β and β[↑] from the previous optimization of ∆𝐸𝑑𝑖𝑠𝑡. 

𝑝 𝑦𝑖 is the attribute vector corresponding to the projected point. Its value can be ( ) computed by 

**==> picture [381 x 47] intentionally omitted <==**

So, Equation 7 simplifies to: 

**==> picture [364 x 100] intentionally omitted <==**

where β𝑣 𝑦𝑖 represents 𝑣[′] 𝑠 component in β 𝑦𝑖 ( ) ( ) 

**==> picture [435 x 174] intentionally omitted <==**

which can be solved in the same way as the other constraints we have seen before. 

40 

_REFERENCES_ 

## **9. Conclusion** 

In this paper, we investigated mesh simplification using edge collapse in detail by performing a deep dive into four important papers providing variations on this algorithm: [Garland and Heckbert 1997; Lindstrom and Turk 1998; Garland and Heckbert 1998; Hoppe 1996]. 

We started by discussing the basics of mesh simplification and the different categories of algorithms that are used for that purpose. We then focused on edge collapse and the half-edge data structure typically used to implement it. Next, we outlined the general algorithm used for simplification via edge collapses, including important edge cases that need to be considered. We then performed an elaborate analysis of the process of computing the error introduced by a candidate vertex placement through a variety of metrics, and discussed how the associated constraints can be assembled to form a solvable system of linear equations that yield the final, optimal vertex placement for a given edge collapse. In the process, we also dealt with other important considerations while performing mesh simplification, such as handling boundary edges and vertex/face attributes. 

We believe this work can help people interested in geometry processing and mesh simplification to understand these potent algorithms and metrics in depth, implement them for their use cases, and inspire further work in this field. 

## **References** 

- Chen, Y.-C., Kim, V., Aigerman, N., and Jacobson, A. Neural progressive meshes. In _ACM SIGGRAPH 2023 Conference Proceedings_ , pages 1–9, 2023. 4 

- Cignoni, P., Montani, C., and Scopigno, R. A comparison of mesh simplification algorithms. _Computers & Graphics_ , 22(1):37–54, 1998. 3 

- Cohen, J., Varshney, A., Manocha, D., Turk, G., Weber, H., Agarwal, P., Brooks, F., and Wright, W. Simplification envelopes. In _Proceedings of the 23rd annual conference on Computer graphics and interactive techniques_ , pages 119–128, 1996. 4 

- DeHaemer Jr, M. J. and Zyda, M. J. Simplification of objects rendered by polygonal approximations. _Computers & Graphics_ , 15(2):175–184, 1991. 3 

- Garland, M. and Heckbert, P. S. Surface simplification using quadric error metrics. In _Proceedings of the 24th Annual Conference on Computer Graphics and Interactive Techniques_ , SIGGRAPH ’97, pages 209–216, USA, 1997. ACM Press/Addison-Wesley Publishing Co. 4, 9, 41 

- Garland, M. and Heckbert, P. S. Simplifying surfaces with color and texture using quadric error metrics. In _Proceedings of the Conference on Visualization ’98_ , VIS ’98, pages 263–269, Washington, DC, USA, 1998. IEEE Computer Society Press. 11, 12, 13, 31, 41 

- He, T., Hong, L., Kaufman, A., Varshney, A., and Wang, S. Voxel based object simplification. In _Proceedings Visualization’95_ , pages 296–303. IEEE, 1995. 4 

41 

_REFERENCES_ 

_REFERENCES_ 

- He, T., Hong, L., Varshney, A., and Wang, S. W. Controlled topology simplification. _IEEE Transactions on visualization and computer graphics_ , 2(2):171–184, 1996. 4 

- Hinker, P. and Hansen, C. Geometric optimization. In _Proceedings Visualization’93_ , pages 189–195. IEEE, 1993. 3 

- Hoppe, H. Progressive meshes. In _Proceedings of the 23rd Annual Conference on Computer Graphics and Interactive Techniques_ , SIGGRAPH ’96, pages 99–108, New York, NY, USA, 1996. Association for Computing Machinery. 33, 35, 39, 41 

- Hoppe, H., DeRose, T., Duchamp, T., McDonald, J., and Stuetzle, W. Mesh optimization. In _Proceedings of the 20th annual conference on Computer graphics and interactive techniques_ , pages 19–26, 1993. 4, 7 

- Lindstrom, P. and Turk, G. Fast and memory efficient polygonal simplification. In _Proceedings Visualization ’98 (Cat. No.98CB36276)_ , pages 279–286, 1998. URL: https: //doi.org/10.1109/VISUAL.1998.745314. 4, 14, 19, 23, 41 

- Low, K.-L. and Tan, T.-S. Model simplification using vertex-clustering. In _Proceedings of the 1997 symposium on Interactive 3D graphics_ , pages 75–ff, 1997. 3 

- McGuire, M. The half-edge data structure. _Website: http://www. flipcode. com/articles/article halfedgepf. shtml_ , 2000. 5 

- Park, J. J., Florence, P., Straub, J., Newcombe, R., and Lovegrove, S. Deepsdf: Learning continuous signed distance functions for shape representation. In _Proceedings of the IEEE/CVF conference on computer vision and pattern recognition_ , pages 165–174, 2019. 4 

- Potamias, R. A., Ploumpis, S., and Zafeiriou, S. Neural mesh simplification. In _Proceedings of the IEEE/CVF conference on computer vision and pattern recognition_ , pages 18583–18592, 2022. 4 

- Rossignac, J. Edgebreaker: Connectivity compression for triangle meshes. _IEEE transactions on visualization and computer graphics_ , 5(1):47–61, 2002. 5 

- Rossignac, J. and Borrel, P. _Multi-resolution 3D approximations for rendering complex scenes_ , pages 455–465. Springer, 1993. 3 

- Schroeder, W. J., Zarge, J. A., and Lorensen, W. E. Decimation of triangle meshes. In _Proceedings of the 19th annual conference on Computer graphics and interactive techniques_ , pages 65–70, 1992. 4 

- Takikawa, T., Litalien, J., Yin, K., Kreis, K., Loop, C., Nowrouzezahrai, D., Jacobson, A., McGuire, M., and Fidler, S. Neural geometric level of detail: Real-time rendering with implicit 3d shapes. In _Proceedings of the IEEE/CVF conference on computer vision and pattern recognition_ , pages 11358–11367, 2021. 4 

- Turk, G. Re-tiling polygonal surfaces. In _Proceedings of the 19th annual conference on Computer graphics and interactive techniques_ , pages 55–64, 1992. 4 

42 

_REFERENCES_ 

_REFERENCES_ 

## **Appendix** 

**Explanation: Both planar and non-planar boundaries yield the same formula for the error in boundary preservation.** 

**Figure 21** . Boundary preservation - non-planar boundary case 

When the edge (𝑣1, 𝑣2) is replaced by vertex 𝑣 as per Figure 21, 𝐴𝑟𝑒𝑎(𝑣1, 𝑣2, 𝑣3, 𝑣5) changes but 𝐴𝑟𝑒𝑎(𝑣5, 𝑣3, 𝑣4) remains unchanged. So the change in area in this case, ℇ[′] , will be: 

**==> picture [260 x 102] intentionally omitted <==**

We know from the computation of change in area of a planar boundary that, 

**==> picture [244 x 71] intentionally omitted <==**

Thus we prove that ℇ= ℇ[′] , meaning the change in area computation is the same for planar and non-planar boundaries. 

**Explanation: In volume preservation, volume of a tetrahedron is given by** 𝑉= 𝑣𝑥 𝑣1𝑥[𝑡] 𝑣2𝑥[𝑡] 𝑣3𝑥[𝑡] 1 𝑣𝑦 𝑣1𝑦[𝑡] 𝑣2𝑦[𝑡] 𝑣3𝑦[𝑡] 6 𝑣𝑧 𝑣1𝑧[𝑡] 𝑣2𝑧[𝑡] 𝑣3𝑧[𝑡] 1 1 1 1 |||||||||||||||| |||||||||||||||| 

To simplify the quantity 𝑉, we apply the column transformations 𝐶2 = 𝐶2 −𝐶1, 𝐶3 = 𝐶3 −𝐶1 and 𝐶4 = 𝐶4 −𝐶1. This gives us: 

43 

_REFERENCES_ 

_REFERENCES_ 

**==> picture [180 x 53] intentionally omitted <==**

Evaluating this determinant along the last row reduces it to a 3 × 3 determinant: 

**==> picture [168 x 41] intentionally omitted <==**

If we ignore the constant in front, we can note that the determinant of a matrix with basis vectors 𝐶1, 𝐶2 and 𝐶3 gives the signed volume of the parallelepiped they span, as shown in Figure 22. 

**Figure 22** . Volume of a tetrahedron parallelopiped illustration 

And, we can show that the parallelepiped geometrically encompasses six tetrahedra of equal volume - see Figure 23. 

**Figure 23** . 6 equal tetrahedra from a parallelopiped 

Thus, to obtain the volume of a single tetrahedron, we divide the parallelepiped volume by 6. 

44 

_REFERENCES_ 

_REFERENCES_ 

## **Helper functions** 

**namespace** util { **template** < **typename** T> vector< **const** T*> GetUnion( **const** vector< **const** T*>& vector1, **const** vector< **const** T*>& vector2) { set< **const** T*> unique_pointers; unique_pointers.insert(vector1.begin(), vector1.end()); unique_pointers.insert(vector2.begin(), vector2.end()); **return** vector< **const** T*>(unique_pointers.begin(), unique_pointers .end()); } **bool** IsBoundaryEdge( **const** IMesh* mesh, **const** IEdge* edge) { **return** mesh->GetConnectedFaces(edge).size() < 2; } vec3 ComputeNormal( **const** IFace* face) { **auto** fv = face->GetVertices(); vec3 normal = cross( fv[1].GetPosition() - fv[0].GetPosition(), fv[2].GetPosition() - fv[0].GetPosition() ); **return** normalize(normal); } mat3 MakeSkewSymMat( **const** vec3& v) { mat3 skew_sym_mat = mat3(0); skew_sym_mat[0][1] = -v[2]; skew_sym_mat[0][2] = v[1]; skew_sym_mat[1][0] = v[2]; skew_sym_mat[1][2] = -v[0]; skew_sym_mat[2][0] = -v[1]; skew_sym_mat[2][1] = v[0]; } } 

45 

_REFERENCES_ 

_REFERENCES_ 

## **Author Contact Information** 

Purva Kulkarni Independent Researcher 1400 Main St. Canonsburg, PA 15317 purvaskulkarni14@gmail.com 

Aravind Shankara Narayanan Independent Researcher 9805 Jake Lane San Diego, CA 92126 aravind.rssn@gmail.com 

46 

