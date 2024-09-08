#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int dev = open("/dev/device_number", O_RDONLY);
    if (dev < 0)
    {
        printf("unable to open file\r\n");
    }
    close(dev);
    return 0;
}
