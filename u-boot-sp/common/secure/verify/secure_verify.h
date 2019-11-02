#ifndef SECURE_VERIFY_H
#define SECURE_VERIFY_H

#include <command.h>
#include <image.h>

int verify_kernel_signature(const image_header_t  *hdr);

#endif
