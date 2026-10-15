#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace p900 {

struct FixResult {
    bool ok = false;
    std::filesystem::path output;
    std::string message;  // error, or a short summary of what was removed
};

// The folder fixed frames go to: <the frame's folder>/CleanedUp.
std::filesystem::path cleanedUpFolder(const std::filesystem::path& input);

// Reads an EXR, removes the column stripe from its colour channels (alpha,
// depth and ids untouched) and writes it, same name, header and compression,
// into cleanedUpFolder(input). Never writes over the input.
FixResult fixExr(const std::filesystem::path& input);

// Expands dropped paths: files as they are, folders to the .exr files in
// them (not recursive, CleanedUp folders skipped). Sorted, duplicates dropped.
std::vector<std::filesystem::path> collectFrames(const std::vector<std::filesystem::path>& dropped);

}  // namespace p900
