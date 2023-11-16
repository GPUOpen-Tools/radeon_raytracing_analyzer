The BLAS List
-------------

The BLAS List tab displays a read-only table of BLAS properties and statistics.

.. image:: media/tlas/blas_list_1.png

The following fields are displayed:

* Address – The virtual address for the BLAS structure in application memory.

* Allow update - The state of the AllowUpdate build flag.

* Allow compaction - The state of the AllowCompaction build flag.

* Low memory - The state of the LowMemory build flag.

* Build type - The state of the FastTrace/FastBuild build flags.

* Instances – The number of instances of the given BLAS in all TLASes.

* Nodes – The total number of nodes in the BLAS, including leaf nodes.

* Boxes – The total number of box nodes within the BLAS, including both Box16 and Box32.

* 32-bit boxes – The total number of box nodes with 32-bit floating point precision bounding boxes in the BLAS.

* 16-bit boxes – The total number of box nodes with 16-bit floating point precision bounding boxes in the BLAS.

* Triangle nodes – The total number of triangle nodes within the BLAS.

* Procedural nodes – The total number of AABB nodes within the BLAS.

* Memory usage - The amount of GPU memory used to store this BLAS.

* Root SAH – The computed surface area heuristic for the BLAS.

* Min SAH – The minimum surface area heuristic for the BLAS.

* Mean SAH – The average surface area heuristic for the BLAS.

* Max. depth – The maximum depth of the BLAS.

* Avg. depth – The average depth of the BLAS.

The columns can be sorted by clicking on them. The arrow in the heading shows if
sorting is in ascending or descending order.

Double-clicking an item in the table will jump to the BLAS Viewer pane and show
the selected BLAS.

