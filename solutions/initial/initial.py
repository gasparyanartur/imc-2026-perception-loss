# Starter scaffold for "Perception-Aware Lossless Simplification of 3D Meshes".
#
# It reads the mesh, does not optimize it and then outputs it.
# Implement your simplification inside simplify()
#
# To run:
#   pypy3 initial.py < mesh.in > mesh.out

import sys

# Mesh representation.
#   V : list of (x, y, z) vertex coordinates.
#   F : list of (a, b, c) faces, 0-indexed (input is 1-indexed; load_obj
#       subtracts 1, save_obj adds it back).
V = []
F = []


# --- fast input -------------------------------------------------------------

def load_obj():
    global V, F
    tok = sys.stdin.buffer.read().split()
    nv = int(tok[0])
    nf = int(tok[1])

    # tokens are: nv nf, then "v x y z" per vertex, then "f a b c" per face.
    p = 2
    V = [None] * nv
    for i in range(nv):
        V[i] = (float(tok[p + 1]), float(tok[p + 2]), float(tok[p + 3]))
        p += 4
    F = [None] * nf
    for i in range(nf):
        F[i] = (int(tok[p + 1]) - 1, int(tok[p + 2]) - 1, int(tok[p + 3]) - 1)
        p += 4


# --- fast output ------------------------------------------------------------

# Print the mesh. Print 10 significant digits using %.10g for performance.
def save_obj():
    out = ["%d %d" % (len(V), len(F))]
    out += ["v %.10g %.10g %.10g" % v for v in V]
    out += ["f %d %d %d" % (a + 1, b + 1, c + 1) for (a, b, c) in F]
    sys.stdout.write("\n".join(out))
    sys.stdout.write("\n")


# --- your implementation ----------------------------------------------------

# Optimize the mesh: replace V and F
def simplify():
    # The initial scaffold performs no simplification: it echoes the input
    # mesh back out unchanged. Real solvers (see solutions/baseline) implement
    # the reduction logic here.
    pass


load_obj()
simplify()
save_obj()
