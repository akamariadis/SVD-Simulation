# SVD-Simulation

## Overview

Singular Value Decomposition tells us that every matrix **A** can be decomposed into a product of three matrices.


A matrix can be viewed as a mathematical transformation that, when applied to a unit sphere (or a unit circle in 2D), deforms it into another geometric shape.

The SVD decomposition allows us to break down this transformation into **three distinct stages**, each represented by one of the three matrices:

1. **Rotation – V^T**

   The first transformation rotates the unit sphere into a new orientation.

2. **Scaling – Σ**

   The second transformation deforms the sphere according to the singular values of the matrix, stretching or compressing it along its principal axes.

3. **Rotation – U**

   The final transformation rotates the resulting shape into its final orientation.

By visualizing these three consecutive transformations, the project aims to make the mathematical intuition behind SVD easier to understand. Through an intuitive visualization of the Singular Value Decomposition, a seemingly complex marix transformation can be understood as a sequence of rotations and scaling operations!
