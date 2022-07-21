The Triangles Tab
-----------------

The Triangles tab displays a read-only table of properties and statistics for
triangle nodes in the selected BLAS.

.. image:: media/blas/triangles_1.png

Above the table is the base address of the BLAS used by all the triangles, and the base
address of the TLAS containing the BLAS. 

The following fields are displayed:

* Triangle address – The virtual address for the instance node within the TLAS.

* Triangle offset – The offset for the instance node within the TLAS.

* Triangle count – The number of triangles in the triangle node.

* Geometry index – The geometry index of the triangle.

* Is inactive - Is this triangle node inactive.

* Triangle surface area - The surface area of the triangle node.

* SAH - The surface area heuristic of the triangle node.

* Vertex0

* Vertex1

* Vertex2

* Vertex3 - Only used if 2 or more triangles in the node.

The columns can be sorted by clicking on the column header, apart from the vertex
columns; sorting is disabled for these columns. The arrow in the heading shows if
sorting is in ascending or decending order.
