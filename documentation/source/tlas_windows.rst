The TLAS windows
================

These panes show information about a top-level acceleration structure
and its associated bottom-level acceleration structures.

The TLAS Viewer
---------------

The top-level acceleration structure consists of a bounding volume hierarchy. Each
bounding volume will consist of child bounding volumes or instances. An instance is
simply a set of geometry inside a bounding volume. There can be multiple instances
in a scene. Each instance corresponds to a BLAS structure, described below. The TLAS
pane will therefore show a rendering of the complete scene inside the very top
bounding volume. There can be multiple TLAS’s in a scene.

Each viewer pane in RRA has a UI reset button, in the top-right corner of the UI:

.. image:: media/reset.png

This allows the rendering and camera controls, and coloring modes to be reset to their
factory defaults. Certain UI controls are shared between viewers, so resetting these on
one viewer pane will be reflected on the other viewer panes. Examples of these would be
the up axis and inverting the horizontal or vertical axes, since these parameters are
considered global to the scene. The movement speed slider isn't currently reset since
its initial value is calculated depending on the scene size, so smaller scenes will
have a smaller movement speed.

An example TLAS viewer pane is shown below:

.. image:: media/tlas/tlas_viewer_1.png

The view is split into 3 sections:

The left section shows a summary of the currently selected TLAS:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. The **TLAS** dropdown allows selection of which TLAS to view.

#. The **Expand tree** text is a toggle button and clicking it will expand the
   treeview so all items in the tree are visible. Pressing it again will collapse the
   treeview.

#. The treeview will show the structure of the TLAS.
   In a TLAS, there are box nodes (either Box16 or Box32 for 16-bit or 32-bit
   floating point values, respectively). Each box node can contain up to 4 child nodes,
   which can be box nodes or instance nodes. Instance nodes are leaf nodes. An instance
   node contains a reference to a bottom-level acceleration structure which contains
   the geometry to render.

   a.  Selecting a node will highlight the node and all its child nodes in the treeview.
       Any instance nodes selected will be shown selected in the scene as having a
       highlighted wireframe around the triangles. This wireframe coloring can be customized
       in the Settings Themes and colors pane.

   b.  If a node has child nodes, it will have a '>' to the left of it. Clicking this will
       open the node and show the child nodes. Typically all box nodes will have child nodes.

   c. Nodes can be shown/hidden using the checkbox for that node, located to the left of the
      node name. Any hidden nodes will be grayed out in the treeview and will not be rendered
      in the scene. This allows the user to just see the geometry of interest.

   d.  Double-clicking on an instance node will open the BLAS it references in the BLAS pane.

#. The section below the treeview gives details about the currently selected node, including
   the parent node and extents. Clicking the parent address will select the parent node.

#. If the selected node is an instance then instance specific information passed to the API
   will be displayed, including the instance transform, instance mask, flags, and the BLAS
   address. Clicking the BLAS address will navigate to that BLAS in the BLAS tab. The position
   of the instance can be found in the last column of the instance transform, separated by a
   vertical bar.

The left-pane can be resized by moving the mouse over the area of the screen where the
left pane and view pane meet. The cursor will change, showing the pane can be resized
by clicking and dragging the mouse.

The center section shows a rendering of the scene:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the center is a rendering of the scene. The scene will consist of a series of
bounding volumes with geometry contained within that volume. Various display modes
can be altered by the user to change how the scene is rendered.

The viewport camera can be manipulated using a mouse and keyboard
using a preset control style (described later).

Pressing the "R" key in all camera modes will reset the camera to its default position.
This is useful to recover from disorientation such as losing sight of the scene in the 3D view.

It is possible to select an instance within the scene by clicking on any mesh within
the viewport. Multiple instances can be selected by holding down the Ctrl key
while clicking. The TLAS hierarchy tree view will expand as necessary to focus on the
selected instance. Pressing the "F" key will focus the camera on the selected object
in the scene. If multiple instances are selected, the view will be adjusted so all
selected instances are visible.

An orientation gizmo is overlaid on the scene, in the top-right corner. This shows how
the coordinate axes are aligned with the scene.

A context menu can be displayed by right-clicking on the scene.

.. image:: media/tlas/context_menu_1.png

The list of options are:

#. **Add instance under mouse to selection** - Select the instance under the mouse. It
   is possible to have more than one instance selected.
   One possible use-case is to select the instances that are of interest and hide everything
   else in the scene.

#. **Deselect all** - Deselect any instances that are selected.

#. **Focus on selection** - Focus the camera to look at the selected instances. If more than
   one instance is selected, the camera will pan back to show all selected instances. Pressing
   the "F" key will accomplish the same thing.

#. **Hide selected** - Hide any selected instances in the scene. Pressing the "H" key will
   accomplish the same thing. By using the mouse to select instances and the "H" key to
   hide them, the scene can be quickly cleared of instances that are not relevant or obscuring
   areas of interest.

#. **Select all visible** - Select all visible instances in the scene.

#. **Show everything** - Make everything in the scene visible. Pressing the "J" key will
   accomplish the same thing.

#. **Show only selection** - Show only instances that are currently selected; all other instances
   will be hidden.

Below the scene is a segmented bar:

.. image:: media/tlas/depth_slider.png

This tree depth slider is used to select which tree depths to show bounding volumes for.
The default has the first segment selected, indicating that only the highest level bounding
volume is to be shown.

To use the slider, click on a segment. Holding the mouse button and dragging will allow
selection of the depth range.

The minimum and maximum tree depths selected are also shown at either side of the slider. In
this case here, just tree level 0 (the root) is shown, so the range is from 0 to 0.

The right section allows control over the rendering and camera:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On the right is a 'Show/hide controls' button. It can be clicked on and will open a side
panel to the right of the rendering. This opens the view controls pane:

.. image:: media/tlas/popout_view_1.png

Rendering controls
~~~~~~~~~~~~~~~~~~

The **Rendering controls** section includes checkboxes to control how the scene is rendered.

The rendering mode radio buttons allow selection of the rendering mode. The default is the
**Geometry rendering mode**, showing the triangles which make up the scene. An example of this
rendering mode can be seen at the beginning of this section.

The **Traversal counter mode** is a display mode that counts ray intersections with elements from within
the acceleration structure. Examples would include triangle/box hit and test counts.
See the section below for more information on the traversal counters.
  
In geometry rendering mode, there are 4 checkboxes that control what is visible in the scene:

* **Show geometry** will only draw the scene if enabled. Switching it off will allow the bounding
  volumes or wireframes to be seen more easily.

* **Show axis aligned BVH** will display the bounding volumes overlaid as wireframes if enabled.
  This bounding volume will be axis aligned in the TLAS.

* **Show instance transform** will display the instance bounding volume overlaid as a dashed wireframe.
  This bounding volume has the instance transform applied, so is effectively in BLAS-space.

* **Show wireframe** will show a wireframe overlay over the geometry, which will allow the individual
  triangles to be seen. 

Finally, a **Culling mode** combo box is available. In geometry rendering mode, this is the standard frontface/backface/none culling
mode which only affects the viewport and does not reflect the state of your application when it was captured.

In traversal counter rendering mode, the controls are slightly different, as seen below:

.. image:: media/tlas/popout_view_3.png

* The **Accept first hit** checkbox simulates the accept first hit ray flag that can be passed to the
  trace ray invocation in the shader. The traversal algorithm visualized by the heatmap will be altered
  based on this flag.

* The **Box sort heuristic** describes how box nodes are sorted during traversal. This determines the
  order in which box nodes are checked for intersections. It's dependent on ray flags and system configuration.

  * The **Closest** sort heuristic checks the closest box nodes first for intersections.

  * The **Middle point** sort heuristic checks the box nodes with the closest centers first for intersections.

  * The **Largest** sort heuristic checks the largest box nodes first for intersections.

* The **Counter range** slider allows the user to set a minimum and maximum traversal count limit to display on
  in the heatmap. The results of changing the slider values can be seen instantly in the viewport.
  
  * The **Counter range** slider has a range between 0 and 1000 but the limit can be changed in the
    **General** section of the settings under **Maximum traversal count**.
  
  * The values under the slider are the current minimum and maximum values of the 2 slider handles, as well as the average of
    the traversal counts of each pixel visible in the viewport.

* The **Counter histogram** above the counter slider shows the distribution of traversal counts in the current viewport. The
  leftmost end of the histogram is 0 and the rightmost end is the maximum slider traversal count set in the settings. The
  histogram lines up with the slider below it, where each vertical slice represents the same traversal count for the slider
  and the histogram.

* Clicking on the **Wand icon** will automatically adjust the slider values to the minimum and maximum
  pixel values visible in the viewport.
  
* The **Continuous update** checkbox, when enabled, will automatically adjust the counter range slider
  to the minimum and maximum pixel values visible as the viewport camera is moved around. It saves the
  work of clicking on the wand icon to update the color range of the scene. NOTE: When **Continuous update**
  is enabled, the wand icon is disabled.

The **Show axis aligned BVH**, **Show instance transform**, and **Show wireframe** checkboxes are also
present, along with the culling mode combo box.

In traversal counter rendering mode, the selected culling mode plays the part of the frontface/backface triangle
culling flags passed to the trace ray call in the shader. This means that the culling behavior can be overridden
or modified for each instance via instance flags.

In other words, the instance flags set up for the instance in the application will override the UI settings, and
not the other way round.

For example, the culling mode ray flags set up in RRA will only have an effect if 
D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE or VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR
are not set.

Camera controls
~~~~~~~~~~~~~~~

The **Camera controls** section allows selection of the camera controls.

* A combo box allows selection of the camera control style. This can be either **CAD control style**,
  **FPS control style** or **Axis-free control style** and is chosen depending on the control style the
  user is most familiar with, whether it be a modeling (CAD) package or a gaming application (FPS).
  The **Axis-free** camera does not constrain the camera by orienting itself with a global up-axis. This
  is useful for applications that do not have a natural up-axis, like space exploration games.
  The camera setting is global, so changing the camera style on the TLAS viewer pane selects the same
  camera style on the BLAS viewer pane, and vice versa. Switching from CAD control style to FPS control
  style will not retain the CAD focal point, so upon switching back to CAD you will need to focus on
  an instance again to revolve the camera around it.

.. image:: media/tlas/popout_view_2.png

* The **Mouse and keyboard** icon will display a list of all the valid hotkeys for the currently
  selected control style and are primarily used to drive the camera. 
  Common keyboard shortcuts are also described in the keyboard shortcuts section in the settings menu. 

* The **Projection** combo box allows selection of the projection mode, switching between
  perspective and orthographic viewing modes. The default is perspective.

* The **Up axis** radio buttons allow the orientation of the scene to be changed according to
  the specified **up** axis. This will be dependent on the coordinate system of the application
  from where the scene originated. Alternatively, the scene can be oriented in the 3D view
  so it looks correct, then the "U" key can be pressed. RRA will then set the up axis automatically.

* The **Coordinate system** checkboxes allow the inversion of the horizontal and vertical axes.

* The **Camera position** edit-boxes show the current camera position. These values can be
  edited manually if needed. The reset icon can be clicked to move the camera to the origin.

* The **Field of view** slider changes the camera's field of view.

* The **Movement speed** slider changes the speed of the camera. The maximum speed can be set in the
  **General** section of the settings under **Maximum camera movement speed**.

Traversal counter visualization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Given the complexity of acceleration structures and the specifics of the ray traversal algorithms that
operates on these structures, it can be very difficult to evaluate the performance cost of a given scene.

The traversal counter visualization will help simplify this complexity and help reduce traversal count
signatures by editing BLASes and repositioning of instances in the TLAS.

  * The counters are calculated on-the-fly and are not the same as those provided by the Radeon GPU Profiler.

  * RRA counters terminate on closest hit and ignore any subsequent rays that are launched.

  * RRA also counts custom intersection volumes as a single unit.

An example of a typical scene using the traversal counters is shown below.

    .. image:: media/tlas/loop_count.png

The visualization depends on a counter range provided by the user via the **Counter range** slider. 
The range is determined by the scene layout and the counter type selected. Adjusting the slider will
alter the coloration of the scene. The colors are displayed as a heatmap, so blue represents a low
counter value and red represents a higher counter value by default. Generally, the lower the counter value, the
more optimal the scene will be. This visualization shows how costly ray traversals are, but does not account
for TLAS and BLAS build times which also affect overall performance.

There are several different counter types to choose from:

  * The **Loop count** is the number of iterations the ray performs on the acceleration structure. It
    allows the user to identify parts of the acceleration structure that are the most taxing for the rays.
    The loop count will have the largest counter range of all the counters.

  * The **Instance hit** is the number of instances that the ray has hit. This is useful in terms
    of how instance positioning affects traversal time. As a ray traverses into the scene, it is
    optimized to discard bounding volumes as needed. A ray can discard a volume if a triangle
    closest hit candidate has been found and the volume is behind the closest hit candidate.
    
    When a ray hits an instance node, it has to context switch into the BLAS and traverse the
    BLAS to get a closest-hit triangle and compare this to the current closest-hit triangle, which
    may be from a different TLAS node. In addition, if instance nodes overlap, the ray must wait
    until each instance is fully checked.

    It is therefore essential to arrange instances so that context switching into BLAS nodes is
    minimized.

  * The **Box volume hit**, **Box volume miss**, and **Box volume test** count how many box nodes were hit, missed, and tested,
    respectively. The number of tests is equal to the sum of the number of hits and misses. Some parts of the scene may be
    denser depending on the perspective. The dense parts may overlap so the ray may not be able to discard volumes.

  * The **Triangle hit** counter is the number of triangles that have been used as the closest hit
    candidate. As the ray traverses an acceleration structure, it may encounter triangles in an
    unspecified order. If the ray hits a triangle, it will compare this triangle with the current
    closest hit triangle. If there isn't a closest hit triangle, this triangle will be assigned as
    the closest hit. The **Triangle miss** is the number of triangles that have been tested but
    were not a closest hit. The **Triangle test** is the sum of hits and misses.

Coloring modes
~~~~~~~~~~~~~~

The coloring modes are available in a row above the scene rendering.

#. **BVH Coloring** allows the bounding volume wireframes to be painted depending on a
   number of different parameters. The following BVH coloring modes are currently supported
   within the TLAS viewer:

   * Volume type
      The bounding volume coloring is based on the node types, allowing box, triangle,
      procedural geometry and instance nodes to be distinguished from one another.
      The selected BVH is also colored differently. These colors can be configured from
      the **Themes and colors** settings pane.

   * Tree depth
      Each bounding volume is assigned a color based on how deep in the hierarchy it is.

#. **Geometry Coloring** is only available for the Geometry rendering mode and allows the scene to
   be painted depending on a number of different parameters, for example, each BLAS can be colored
   differently enabling the user to see if their grouping of objects in the scene is optimal.

   Some of these coloring modes use a heatmap coloring scheme, some use fixed colors and some have
   colors that are selectable from the **Themes and colors** pane. The type of heatmap can be selected
   from the **Heatmap** combo box to the right of the **Geometry coloring** combo box. This is
   described in a bit more detail later on.

   Several coloring modes mention the surface area heuristic (SAH) of triangles. This is a value between
   0 and 1 which is proportional to the probability a ray will intersect with a triangle given that it
   intersects with its bounding box, where 0 (bad) means low probability and 1 (good) means high probability.
   Triangles with low SAH often are long, skinny, and not axis-aligned in BLAS space.

   The following geometry coloring modes are supported within the TLAS viewer, and its coloring scheme:

   * Average SAH (BLAS)
      A heatmap showing the average surface area heuristic of all triangles in a BLAS.

   * SAH (Triangle)
      A heatmap showing the surface area heuristic of each individual triangle.

   * Minimum SAH (BLAS)
      A heatmap showing the minimum surface area heuristic of all triangles in a BLAS.

   * Mask (Instance)
      A unique color for each combination of instance mask flags.

   * Opacity (Geometry)
      A color showing the final opacity of each geometry as a function of instance and geometry flags. These colors can be configured in the Themes and colors section of the Settings under 'Opacity coloring'.

   * Opacity (Geometry)
      A color showing the presence of the opacity flag. These colors can be configured in the Themes and colors section of the Settings under 'Opacity coloring'.

   * Force opaque / no opaque flag (Instance)
      Combines the ForceOpaque and ForceNoOpaque instance flags, giving 4 possible color combinations. These colors can be configured in the Themes and colors section of the Settings under 'Instance force opaque/no-opaque'.

   * Geometry index (Geometry)
      A unique color for each geometry index within a BLAS.

   * Fast build/trace flag (BLAS)
      Combines the FastBuild and FastTrace build flags, giving 4 possible color combinations. These colors can be configured in the Themes and colors section of the Settings under 'Build type coloring'.

   * Allow update flag (BLAS)
      Shows whether the 'AllowUpdate' build flag is enabled. These colors can be configured in the Themes and colors section of the Settings under 'Build type coloring'.

   * Allow compaction flag (BLAS)
      Shows whether the 'AllowCompaction' build flag is enabled. These colors can be configured in the Themes and colors section of the Settings under 'Flag indication colors'.

   * Low memory flag (BLAS)
      Shows whether the 'LowMemory' build flag is enabled. These colors can be configured in the Themes and colors section of the Settings under 'Flag indication colors'.

   * Facing cull disable flag (Instance)
      Shows whether the 'FacingCullDisable' instance flag is enabled. These colors can be configured in the Themes and colors section of the Settings under 'Flag indication colors'.

   * Flip facing flag (Instance)
      Shows whether the 'FlipFacing' instance flag is enabled. These colors can be configured in the Themes and colors section of the Settings under 'Flag indication colors'.

   * Tree level (Triangle)
      A heatmap showing the triangle's depth within the BVH.

   * Max tree depth (BLAS)
      A heatmap showing the maximum tree depth of each BLAS.

   * Average tree depth (BLAS)
      A heatmap showing the average tree depth of each BLAS.

   * Unique color (BLAS)
      A unique color for each BLAS.

   * Unique color (Instance)
      A unique color for each instance.

   * Instance count (BLAS)
      A heatmap showing how many instances each BLAS has.

   * Triangle count (BLAS)
      A heatmap showing the triangle count of each BLAS.

   * Rebraiding (Instance)
      Shows which instances have been rebraided by the driver. See the section on `Rebraiding`_ below for more information.

   * Triangle splitting (Triangle)
      Shows which triangles have been split by the driver. See the section on :ref:`Triangle splitting <triangle-splitting-label>` in the BLAS viewer section for more information.

   * Lighting
      Directionally lit shading.

   * Technical drawing
      Directionally lit Gooch shading.

#. **Traversal counters** is only available when the traversal rendering mode is
   enabled, and allows for different hit and test counters to be used when colorizing
   the scene. Each pixel shows how many bounding volume tests or hits were performed.
   There are a number of counters available and details of each can be obtained by
   opening up the combo box and mousing over each option which will display a tooltip.
   All of the traversal counter coloring modes use the heatmap coloring scheme.

   The following counters are supported:

   * Loop count
      The number of iterations the ray performs on the acceleration structure.

   * Instance hit
      The number of instances that are hit before the closest hit is found.

   * Box volume hit
      The number of volumes the ray intersects with.

   * Box volume miss
      The number of volumes the ray has been tested with but doesn't intersect with.

   * Box volume test
      The number of volumes the ray is tested with. This is the sum of box hits and misses.

   * Triangle hit
      The number of triangles which have been considered the closest hit candidate.

   * Triangle miss
      The number of triangles which have been tested but not considered the closest hit candidate.

   * Triangle test
      The number of triangles which the ray has been tested against. This is the sum of triangle hits and misses.

#. **Heatmap selection** allows which heatmap to use. The default heatmap uses a **Temperature** scheme
   where the colors vary from red to green to blue. The **Spectrum** scheme uses more of the visible
   color spectrum, giving a wider range of colors. The **Viridis** and **Plasma** color schemes are
   perceptually uniform heatmaps. Each heatmap will show the scene slightly differently with some heatmaps
   showing certain areas of the scene better than others.

UI Persistent state
~~~~~~~~~~~~~~~~~~~
The state of some of the lesser-used user interface controls is remembered between RRA sessions.
This is to try to avoid having to configure the UI each time RRA is loaded, particularly in the
case where the same application is being analyzed between sessions. The persistent state can
be toggled from the settings, described in the **Settings** section.

Above the 'Show/hide controls' text button in the top-right of the view is an icon that allows
the UI to be reset to its default settings. There is a similar button on the BLAS Viewer pane.
Hoving over the button will display a help tooltip. Clicking on the button will reset the
persistent UI controls back to their defaults. Since some elements are shared between the BLAS
and TLAS viewers, both the TLAS and BLAS controls will be reset, regardless of which reset
button was pressed.

The following UI elements are shared between all viewers:

-  Culling mode

-  Up axis

-  Invert vertical

-  Invert horizontal

-  Continuous update

-  Projection mode

The following UI elements are saved per-view:

-  Control style

-  BVH coloring mode

-  Geometry coloring mode

-  Heatmap coloring mode

-  Traversal counter coloring mode

-  Rendering mode

-  Show geometry checkbox

-  Show axis-aligned BVH checkbox

-  Show instance transform checkbox (TLAS viewer only)

-  Show wireframe checkbox

-  Accept first hit checkbox

-  Cull back-facing triangles

-  Cull front-facing triangles

The movement speed and field of view states currently do not persist between RRA sessions.

Instance mask filter
~~~~~~~~~~~~~~~~~~~~
The instance mask filter allows setting an 8-bit instance mask to simulate the effect of the InstanceInclusionMask
argument of TraceRay() in HLSL or the cullMask argument of traceRayEXT() in GLSL. In both geometry rendering mode and
traversal rendering mode, the instances not included in the mask will be culled.

.. image:: media/tlas/instance_mask_filter.png

The mask can be set either by clicking the individual bits to toggle them on the left side, or by typing or pasting in a hex value on the right side.

.. _Rebraiding:

Rebraiding
~~~~~~~~~~
Rebraiding is a TLAS build strategy used by the driver. When the build algorithm determines that the
combination of an instance transform with a particular BLAS will yield a small SAH number it may decide to
rebraid that instance to reduce the amount of empty bounding box volume. A rebraided instance will be split
into the number of immediate child nodes of the BLAS root node. The instances that are split will retain
all the data used by the original instance, however, the extents of each instance will now use the extent
of the BLAS node it corresponds to. Rebraiding is an optimization done automatically by the driver, so the
application developer has no direct control over it.

Rebraided instances will be clearly marked on the left-side pane when an instance is selected, as seen below.
If the instance has been rebraided, the sibling nodes will be listed, allowing for easy selection.

.. image:: media/tlas/rebraiding_stats_1.png

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

The TLAS Properties Tab
-----------------------

The Properties tab displays a read-only table of properties and statistics for
the selected TLAS.

.. image:: media/tlas/tlas_properties_1.png
