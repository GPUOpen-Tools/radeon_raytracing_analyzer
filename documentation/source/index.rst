The Radeon™ Raytracing Analyzer (RRA)
=====================================

The Radeon Raytracing Analyzer is a tool designed to help developers improve the raytracing performance on AMD Radeon 6000 and 7000 series GPU's.
The tool focuses on the visualization of Acceleration Structures, which in our case consist of Bounding Volume Hierarchies. RRA allows the developer
to visualize the bounding box hierarchies, and related scene geometries, to quickly identify issues with the bounding volume hierarchies, such as
overlapping bounding volumes and sparse geometry layout within bounding volumes. Once identified, the developer can revisit their BVH generation
strategy.

This document describes how the Radeon Raytracing Analyzer can be used to examine a bounding volume hierarchy.

Supported graphics APIs, RDNA hardware, and operating systems
-------------------------------------------------------------

**Supported APIs**

-  DirectX 12

-  Vulkan

\ **Supported RDNA hardware**

-  AMD Radeon RX 7000 series

-  AMD Radeon RX 6000 series

\ **Supported Operating Systems**

-  Windows® 10

-  Windows® 11

-  Ubuntu® 22.04 LTS (Vulkan only)

Radeon Raytracing Analyzer - Quick Start
========================================

.. include:: capture.rst

Starting the Radeon Raytracing Analyzer
---------------------------------------

Start **RadeonRaytracingAnalyzer.exe** (this is the tool used to view BVH trace data).

How to load a trace
-------------------

There are a few ways to load a trace into RRA.

1) Use the “File/Open trace” pull down menu, or the “File/Recent
   trace” pull down menu item.

2) Go to the “Welcome” view and click on the “Open a Radeon Raytracing Analyzer file…”

3) Go to the “Welcome” view and click on a trace that you have
   previously loaded in the Recent list.

.. image:: media/welcome_1.png

4) Go to the Recent traces view to see a full list of all your recent traces.

  Notice that there is additional information provided for each trace when
  viewed in this pane, such as the date when the trace was last accessed. It is
  also possible to remove recent traces from the list using the "Remove from list"
  link button. Note that they will only be removed from the list; they won't be
  deleted from the file system. There is also a link button, "Open file location"
  to open the folder where that trace file is on the disk.

.. image:: media/recent_traces_1.png

5) Drag and drop a BVH trace file onto the **Radeon Raytracing Analyzer**
   executable, or onto an already open RRA instance.

The Radeon Raytracing Analyzer user interface
---------------------------------------------

There are four main menus in the Radeon Raytracing Analyzer and each may have a
number of sub-windows..

1. **START**

   a. **Welcome** - Shows links to help documentation, and a list of
      recently opened traces.

   b. **Recent traces** - Displays a list of the recently opened
      traces.

   c. **About** - Shows build information about RRA and useful links.

2. **OVERVIEW**

   a. **Summary** - Gives an overview of the trace, showing the number
      of acceleration structures, the resources they use and their memory usage.

   b. **Device configuration** - Provides details of the GPU used to
      record the trace.

3. **TLAS**

   a. **Viewer** - The main viewer for a top-level acceleration structure.

   b. **Instances** - Lists statistics for all instances referenced in the selected
      top-level acceleration structure.

   c. **BLAS List** - Lists statistics for all bottom-level acceleration
      structures referenced in the selected TLAS.

   d. **Properties** - Lists statistics and properties for the currently
      selected top-level acceleration structure.

4. **BLAS**

   a. **Viewer** - The main viewer for a bottom-level acceleration structure.

   b. **Instances** - Lists statistics for all instances of the selected
      bottom-level acceleration structure.

   c. **Triangles** - Lists statistics for all triangles of the selected
      bottom-level acceleration structure.

   d. **Geometries** - Lists statistics for all geometries of the selected
      selected bottom-level acceleration structure.

   e. **Properties** - Lists statistics and properties for the currently
      selected bottom-level acceleration structure.

Settings
========

The settings tab is used to control global settings throughout the product.
These settings are stored, and are persistent for all instances of RRA.

.. include:: settings.rst

UI Navigation
-------------

In an effort to improve workflow, RRA supports keyboard shortcuts and
back and forward history to quickly navigate throughout the UI.

Back and forward navigation
~~~~~~~~~~~~~~~~~~~~~~~~~~~

RRA tracks navigation history, which allows users to navigate back and
forward between all of RRA’s panes. This is achieved using global
navigation **hotkeys** shown above, or the back and forward **buttons**
on all panes in the top left below the file menu.

Currently, back and forward navigation is restricted to pane switches.

The OVERVIEW windows
====================

.. include:: overview.rst

The TLAS windows
================

These panes show information about a top-level acceleration structure
and its associated bottom-level acceleration structures.

.. include:: tlas_viewer.rst

.. include:: tlas_instance_list.rst

.. include:: blas_list.rst

.. include:: tlas_properties.rst

The BLAS windows
================

These panes show information about a bottom-level acceleration structure.

.. include:: blas_viewer.rst

.. include:: blas_instance_list.rst

.. include:: triangle_list.rst

.. include:: geometries_list.rst

.. include:: blas_properties.rst

DISCLAIMER
==========
The information contained herein is for informational purposes only, and is subject to change without notice. While every
precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical
errors, and AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no
representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no
liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with
respect to the operation or use of AMD hardware, software or other products described herein. No license, including implied or
arising by estoppel, to any intellectual property rights is granted by this document. Terms and limitations applicable to the purchase
or use of AMD’s products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions
of Sale.

AMD, the AMD Arrow logo, Radeon, Ryzen, RDNA and combinations thereof are trademarks of Advanced Micro Devices, Inc. Other product names used in
this publication are for identification purposes only and may be trademarks of their respective companies.

DirectX is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Vulkan and the Vulkan logo are registered trademarks of the Khronos Group Inc.

Microsoft is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Windows is a registered trademark of Microsoft Corporation in the US and other jurisdictions.


© 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
