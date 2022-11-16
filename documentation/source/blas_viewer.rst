The BLAS Viewer
---------------

This pane is very similar to the TLAS viewer, except the leaf nodes are triangles
instead of instances.

.. image:: media/blas/blas_viewer_1.png

The most commonly used widgets are available in a row below the pane tabs.

#. The **BLAS** dropdown allows selection of which BLAS to view.

#. **BVH Coloring** allows the bounding volume wireframes to be painted depending on a
   number of different parameters. See the TLAS documentation for more details on these
   coloring modes.

#. **Geometry coloring** allows the scene to be painted depending on a number of different
   parameters. The list of coloring modes is a subset used by the TLAS geometry modes
   combo box. See the TLAS documentation for more details on these coloring modes.

#. **Heatmap selection** allows changing the color scheme of the heatmap. See the
   TLAS documentation for more details on these coloring modes.

On the left will be information for the bottom level acceleration structure:

#. The treeview under the dropdown will show the structure of the BLAS. As seen here,
   the topmost level contains a Box32 structure. This Box32 contains 4 more Box32
   structures. The currently opened box shows a list of triangles inside this box.

#. The section below the treeview gives details about the currently selected node,
   including the surface area heuristic, and extents.
   
#. If the selected node is a triangle node then the triangle vertex positions will be
   displayed, as well as geometry flags of the geometry that the triangle is a part of.

In the center is a rendering of the bounding volume and geometry contained within that
volume. Please see the TLAS help section for more information since the control modes
and functionality is similar to the TLAS scene display.

It is possible to select individual triangles within the scene by clicking on a mesh within
the viewport. The BLAS hierarchy tree view will expand as necessary to focus on the
selected triangle node.

On the right are the same rendering and camera controls as seen in the TLAS pane; the functionality of
these are nearly identical. The only difference is that there is no option to show instance transforms
in the BLAS pane.

.. _triangle-splitting-label:

Triangle splitting
~~~~~~~~~~~~~~~~~~
Triangle splitting is another driver optimization that can be used if a triangle isn't axis aligned. This
can result in a lot of empty space in the bounding volume. In this case, the triangle is split into
smaller triangles, each with a smaller bounding volume. This reduces the triangle-area to bounding volume
ratio.

It should be noted that all split triangles will share the same 3 triangle vertices, and parts of the
triangle will lie outside of the split triangle bounding volume. This can be seen with geometry rendering
(below).

.. image:: media/blas/split_triangles_1.png

Image (a) Shows the bounding volume around the entire triangle geometry. Notice that this triangle is about
45 degrees to one of the axes; the unused space in the bounding volume can be clearly seen. Images (b) and (c)
show the individual split triangles making up the triangle geometry in (a).

Since ray tracing deals with bounding volumes, only the part of the triangle inside the bounding volume
will be considered. This can be seen with the traversal rendering, where only the area of the triangle inside
the bounding volume is visible (below).

.. image:: media/blas/split_triangles_2.png

These 2 images are from the same scene, but seen from a slightly different angle.

When a split triangle is selected, the left-side pane will indicate that the triangle is split, as seen below.
A list of the split triangle siblings is also shown, allowing easy selection.

.. image:: media/blas/split_triangles_stats_1.png
