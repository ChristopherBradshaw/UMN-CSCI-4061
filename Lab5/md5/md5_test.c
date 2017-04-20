
#include "md5sum.h"

int main(int argc, char **argv)
{
    int i;
    unsigned char sum[MD5_DIGEST_LENGTH];

    if (argc != 2) {
        fprintf(stderr, "Incorrect arguments..\n");
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    md5sum(argv[1], sum);

    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
        printf("%02x", sum[i]);
    printf("\n");
    exit(EXIT_SUCCESS);
}
