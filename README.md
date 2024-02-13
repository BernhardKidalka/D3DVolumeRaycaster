# D3DVolumeRaycaster
Volume Raycaster for **3D MIP** Rendering of **MRI** and **CT** volume datasets implemented in C++ and Direct3D 11.

MIP stands for **M**aximum **I**ntensity **P**rojection, which means the projection of voxels of a 3D volume dataset with maximum intensity to a 2D image. It is a method in the medical imaging domain and is very often applied in CT to visualize bone structures or in MRI/CT angiography examinations.

The repository contains four demo datasets, 2 x CT and 2 x MRI (only pixel data, 8 bit resolution - no DICOM dataset) to be able to start immediately to figure out how volume raycasting works.

## Screenshots of the demo datasets

### CT Head, columns : 256, rows : 256, slices : 225
![alt text](doc/screenshots/CT_Head_3DMIP_01.png)

### CT Head Angio, columns : 512, rows : 512, slices : 79
![alt text](doc/screenshots/CT_Head_Angio_3DMIP_01.png)

### MRI Head TOF Angio, columns : 416, rows : 512, slices : 112
![alt text](doc/screenshots/MR_Head_TOF_3DMIP_01.png)

### MRI Abdomen Angio, columns : 384, rows : 512, slices : 80
![alt text](doc/screenshots/MR_Abdomen_Angio_3DMIP_01.png)

## Download and Install

### Clone the repository

```shell
git clone https://github.com/BernhardKidalka/D3DVolumeRaycaster.git
```

If you have no local git client installed, you can also download the zip-archive of the repository and extract it to your local disk.

### Building the source code

**Requirements**

| HW / SW | Required |
| --- | ----------- |
| CPU | Any AMD or Intel x64 CPU, Quad-Core CPU recommended |
| GPU | Any AMD or NVidia DX11 compliant GPU supporting Shader Model 5.0 |
| RAM | PC with at least 8GB RAM |
| Operating System | Windows 10 or later |
| Compiler / IDE | Visual Studio 2022 Community Edition |

The **D3DVolumeRaycaster** runs only on Windows 10 OS, as it is fully implemented on the DX11 technology stack. The hardware requirements should not be very demanding. Any Quad-Core PC not older than 10 years should be able to run the raycaster. Even on-board graphic should be sufficient for execution. Please use the Windows **dxdiag** tool to check your hardware/driver spec.

**How to build**

The repository provides a maintained Visual Studio 2022 solution. The project is pre-configured for x64 target with debug or release build profile.
Please open the solution
```
D3DVolumeRaycaster.sln
```
and build the solution.

## External dependencies

As the **DirectX SDK** is meanwhile part of the **Windows SDK**, no additional SDK installation is necessary. Everything you need comes with the Visual Studio 2022 Community installation. Please ensure that the workloads **C++** and **Game Development with C++** are selected in the Visual Studio Installer.

This project uses th GUI Overlay library **AntTweakBar** for the UI controls. As this project is meanwhile no longer maintained, this repository contains also the lib and dll and takes care to be linked/referenced correctly. A post-build step copies the AntTweakBar.dll to the required target binary folder.

Please refer to the following link to learn more about AntTweakBar:
[link](https://anttweakbar.sourceforge.io/doc/)





