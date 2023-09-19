The Inspector Tab
-----------------

.. image:: media/ray/ray_inspector_1.png

The screen layout is similar to the TLAS and BLAS viewer panes, and is split into 3 areas.

The left section shows a summary of the currently selected dispatch coordinate from the Dispatches tab:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. image:: media/ray/ray_inspector_2.png

In the top-left is the **Ray table**. The ray index provides the order in which the rays were cast in the
application's shaders; index 0 will be the first ray generated.

Rays can be chosen by clicking on an entry in the table. The selected ray will be outlined in yellow by default. The ray
colors can be changed in the Themes and colors settings pane.

Underneath this is the Selected ray section showing the values passed to the TraceRay() call in the shader. Clicking on
the box icon to the right of the Selected ray header will focus ray in the 3D view.

The Ray result section shows the distance of the accept hit as well as the instance index, geometry index, and primitive
index of the hit triangle. These fields will be blank if the ray misses.

The center section shows a rendering of the scene:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. image:: media/ray/ray_inspector_3.png

Super-imposed on the scene are the rays corresponding to the pixel selected from the Ray dispatches pane. The scene
coloring defaults to a new grayscale heatmap coloring mode so that the rays stand out from the background.

Clicking on a ray in the 3D view will select it, as indicated by a yellow (by default) outline around it.
The bottom of the screen shows a legend for the color of the rays, which can be modified in the Themes and colors
settings pane. The TLAS geometry shown cannot be selected in the ray inspector, it's just shown as a visual aid
providing context to the rays.

The instance mask in the top left shows the instance mask of the currently selected ray, and will update the 3D
view to only show instances which are included by the mask. Unlike in the TLAS tab, the instance mask filter is
read-only in the Ray tab. The rays with an instance filter mask of 0 will be colored red (by default) to indicate
that the ray will have no chance of invoking a hit shader.

The right section allows control over the rendering and camera:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. image:: media/ray/ray_inspector_4.png

These controls function almost identically to those in the TLAS and BLAS viewer panes.

A notable difference is that this pane contains a lock button to the right of the Camera position label. When locked,
the camera will preserve its position when changing the selected dispatch coordinate in the Dispatches tab instead of
focusing on the first ray in that coordinate's ray list.
