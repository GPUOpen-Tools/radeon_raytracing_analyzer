General
-------
**Check for updates** If checked, the Radeon Raytracing Analyzer will alert you
that a new version is available for download.

**Camera reset** If checked, the camera will reset to the starting position and
orientation each time the camera's control style is changed.

**TLAS/BLAS Node display** Will allow the choice between showing the nodes
in the treeviews on the viewer panes either by their GPU address, or by an offset.

**Maximum traversal count** Is used to set the maximum value of the traversal counter
slider. The value can be lowered in the case where the scene has low counter values
which will increase the resolution of the slider, or raised in the case where the
traversal count of the scene exceeds the default value.

**Maximum camera movement speed** Sets the maximum speed of the camera when navigating
around the scene. The speed is altered using a slider on the viewer panes. The default
maximum speed is optimal in most cases but for very large scenes, it may be useful
to increase this value.

**Frustum cull ratio** Objects which take small amounts of screenspace are culled for
efficiency. Lowering this ratio will allow further objects to be rendered at the cost
of rendering time. The default value is sufficient for most needs.

**Decimal precision** The number of decimal places that floating point values throughout
the app will be displayed at. Hovering the mouse over a floating point value will display
its full precision through a tooltip.

.. image:: media/settings/general_1.png

Themes and colors
-----------------
The Radeon Raytracing Analyzer makes heavy use of coloring to display its information.
This pane allows users to thoroughly customize those colors. These include being able
to specify how to color geometry based on different attributes and showing node types
using different colors.

The **Bounding volume** refers to the colors used to show the bounding volume wireframe
when the "Volume type" BVH coloring mode is selected (coloring modes are described later).

The **Wireframe coloring** colors are used when displaying the wireframe between each
triangle.

The **Selected triangles** is the color used in the BLAS view to show the currently selected
triangle nodes. Note that the selected triangle color is only available in the geometry
rendering mode.

The **Background coloring** refers to the 2 colors that are used to paint the viewer background.
By default, it consists of a checkerboard. Making the 2 colors the same will result in a single
solid color.

The **Opacity coloring** is used for the Opacity geometry coloring mode.

The **Flag indication colors** are the 2 colors used for the build flag geometry coloring modes,
indicating if the build flag is enabled or disabled.

The **Build type coloring** Combines the FastBuild and FastTrace flags. These colors are
used for the **fast build/trace** geometry coloring mode. Specifying both build flags is improper
API usage, but coloring it allows the user to spot potential errors.

The **Instance force opaque/no-opaque** Combines the ForceOpaque and ForceNoOpaque flags. These colors
are used for the **opacity** geometry coloring mode. Specifying both build flags is improper
API usage, but coloring it allows the user to spot potential errors.

.. image:: media/settings/themes_and_colors_1.png

Keyboard shortcuts
------------------

Here users will find the **Keyboard shortcuts** pane:

The **Global navigation** section refers to keystrokes that aid user
navigation, and are always detected regardless of which pane is visible.

The **Camera hotkeys** shortcuts are specific to moving and panning
operations that can be performed with the camera (see below).

The **Render hotkeys** shortcuts enable certain rendering operations to
be toggled, such as wireframe rendering.

The **Global hotkeys** section refers to any hotkeys available anywhere in
the product.

.. image:: media/settings/keyboard_shortcuts_1.png

All users are encouraged to adopt these keystrokes while using RRA.
