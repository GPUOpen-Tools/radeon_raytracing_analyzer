The Dispatches Tab
---------------

The main ray dispatches pane allows you to see more information about the selected dispatch,
and select individual dispatch coordinates for further inspection.

The display consists of 2 sections; an information section on the left, and a heatmap image of
the dispatch output on the right.

.. image:: media/ray/ray_history_1.png

The Information section
~~~~~~~~~~~~~~~~~~~~~~~
As well as selecting dispatches from the **Overview pane**, they can also be selected from a dropdown
in the top-left. The UI will be updated according to the dispatch selected.

Below the dispatch selection dropdown is the **Dispatch counters** table which shows some high-level
statistics for the selected regions in the heatmap (see below), such as the ray count and number of instance
intersections per ray.

Below this is the **Dispatch coordinates** table. This shows the information for each dispatch
coordinate, including the ray count, the traversal loop count and the number of instance intersections.
The table columns can be sorted to help highlight problematic areas, for example, sorting by Ray count 
to find coordinates that cast too many rays. Note that only the coordinates with at least 1 ray cast
will be displayed.

The Heatmap section
~~~~~~~~~~~~~~~~~~~
On the right hand side is a heatmap visualizing traversal statistics of the dispatch. A typical heatmap
image will be a 2D image that correlates closely to the final 2D rendered image in the capture application.
However, applications are free to use 1D, 2D, or 3D dispatches in an arbitrary way that may or may not be
easily mappable to the final rendered image. For 3D dispatches, they can be displayed as a series of 2D
'slices' by changing the **XY image with Z index at** dropdown to change the slice plane displayed, and
index into the slices by editing the number to the right of the dropdown.

The image can visualize each column of the dispatch coordinates table using the **Color heatmap by ..**
dropdown, and the color palette of the heatmap can be changed using the **Heatmap as ..** dropdown.

A slider bar below the image can be used to adjust the heatmap range, which enables potential problems to be
seen easier.

The rendered image can be manipulated with the mouse. An area of interest can be selected by holding down
the left mouse button and dragging the mouse. Repeating this process will select a new rectangle. When
a rectangle is selected, the unselected area will be grayed out, and the Ray list will update to filter out
the unselected dispatch coordinates. The right mouse button can be used to cancel or deselect the
selected area. Since some dispatches can be large, the mouse wheel can be used to zoom in or out while the
middle mouse button will pan when the mouse is dragged. The zoom will be focused on the mouse cursor position.

**Zoom controls**

Zoom control buttons are provided to view sections of interest in the heatmap image and are displayed in the top
right, immediately above the heatmap image. These are:

.. |ZoomSelectionRef| image:: media/zoom_to_selection.png
.. |ZoomResetRef| image:: media/zoom_reset.png
.. |ZoomInRef| image:: media/zoom_in.png
.. |ZoomOutRef| image:: media/zoom_out.png

|ZoomSelectionRef| **Zoom to selection**

When **Zoom to selection** is clicked, the zoom level is increased to show the currently selected region.
If there is no selected region, the button will be grayed out and non-functional.

|ZoomResetRef| **Zoom reset**

When **Zoom reset** is clicked, the zoom level is returned to the original default zoom level.

|ZoomInRef| **Zoom in**

Increases the zoom level incrementally to show a smaller area of the image, and duplicates the functionality
accomplished with the mouse wheel. The button will be grayed out and non-functional if at the maximum zoom
level.

|ZoomOutRef| **Zoom out**

Decreases the zoom level incrementally to show a larger area of the image, and duplicates the functionality
accomplished with the mouse wheel. The button will be grayed out and non-functional if at the minimum zoom
level.

Clicking a pixel without dragging will select just that pixel, displaying a cursor icon directly below it.
This dispatch coordinate will be selected in the dispatch coordinates table as well.

An area of interest is shown in the image below:

 .. image:: media/ray/ray_history_2.png

After selecting a region, the dispatch table will be updated to only contain the rays within the dispatch
coordinates of the selected region and the dispatch counters statistics will be updated as well. Double-clicking
on an entry in the table or a pixel in the heatmap image will navigate to the **Ray Inspector** pane.
