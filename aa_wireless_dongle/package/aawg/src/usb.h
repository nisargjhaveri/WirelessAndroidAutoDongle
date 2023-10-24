#include <string>

class UsbManager {
public:
    static UsbManager& instance();

    void init();
    void enableDefaultAndWaitForAccessroy();
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