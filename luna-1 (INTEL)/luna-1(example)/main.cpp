#include <iostream>
#include "luna-1.hpp"

int main()
{
    if (!i6::begin()) // call this before anything else.
    {
        std::cout << "[!] failed to init..." << std::endl;
        std::cin.get();
        return -1;
    }
    else
    {
        auto notepad_pid = i6::get_pid("notepad.exe");
        auto notepad_base = i6::get_process_base(notepad_pid);

        std::cout << "[+] notepad pid: " << notepad_pid << std::endl;
        std::cout << "[+] notepad base address: " << std::hex << notepad_base << std::endl;

        while(true)
            std::cout << "[+] notepad MZ: " << std::hex << i6::read<short>(notepad_pid, notepad_base) << std::endl;
    }
}