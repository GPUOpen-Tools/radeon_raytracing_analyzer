The Radeon™ Raytracing Analyzer (RRA)
=====================================

The Radeon Raytracing Analyzer is a tool designed to help developers improve the raytracing performance on AMD Radeon 6000 and 7000 series GPU's.
The tool focuses on the visualization of Acceleration Structures, which in our case consist of Bounding Volume Hierarchies. RRA allows the developer
to visualize the bounding box hierarchies, and related scene geometries, to quickly identify issues with the bounding volume hierarchies, such as
overlapping bounding volumes and sparse geometry layout within bounding volumes. Once identified, the developer can revisit their BVH generation
strategy.

This document describes how the Radeon Raytracing Analyzer can be used to examine a bounding volume hierarchy.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   quickstart.rst
   settings.rst
   overview.rst
   tlas_windows.rst
   blas_windows.rst
   ray_windows.rst

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

DISCLAIMER
----------
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


© 2022-2024 Advanced Micro Devices, Inc. All rights reserved.

