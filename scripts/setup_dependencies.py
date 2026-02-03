#!/usr/bin/env python3
"""
Setup script for downloading third-party dependencies
Downloads GLAD, stb_image, and other required libraries
"""

import os
import sys
import urllib.request
import ssl

# 禁用 SSL 验证（某些环境需要）
ssl._create_default_https_context = ssl._create_unverified_context

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
THIRD_PARTY_DIR = os.path.join(PROJECT_ROOT, "third_party")

def download_file(url, dest_path):
    """Download a file from URL to destination path"""
    print(f"Downloading: {url}")
    print(f"  -> {dest_path}")
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    try:
        urllib.request.urlretrieve(url, dest_path)
        print("  Done!")
        return True
    except Exception as e:
        print(f"  Error: {e}")
        return False

def setup_stb():
    """Download stb_image.h"""
    print("\n=== Setting up stb ===")
    stb_dir = os.path.join(THIRD_PARTY_DIR, "stb")
    os.makedirs(stb_dir, exist_ok=True)
    
    files = [
        ("stb_image.h", "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"),
        ("stb_image_write.h", "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"),
    ]
    
    for filename, url in files:
        dest = os.path.join(stb_dir, filename)
        if not os.path.exists(dest):
            download_file(url, dest)
        else:
            print(f"  {filename} already exists, skipping...")

def setup_glad():
    """Setup GLAD - OpenGL Loader"""
    print("\n=== Setting up GLAD ===")
    glad_dir = os.path.join(THIRD_PARTY_DIR, "glad")
    
    # Check if already exists
    if os.path.exists(os.path.join(glad_dir, "src", "glad.c")):
        print("  GLAD already exists, skipping...")
        return True
    
    print("  GLAD needs to be generated. Options:")
    print("  1. Use pip: pip install glad")
    print("     Then run: python -m glad --generator=c --api gl:core=4.3 --out-path=third_party/glad")
    print("  2. Download from: https://glad.dav1d.de/")
    print("     Settings: Language=C/C++, Specification=OpenGL, API=gl 4.3, Profile=Core")
    print("")
    
    # Try to use glad package if installed
    try:
        import subprocess
        result = subprocess.run([
            sys.executable, "-m", "glad",
            "--generator=c",
            "--api", "gl:core=4.3",
            "--out-path", glad_dir
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("  GLAD generated successfully!")
            return True
        else:
            print(f"  GLAD generation failed: {result.stderr}")
    except Exception as e:
        print(f"  Could not run glad: {e}")
    
    print("\n  Please install glad manually:")
    print("    pip install glad")
    print("    python -m glad --generator=c --api gl:core=4.3 --out-path=third_party/glad")
    return False

def create_directory_structure():
    """Create required directory structure"""
    print("\n=== Creating directory structure ===")
    
    dirs = [
        "src/core",
        "src/renderer", 
        "src/transpiler",
        "src/input",
        "src/ui",
        "src/utils",
        "shaders/common",
        "shaders/examples",
        "resources/textures",
        "resources/fonts",
        "third_party/glad/include/glad",
        "third_party/glad/include/KHR",
        "third_party/glad/src",
        "third_party/stb",
    ]
    
    for d in dirs:
        path = os.path.join(PROJECT_ROOT, d)
        os.makedirs(path, exist_ok=True)
        print(f"  Created: {d}")

def main():
    print("=" * 50)
    print("Local Shadertoy - Dependency Setup")
    print("=" * 50)
    
    create_directory_structure()
    setup_stb()
    glad_ok = setup_glad()
    
    print("\n" + "=" * 50)
    print("Setup Complete!")
    if not glad_ok:
        print("WARNING: GLAD needs to be set up manually (see instructions above)")
    print("=" * 50)

if __name__ == "__main__":
    main()
