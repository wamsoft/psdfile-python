"""
PSDParser - Higher-level wrapper around the PSD class
"""

import os
import io
from typing import Dict, List, Tuple, Union, Optional, Any
import numpy as np
from .module import PSD, ColorMode, LayerType, BlendMode

class PSDParser:
    """
    A high-level interface for parsing and extracting data from PSD files.
    This class wraps the lower-level C++ bindings for a more Pythonic experience.
    """
    
    def __init__(self, file_path=None, file_data=None):
        """
        Initialize a new PSDParser instance
        
        Args:
            file_path: Optional path to a PSD file
            file_data: Optional bytes containing PSD data
        """
        self.psd = PSD()
        self._loaded = False
        
        if file_path or file_data:
            self.load(file_path if file_path else file_data)
    
    @property
    def is_loaded(self) -> bool:
        """Check if a PSD file is loaded"""
        return self._loaded
    
    def load(self, source: Union[str, bytes, io.BytesIO]) -> bool:
        """
        Load PSD data from a file path, bytes, or BytesIO object
        
        Args:
            source: File path, bytes, or BytesIO object containing PSD data
            
        Returns:
            bool: True if loading was successful, False otherwise
        """
        try:
            if isinstance(source, str):
                # Load from file path
                result = self.psd.load_from_file(source)
            elif isinstance(source, bytes):
                # Load from bytes
                result = self.psd.load_from_bytes(source)
            elif isinstance(source, io.BytesIO):
                # Load from BytesIO
                result = self.psd.load_from_bytes(source.getvalue())
            else:
                raise TypeError(f"Unsupported source type: {type(source)}")
            
            self._loaded = result
            return result
        except Exception as e:
            self._loaded = False
            raise RuntimeError(f"Failed to load PSD: {str(e)}") from e
    
    def parse(self):
        """
        Parse the loaded PSD file and return basic information
        
        Returns:
            Dict: Basic information about the PSD file
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_basic_info()
    
    @property
    def info(self) -> Dict[str, Any]:
        """Get basic information about the PSD file"""
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_basic_info()
    
    @property
    def width(self) -> int:
        """Get the width of the PSD image"""
        return self.psd.width
    
    @property
    def height(self) -> int:
        """Get the height of the PSD image"""
        return self.psd.height
    
    @property
    def channels(self) -> int:
        """Get the number of channels in the PSD image"""
        return self.psd.channels
    
    @property
    def depth(self) -> int:
        """Get the bit depth of the PSD image"""
        return self.psd.depth
    
    @property
    def color_mode(self) -> int:
        """Get the color mode of the PSD image"""
        return self.psd.color_mode
    
    @property
    def layer_count(self) -> int:
        """Get the number of layers in the PSD image"""
        return self.psd.layer_count
    
    def get_layer_type(self, layer_no: int) -> int:
        """
        Get the type of a layer
        
        Args:
            layer_no: Layer index (0-based)
            
        Returns:
            int: Layer type constant
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_type(layer_no)
    
    def get_layer_name(self, layer_no: int) -> str:
        """
        Get the name of a layer
        
        Args:
            layer_no: Layer index (0-based)
            
        Returns:
            str: Layer name
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_name(layer_no)
    
    def get_layer_info(self, layer_no: int) -> Dict[str, Any]:
        """
        Get detailed information about a layer
        
        Args:
            layer_no: Layer index (0-based)
            
        Returns:
            Dict: Layer information dictionary
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_info(layer_no)
    
    def get_layer_data(self, layer_no: int) -> np.ndarray:
        """
        Get layer image data with mask applied
        
        Args:
            layer_no: Layer index (0-based)
            
        Returns:
            numpy.ndarray: Layer image data as BGRA numpy array
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_data(layer_no)
    
    def get_layer_data_raw(self, layer_no: int) -> np.ndarray:
        """
        Get raw layer image data without mask applied
        
        Args:
            layer_no: Layer index (0-based)
            
        Returns:
            numpy.ndarray: Raw layer image data as BGRA numpy array
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_data_raw(layer_no)
    
    def get_layer_data_mask(self, layer_no: int) -> np.ndarray:
        """
        Get layer mask data
        
        Args:
            layer_no: Layer index (0-based)
            
        Returns:
            numpy.ndarray: Layer mask data as BGRA numpy array
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_data_mask(layer_no)
    
    def get_blend(self) -> np.ndarray:
        """
        Get the composite image
        
        Returns:
            numpy.ndarray: Composite image data as BGRA numpy array
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_blend()
    
    def get_slices(self) -> Optional[Dict[str, Any]]:
        """
        Get slice information
        
        Returns:
            Dict or None: Slice information dictionary, or None if no slices exist
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_slices()
    
    def get_guides(self) -> Optional[Dict[str, Any]]:
        """
        Get guide information
        
        Returns:
            Dict or None: Guide information dictionary, or None if no guides exist
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_guides()
    
    def get_layer_comp(self) -> Optional[Dict[str, Any]]:
        """
        Get layer composition information
        
        Returns:
            Dict or None: Layer composition dictionary, or None if no compositions exist
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.get_layer_comp()
    
    def assign_auto_ids(self, base_id: int = 0) -> int:
        """
        Assign automatic IDs to layers
        
        Args:
            base_id: Base ID (default: 0)
            
        Returns:
            int: Number of layers that were assigned IDs
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        return self.psd.assign_auto_ids(base_id)
    
    def get_all_layers_info(self) -> List[Dict[str, Any]]:
        """
        Get detailed information about all layers
        
        Returns:
            List[Dict]: List of layer information dictionaries
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        
        return [self.get_layer_info(i) for i in range(self.layer_count)]
    
    def extract_all_layers(self, get_mask: bool = False) -> Dict[str, np.ndarray]:
        """
        Extract all layer images
        
        Args:
            get_mask: Whether to also extract layer masks (default: False)
            
        Returns:
            Dict[str, np.ndarray]: Dictionary mapping layer names to image data
        """
        if not self._loaded:
            raise RuntimeError("No PSD data loaded")
        
        result = {}
        for i in range(self.layer_count):
            name = self.get_layer_name(i)
            unique_name = name
            counter = 1
            
            # Handle duplicate layer names
            while unique_name in result:
                unique_name = f"{name} ({counter})"
                counter += 1
            
            result[unique_name] = self.get_layer_data(i)
            
            if get_mask:
                mask_name = f"{unique_name} (mask)"
                mask_data = self.get_layer_data_mask(i)
                
                # Check if mask has data (not a dummy 1x1 mask)
                if mask_data.shape[0] > 1 or mask_data.shape[1] > 1:
                    result[mask_name] = mask_data
        
        return result