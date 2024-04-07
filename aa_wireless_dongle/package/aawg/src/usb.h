#include <string>
#include <chrono>

class UsbManager {
public:
    static UsbManager& instance();

    void init();
    bool enableDefaultAndWaitForAccessory(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    void switchToAccessoryGadget();
    void disableGadget();

private:
    UsbManager();
    UsbManager(UsbManager const&);
    UsbManager& operator=(UsbManager const&);

    void writeGadgetFile(std::string gadgetName, std::string relativeFilePath, const char* content);
    void enableGadget(std::string name);
    void disableGadget(std::string name);

    static std::string s_udcName; 
};