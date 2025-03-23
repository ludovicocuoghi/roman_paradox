#include "ResourcePath.h"
#include <iostream>
#include <filesystem>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#endif

std::string getResourcePath(const std::string& relativePath) {
    static std::string cachedBasePath = "";
    
    // Only determine the base path once for efficiency
    if (cachedBasePath.empty()) {
        std::vector<std::string> possiblePaths;
        
        #ifdef __APPLE__
        // Try to get path from bundle first
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (mainBundle) {
            CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
            if (resourcesURL) {
                char path[PATH_MAX];
                if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
                    possiblePaths.push_back(std::string(path) + "/");
                    possiblePaths.push_back(std::string(path) + "/assets/");
                }
                CFRelease(resourcesURL);
            }
        }
        
        // Get executable path as fallback
        char execPath[PATH_MAX];
        uint32_t size = sizeof(execPath);
        if (_NSGetExecutablePath(execPath, &size) == 0) {
            std::string exePath(execPath);
            size_t lastSlash = exePath.find_last_of('/');
            if (lastSlash != std::string::npos) {
                std::string execDir = exePath.substr(0, lastSlash + 1);
                possiblePaths.push_back(execDir);
                possiblePaths.push_back(execDir + "assets/");
                possiblePaths.push_back(execDir + "../assets/");
                possiblePaths.push_back(execDir + "Resources/assets/");
                // Add levels directory paths
                possiblePaths.push_back(execDir + "levels/");
                possiblePaths.push_back(execDir + "assets/levels/");
                possiblePaths.push_back(execDir + "../assets/levels/");
                possiblePaths.push_back(execDir + "Resources/assets/levels/");
            }
        }
        #endif
        
        // Common relative paths for development environment
        possiblePaths.push_back("./");
        possiblePaths.push_back("./assets/");
        possiblePaths.push_back("../assets/");
        possiblePaths.push_back("../../assets/");
        possiblePaths.push_back("bin/");
        possiblePaths.push_back("bin/assets/");
        
        // Add levels-specific paths for development environment
        possiblePaths.push_back("./levels/");
        possiblePaths.push_back("./assets/levels/");
        possiblePaths.push_back("../assets/levels/");
        possiblePaths.push_back("../../assets/levels/");
        possiblePaths.push_back("bin/levels/");
        possiblePaths.push_back("bin/assets/levels/");
        
        // Check which path contains a known asset file or directory
        for (const auto& path : possiblePaths) {
            // Check if the path exists
            if (std::filesystem::exists(path)) {
                std::cout << "Found valid resource path: " << path << std::endl;
                cachedBasePath = path;
                break;
            }
        }
        
        // If we still didn't find a valid path, use a default
        if (cachedBasePath.empty()) {
            std::cout << "Warning: Could not determine resource path, using current directory." << std::endl;
            cachedBasePath = "./";
        }
        
        std::cout << "Set resource base path to: " << cachedBasePath << std::endl;
    }
    
    // Combine with relative path
    std::string fullPath = cachedBasePath + relativePath;
    
    // Log the path access for debugging
    if (!std::filesystem::exists(fullPath)) {
        std::cout << "Warning: Resource not found: " << fullPath << std::endl;
    }
    
    return fullPath;
}

// Add a debug function to help find resource issues
void printResourceInfo() {
    std::cout << "\n--- Resource Path Debug Info ---\n";
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";
    std::cout << "Resource base path: " << getResourcePath("") << "\n";
    
    // Try to find some common asset types
    std::vector<std::string> testFiles = {
        "images/player.png",
        "fonts/arial.ttf",
        "levels/level1.txt",
        "assets.txt"
    };
    
    // Add a test specifically for the levels directory
    std::cout << "Checking for levels directory: " << getResourcePath("levels") << " (exists: "
              << (std::filesystem::exists(getResourcePath("levels")) ? "YES" : "NO") << ")\n";
    
    // Try to list contents of levels directory if it exists
    std::string levelsPath = getResourcePath("levels");
    if (std::filesystem::exists(levelsPath) && std::filesystem::is_directory(levelsPath)) {
        std::cout << "Contents of levels directory:\n";
        for (const auto& entry : std::filesystem::directory_iterator(levelsPath)) {
            std::cout << "  - " << entry.path().filename().string() << "\n";
        }
    }
    
    for (const auto& file : testFiles) {
        std::string path = getResourcePath(file);
        std::cout << "Path for '" << file << "': " << path << " (exists: " 
                  << (std::filesystem::exists(path) ? "YES" : "NO") << ")\n";
    }
    
    std::cout << "-------------------------------\n\n";
}
