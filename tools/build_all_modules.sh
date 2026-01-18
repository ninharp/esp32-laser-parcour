#!/bin/bash

# Build All Module Roles Script
# Automatically builds firmware for all three module types: Control, Laser, Finish
# 
# Author: ninharp
# Date: 2026-01-18

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ESP32 Laser Parcour - Build All Modules${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Backup current sdkconfig
if [ -f "sdkconfig" ]; then
    echo -e "${YELLOW}Backing up current sdkconfig...${NC}"
    cp sdkconfig sdkconfig.backup
fi

# Function to set module configuration
configure_module() {
    local ROLE=$1
    local MODULE_ID=$2
    local ENABLE_SOUND=$3
    
    echo -e "${GREEN}Configuring ${ROLE} Module (ID: ${MODULE_ID})...${NC}"
    
    # Start with defaults
    cp sdkconfig.defaults sdkconfig
    
    # Set module role
    sed -i.tmp "s/^CONFIG_MODULE_ROLE_CONTROL=y/# CONFIG_MODULE_ROLE_CONTROL is not set/" sdkconfig
    sed -i.tmp "s/^CONFIG_MODULE_ROLE_LASER=y/# CONFIG_MODULE_ROLE_LASER is not set/" sdkconfig
    sed -i.tmp "s/^CONFIG_MODULE_ROLE_FINISH=y/# CONFIG_MODULE_ROLE_FINISH is not set/" sdkconfig
    
    case $ROLE in
        "CONTROL")
            echo "CONFIG_MODULE_ROLE_CONTROL=y" >> sdkconfig
            ;;
        "LASER")
            echo "CONFIG_MODULE_ROLE_LASER=y" >> sdkconfig
            ;;
        "FINISH")
            echo "CONFIG_MODULE_ROLE_FINISH=y" >> sdkconfig
            ;;
    esac
    
    # Set module ID
    sed -i.tmp "s/^CONFIG_MODULE_ID=.*/CONFIG_MODULE_ID=${MODULE_ID}/" sdkconfig
    
    # Set sound manager
    if [ "$ENABLE_SOUND" = "true" ]; then
        sed -i.tmp "s/^# CONFIG_ENABLE_SOUND_MANAGER is not set/CONFIG_ENABLE_SOUND_MANAGER=y/" sdkconfig
    else
        sed -i.tmp "s/^CONFIG_ENABLE_SOUND_MANAGER=y/# CONFIG_ENABLE_SOUND_MANAGER is not set/" sdkconfig
    fi
    
    # Clean up temp files
    rm -f sdkconfig.tmp
}

# Function to build and merge
build_module() {
    local ROLE=$1
    
    echo -e "${BLUE}Building ${ROLE} Module...${NC}"
    
    # Clean build directory to avoid conflicts
    idf.py fullclean > /dev/null 2>&1 || true
    
    # Build
    if idf.py build; then
        echo -e "${GREEN}✓ Build successful${NC}"
    else
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    fi
    
    # Merge binaries
    echo -e "${BLUE}Merging binaries...${NC}"
    if idf.py merge; then
        echo -e "${GREEN}✓ Merge successful${NC}"
    else
        echo -e "${RED}✗ Merge failed${NC}"
        return 1
    fi
    
    echo ""
}

# Build Control Module (ID: 1, Sound Manager enabled)
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Building CONTROL Module${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
# First build: Allow version bump
unset NO_BUMP_VERSION
configure_module "CONTROL" 1 "true"
build_module "CONTROL"

# Build Laser Module (ID: 2, Sound Manager disabled)
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Building LASER Module${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
# Subsequent builds: Prevent version bump (keep same version as Control)
export NO_BUMP_VERSION=1
configure_module "LASER" 2 "false"
build_module "LASER"

# Build Finish Module (ID: 3, Sound Manager disabled)
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Building FINISH Module${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
# Keep NO_BUMP_VERSION=1 for consistent versioning
configure_module "FINISH" 3 "false"
build_module "FINISH"

# Restore original sdkconfig
if [ -f "sdkconfig.backup" ]; then
    echo -e "${YELLOW}Restoring original sdkconfig...${NC}"
    mv sdkconfig.backup sdkconfig
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}All modules built successfully!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Firmware files location:"
echo -e "  ${BLUE}Control:${NC} dist/control_module/"
echo -e "  ${BLUE}Laser:${NC}   dist/laser_module/"
echo -e "  ${BLUE}Finish:${NC}  dist/finish_module/"
echo ""
