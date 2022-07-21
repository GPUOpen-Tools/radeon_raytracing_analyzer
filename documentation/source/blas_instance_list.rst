The BLAS Instances Tab
----------------------

The Instances tab displays a read-only table of properties and statistics for
instances of the selected BLAS.

.. image:: media/blas/blas_instances_1.png

Above the table is the base address of the BLAS used by all the instances.

The following fields are displayed:

* Instance address – The virtual address for the instance node within the TLAS.

* Instance offset – The offset for the instance node within the TLAS.

* Instance mask - The mask specified for the instance node.

* X Position – The X-position of the instance in the scene.

* Y Position – The Y-position of the instance in the scene.

* Z Position – The Z-position of the instance in the scene.

* Transform[x][y] - The instance transform, comprising of the rotation and scaling components.

The columns can be sorted by clicking on them. The arrow in the heading shows if
sorting is in ascending or decending order.

Typically, instances are created with their own local co-ordinate system. When
placed in the scene, each instance requires a transformation from its local
co-ordinate system to the world co-ordinate system. This is shown by the
positional information in the table; if an instance is used more than once,
each instance will have a unique position. If there are duplicate positions,
then there are multiple instances at the same world position, which is
redundant; the duplicates can be removed.

In some cases, the positions can all be 0. This just means that the instance
has been created in world-space co-ordinates and does not need to be translated.
This is typically done when only one instance of a BLAS is used in a scene.
