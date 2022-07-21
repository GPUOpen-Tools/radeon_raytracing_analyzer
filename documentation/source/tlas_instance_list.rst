The TLAS Instances Tab
----------------------

The TLAS Instances tab displays a read-only table of properties and statistics for
all instances in the selected TLAS.

.. image:: media/tlas/tlas_instances_1.png

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
position and transform matrix in the table.

In some cases, the positions can all be 0. This just means that the instance
has been created in world-space co-ordinates and does not need to be translated.
This is typically done when only one instance of a BLAS is used in a scene.
