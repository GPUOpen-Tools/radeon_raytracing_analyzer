# Radeon™ Raytracing Analyzer

The Radeon Raytracing Analyzer (RRA) is a tool designed to help improve the raytracing performance of AMD GPUs that support raytracing.
The tool thus far focuses on the visualization of the Acceleration Structures, which consist of Bounding Volume Hierarchies.

Game developers are responsible for creating the acceleration structures and so need a method of visualizing these acceleration structures
and how they can affect performance.

RRA allows the developer to visualize the bounding box hierarchies, and related scene geometries, via a standard rasterizer renderer or using
a traversal counter view which will quickly highlight areas of concern. Once identified, the developer can revisit their BVH generation strategy
to reduce performance bottlenecks.

## Getting Started

1. Install the latest AMD Video/display driver. Completely remove previously installed drivers. On Windows, the driver installation factory reset option should be used.
2. Unzip/Untar the download file. The directory includes the following:
   * Radeon Developer Service (RDS)
   * Radeon Developer Service CLI (RDS headless)
   * Radeon Developer Panel (RDP)
   * Radeon Raytracing Analyzer (RRA)
3. To capture a trace from a game, run the Radeon Developer Panel and follow the instructions in the Help. Help can be found in the following locations:
   * Help web pages exist in the "docs" sub directory
   * Help web pages can be accessed from the **Help** button in the Developer Panel
   * Help web pages can be accessed from the Welcome screen in the Radeon Raytracing Analyzer, or from the **Help** menu
   * The documentation is hosted publicly at:
     * https://radeon-developer-panel.readthedocs.io/en/latest/
     * https://radeon-raytracing-analyzer.readthedocs.io/en/latest/

## Supported APIs
 * DirectX12
 * Vulkan

## Supported ASICs
* AMD Radeon RX 7000 series
* AMD Radeon RX 6000 series

## Supported Operating Systems
* Windows® 10
* Windows® 11
* Ubuntu 20.04 LTS (Vulkan only)
* Ubuntu 22.04 LTS (Vulkan only)

## Build instructions
See [BUILD.md](BUILD.md) for more details.

## License ##
Radeon Raytracing Analyzer is licensed under the MIT license. See the [License.txt](License.txt) file for complete license information.

## Copyright information ##
Please see [NOTICES.txt](NOTICES.txt) for third party license information.

## DISCLAIMER ##
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

Visual Studio, DirectX and Windows are registered trademarks of Microsoft Corporation in the US and other jurisdictions.

Vulkan and the Vulkan logo are registered trademarks of the Khronos Group Inc.

Python is a registered trademark of the PSF. The Python logos (in several variants) are use trademarks of the PSF as well.

CMake is a registered trademark of Kitware, Inc.

Qt and the Qt logo are registered trademarks of the Qt Company Ltd and/or its subsidiaries worldwide.


© 2022 Advanced Micro Devices, Inc. All rights reserved.
