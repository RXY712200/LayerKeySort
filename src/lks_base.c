#include "layerkeysort.h"

const char *lks_status_string(LksStatus status)
{
    switch (status) {
    case LKS_STATUS_OK:
        return "OK";
    case LKS_STATUS_INVALID_ARGUMENT:
        return "Invalid argument";
    case LKS_STATUS_OUT_OF_MEMORY:
        return "Out of memory";
    case LKS_STATUS_BUFFER_TOO_SMALL:
        return "Buffer too small";
    case LKS_STATUS_LEVEL_LIMIT:
        return "Path level limit reached";
    case LKS_STATUS_ALREADY_EXISTS:
        return "Already exists";
    case LKS_STATUS_NOT_FOUND:
        return "Not found";
    case LKS_STATUS_NOT_IMPLEMENTED:
        return "Not implemented";
    case LKS_STATUS_INTERNAL_ERROR:
        return "Internal error";
    default:
        return "Unknown status";
    }
}
