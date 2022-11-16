The Triangles Tab
-----------------

The Triangles tab displays a read-only table of properties and statistics for
triangle nodes in the selected BLAS.

.. image:: media/blas/triangles_1.png

Above the table is the base address of the BLAS used by all the triangles, and the base
address of the TLAS containing the BLAS. 

The following fields are displayed:

* Geometry index - The index of the geometry that the triangle belongs to.

* Opaque - Presence of the opaque geometry flag.

* No duplicate any hit invocation - Presence of the no duplicate any hit invocation geometry flag.

* Primitive index - The API index of the triangle accessible in shaders.

* Node address - The virtual address of this node in GPU memory.

* Node offset - The relative address of this node relative to the BLAS address.

* Active - Whether or not this triangle is active according to the API specification definition.

* Triangle surface area - The surface area of the triangle node.

* SAH - The surface area heuristic of the triangle node.

* Vertex0 - The vertex position of the first triangle vertex.

* Vertex1 - The vertex position of the second triangle vertex.

* Vertex2 - The vertex position of the third triangle vertex.

The columns can be sorted by clicking on the column header, apart from the vertex
columns; sorting is disabled for these columns. The arrow in the heading shows if
sorting is in ascending or decending order.
