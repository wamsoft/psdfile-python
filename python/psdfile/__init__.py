# This file initializes the psdfile package and imports the bindings for the psdparse library.

from .module import PSD, ColorMode, LayerType, BlendMode
from .parser import PSDParser

__all__ = ['PSD', 'PSDParser', 'ColorMode', 'LayerType', 'BlendMode']

__version__ = '0.1.0'
__author__ = 'Go Watanabe'
__license__ = 'MIT'