# PSDParse Python Binding

This project provides Python bindings for the `psdparse` library, allowing you to work with Adobe Photoshop (PSD) files in a Python environment. It enables extraction of layers, images, and metadata from PSD files, which can be manipulated as Python dictionaries.

## Features

- **PSD File Analysis**: Efficiently extract layers, images, metadata, and more
- **NumPy Compatible**: Access image data as NumPy arrays
- **Python Interface**: Easily access parsed data as Python dictionaries or objects
- **C++ Backend**: Implementation based on the high-performance `psdparse` library
- **Memory or File Loading**: Load from file paths or directly from byte data

## Prerequisites

Set up vcpkg on your system. It will be used for library references in cmake.

## Installation

To install the project, clone the repository and run the following command:

```bash
# Specify the path to your vcpkg toolchain file for cmake
export VCPKG_ROOT=c:/work/vcpkg
# set vcpkg triplet (to specify with static boost/zlib library)
export VCPKG_TARGET_TRIPLET=x64-windows-static
pip install . -v
```

```powershell
# windows powershell
$Env:VCPKG_ROOT = "c:/work/vcpkg"
$Env:VCPKG_TARGET_TRIPLET = "x64-windows-static"
pip install . -v
```

## Usage

After installation, you can use the library as follows:

```python
import numpy as np
from psdfile import PSDParser

# Load a PSD file
parser = PSDParser()
parser.load("path/to/your/file.psd")

# Get basic information
print(f"Image size: {parser.width}x{parser.height}")
print(f"Layer count: {parser.layer_count}")

# Get information for all layers
layers_info = parser.get_all_layers_info()
for i, layer_info in enumerate(layers_info):
    print(f"Layer {i}: {layer_info['name']}")

# Get image data for a specific layer as NumPy array
layer_image = parser.get_layer_data(0)  # Get the first layer
print(f"Layer size: {layer_image.shape}")

# Get the composite image
composite_image = parser.get_blend()

# Get slice information
slices = parser.get_slices()
if slices:
    print(f"Slice count: {len(slices['slices'])}")

# Get guide information
guides = parser.get_guides()
if guides:
    print(f"Horizontal guides: {len(guides['horizontal'])}")
    print(f"Vertical guides: {len(guides['vertical'])}")
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details. 
