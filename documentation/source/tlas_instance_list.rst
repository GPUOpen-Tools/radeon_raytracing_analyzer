The TLAS Instances Tab
----------------------

The TLAS Instances tab displays a read-only table of properties and statistics for
all instances in the selected TLAS.

.. image:: media/tlas/tlas_instances_1.png

The following fields are displayed:

* Instance index - The API index for the instance.

* Instance address – The virtual GPU address for the instance node within the TLAS.

* Instance offset – The relative address for the instance node with respect to the TLAS address.

* Instance mask - The mask specified for the instance node determining which trace ray calls will interact with it.

* Cull disable - Instance flag specifying if the cull mode is disabled.

* Flip facing - Instance flag specifying whether triangles front face should be inverted.

* Force opaque - Instance flag specifying if this instance should be opaque regardless of geometry flags.

* Force no opaque - Instance flag specifying if this instance should be non-opaque regardless of geometry flags.

* Rebraid sibling count - If this instance was split into multiple instance nodes by the driver, this is how many sibling instance nodes this instance has.

* X Position – The X-position of the instance in the scene.

* Y Position – The Y-position of the instance in the scene.

* Z Position – The Z-position of the instance in the scene.

* Transform[x][y] - The instance transform, comprising of the rotation and scaling components.

The columns can be sorted by clicking on them. The arrow in the heading shows if
sorting is in ascending or descending order.

Typically, instances are created with their own local co-ordinate system. When
placed in the scene, each instance requires a transformation from its local
co-ordinate system to the world co-ordinate system. This is shown by the
position and transform matrix in the table.
