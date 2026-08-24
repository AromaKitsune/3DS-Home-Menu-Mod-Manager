#include <3ds.h>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

const std::string TITLES_PATH = "sdmc:/luma/titles/";
std::string TARGET_TID = "";
std::string REGION_NAME = "";
const int MAX_VISIBLE_ITEMS = 22; // Safely fits below the 30-row limit on the top screen

struct ModInfo {
    std::string folderName;
    std::string modName;
    bool isActive;
};

// Initializes the CFGU service to detect console region and set the target TID
void initRegionAndTID() {
    u8 region = CFG_REGION_EUR; // Default fallback

    // Initialize configuration service
    if (R_SUCCEEDED(cfguInit())) {
        CFGU_SecureInfoGetRegion(&region);
        cfguExit();
    }

    // Map the detected region to the correct Home Menu Title ID
    switch (region) {
        case CFG_REGION_JPN:
            TARGET_TID = "0004003000008202";
            REGION_NAME = "JPN";
            break;
        case CFG_REGION_USA:
            TARGET_TID = "0004003000008F02";
            REGION_NAME = "USA";
            break;
        case CFG_REGION_EUR:
        case CFG_REGION_AUS:
            TARGET_TID = "0004003000009802";
            REGION_NAME = "EUR";
            break;
        case CFG_REGION_CHN:
            TARGET_TID = "000400300000B102";
            REGION_NAME = "CHN";
            break;
        case CFG_REGION_KOR:
            TARGET_TID = "000400300000A902";
            REGION_NAME = "KOR";
            break;
        case CFG_REGION_TWN:
            TARGET_TID = "000400300000B202";
            REGION_NAME = "TWN";
            break;
        default:
            TARGET_TID = "0004003000009802";
            REGION_NAME = "Unknown/EUR";
            break;
    }
}

// Converts forbidden characters to '_'
std::string sanitizeFolderName(std::string input) {
    const std::string forbidden = "<>:\"/\\|?*";
    for (char& c : input) {
        if (forbidden.find(c) != std::string::npos || c < 32) {
            c = '_';
        }
    }
    return input;
}

// Reads the mod name from mod_name.txt
std::string readModName(const std::string& folderPath) {
    std::ifstream file(folderPath + "/mod_name.txt");
    if (file.is_open()) {
        std::string name;
        std::getline(file, name);
        // Strip carriage return if the file was saved on Windows (CRLF)
        if (!name.empty() && name.back() == '\r') {
            name.pop_back();
        }
        return name.empty() ? "Unknown Mod" : name;
    }
    return "Unknown Mod";
}

// Scans the titles directory for matching mods and sorts them
void scanMods(std::vector<ModInfo>& mods) {
    mods.clear();
    DIR* dir = opendir(TITLES_PATH.c_str());
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        std::string dname = ent->d_name;

        // Check if the directory starts with our target Title ID
        if (dname.find(TARGET_TID) == 0) {
            ModInfo info;
            info.folderName = dname;
            info.isActive = (dname == TARGET_TID);
            info.modName = readModName(TITLES_PATH + dname);
            mods.push_back(info);
        }
    }
    closedir(dir);

    // Sort naturally by mod name so numbers are evaluated mathematically
    std::sort(mods.begin(), mods.end(), [](const ModInfo& a, const ModInfo& b) {
        const std::string& s1 = a.modName;
        const std::string& s2 = b.modName;
        size_t i = 0, j = 0;

        while (i < s1.length() && j < s2.length()) {
            if (std::isdigit(s1[i]) && std::isdigit(s2[j])) {
                unsigned long long num1 = 0, num2 = 0;
                while (i < s1.length() && std::isdigit(s1[i])) {
                    num1 = num1 * 10 + (s1[i] - '0');
                    i++;
                }
                while (j < s2.length() && std::isdigit(s2[j])) {
                    num2 = num2 * 10 + (s2[j] - '0');
                    j++;
                }
                if (num1 != num2) return num1 < num2;
            } else {
                if (s1[i] != s2[j]) return s1[i] < s2[j];
                i++;
                j++;
            }
        }
        return s1.length() < s2.length();
    });
}

// Restores the currently active mod back to an inactive state
void deactivateCurrentMod(std::vector<ModInfo>& mods) {
    for (auto& mod : mods) {
        if (mod.isActive) {
            std::string safeName = sanitizeFolderName(mod.modName);
            std::string oldPath = TITLES_PATH + TARGET_TID;
            std::string newPath = TITLES_PATH + TARGET_TID + " [" + safeName + "]";

            rename(oldPath.c_str(), newPath.c_str());

            mod.folderName = TARGET_TID + " [" + safeName + "]";
            mod.isActive = false;
            break;
        }
    }
}

int main(int argc, char **argv) {
    gfxInitDefault();

    // Initialize separate consoles for the top and touch screens
    PrintConsole topScreen, bottomScreen;
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);

    // Identify console region and set TID before scanning
    initRegionAndTID();

    std::vector<ModInfo> mods;
    scanMods(mods);

    int selectedIndex = 0;
    int listOffset = 0; // Tracks the first item drawn on screen
    bool needsRedraw = true; // Add a flag to track when the UI should update

    // Main loop
    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break;
        }

        // Navigation
        if (kDown & KEY_DOWN) {
            if (!mods.empty()) {
                selectedIndex = (selectedIndex + 1) % mods.size();

                // Adjust list offset for scrolling down or wrapping to the top
                if (selectedIndex == 0) {
                    listOffset = 0;
                } else if (selectedIndex >= listOffset + MAX_VISIBLE_ITEMS) {
                    listOffset = selectedIndex - MAX_VISIBLE_ITEMS + 1;
                }

                needsRedraw = true; // Trigger UI redraw
            }
        }
        if (kDown & KEY_UP) {
            if (!mods.empty()) {
                selectedIndex = (selectedIndex - 1 + mods.size()) % mods.size();

                // Adjust list offset for scrolling up or wrapping to the bottom
                if (selectedIndex == (int)mods.size() - 1) {
                    listOffset = std::max(0, (int)mods.size() - MAX_VISIBLE_ITEMS);
                } else if (selectedIndex < listOffset) {
                    listOffset = selectedIndex;
                }

                needsRedraw = true; // Trigger UI redraw
            }
        }
        if (kDown & KEY_RIGHT) {
            if (!mods.empty()) {
                selectedIndex += 5;

                // Clamp to the last item
                if (selectedIndex >= (int)mods.size()) {
                    selectedIndex = (int)mods.size() - 1;
                }

                // Adjust list offset to keep the selection visible
                if (selectedIndex >= listOffset + MAX_VISIBLE_ITEMS) {
                    listOffset = selectedIndex - MAX_VISIBLE_ITEMS + 1;
                }

                needsRedraw = true; // Trigger UI redraw
            }
        }
        if (kDown & KEY_LEFT) {
            if (!mods.empty()) {
                selectedIndex -= 5;

                // Clamp to the first item
                if (selectedIndex < 0) {
                    selectedIndex = 0;
                }

                // Adjust list offset to keep the selection visible
                if (selectedIndex < listOffset) {
                    listOffset = selectedIndex;
                }

                needsRedraw = true; // Trigger UI redraw
            }
        }

        // [Y] Restore Original Home Menu
        if (kDown & KEY_Y) {
            if (!mods.empty()) {
                std::string currentSelection = mods[selectedIndex].modName;

                deactivateCurrentMod(mods);
                scanMods(mods);

                // Restore cursor index and verify offset bounds
                for (size_t i = 0; i < mods.size(); ++i) {
                    if (mods[i].modName == currentSelection) {
                        selectedIndex = i;
                        if (selectedIndex < listOffset) listOffset = selectedIndex;
                        if (selectedIndex >= listOffset + MAX_VISIBLE_ITEMS) listOffset = selectedIndex - MAX_VISIBLE_ITEMS + 1;
                        break;
                    }
                }
                needsRedraw = true; // Trigger UI redraw
            }
        }

        // [A] Apply Mod
        if (kDown & KEY_A) {
            if (!mods.empty() && !mods[selectedIndex].isActive) {
                std::string currentSelection = mods[selectedIndex].modName;

                // Disable the current mod first
                deactivateCurrentMod(mods);

                // Activate the selected mod
                std::string oldPath = TITLES_PATH + mods[selectedIndex].folderName;
                std::string newPath = TITLES_PATH + TARGET_TID;
                rename(oldPath.c_str(), newPath.c_str());

                // Rescan filesystem to update state
                scanMods(mods);

                // Restore cursor index and verify offset bounds
                for (size_t i = 0; i < mods.size(); ++i) {
                    if (mods[i].modName == currentSelection) {
                        selectedIndex = i;
                        if (selectedIndex < listOffset) listOffset = selectedIndex;
                        if (selectedIndex >= listOffset + MAX_VISIBLE_ITEMS) listOffset = selectedIndex - MAX_VISIBLE_ITEMS + 1;
                        break;
                    }
                }
                needsRedraw = true; // Trigger UI redraw
            }
        }

        // [Select] Reboot
        if (kDown & KEY_SELECT) {
            if (R_SUCCEEDED(ptmSysmInit())) {
                PTMSYSM_RebootAsync(0);
                ptmSysmExit();
            }
        }

        // Rendering the UI (Only runs when a button is pressed)
        if (needsRedraw) {
            // --- TOP SCREEN ---
            consoleSelect(&topScreen);
            consoleClear();
            printf("\x1b[1;1HHome Menu Mod Manager");
            printf("\x1b[3;1HTitle ID: %s (%s)", TARGET_TID.c_str(), REGION_NAME.c_str());

            if (mods.empty()) {
                printf("\x1b[5;1HNo mods found in:");
                printf("\x1b[6;1H%s", TITLES_PATH.c_str());
            } else {
                // Constrain the drawing loop to only visible items
                int limit = std::min((int)mods.size(), listOffset + MAX_VISIBLE_ITEMS);

                for (int i = listOffset; i < limit; ++i) {
                    bool isHighlighted = (i == selectedIndex);
                    bool isActive = mods[i].isActive;

                    // Determine conditional color assignment
                    const char* colorCode = "\x1b[0m"; // Default (Reset)
                    if (isHighlighted) {
                        colorCode = "\x1b[36m"; // Cyan
                    } else if (isActive) {
                        colorCode = "\x1b[32m"; // Green
                    }

                    // Calculate correct vertical position independent of index
                    int displayRow = 5 + (i - listOffset);

                    // Draw cursor
                    if (isHighlighted) {
                        printf("\x1b[%d;1H%s>\x1b[0m", displayRow, colorCode);
                    }

                    // Draw mod name with assigned color
                    printf("\x1b[%d;3H%s%s", displayRow, colorCode, mods[i].modName.c_str());
                    if (isActive) {
                        printf(" [Active]");
                    }

                    // Reset color code to prevent bleeding to the next line
                    printf("\x1b[0m");
                }
            }

            // --- TOUCH SCREEN (BOTTOM) ---
            consoleSelect(&bottomScreen);
            consoleClear();
            printf("\x1b[1;1HControls:");
            printf("\x1b[3;1H[A]      Apply mod");
            printf("\x1b[4;1H[Y]      Restore original Home Menu");
            printf("\x1b[5;1H[SELECT] Reboot system");
            printf("\x1b[6;1H[START]  Exit");

            gfxFlushBuffers(); // Ensure the text is pushed to the screen
            needsRedraw = false; // Reset the flag so it doesn't draw next frame
        }
    }

    gfxExit();
    return 0;
}
