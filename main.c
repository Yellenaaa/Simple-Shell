#include "simple_shell.h"

 int main() {
    signal(SIGINT, signal_handler);  // Ignore Ctrl+C to prevent shell exit
    shell_loop();
    return 0;
} 
