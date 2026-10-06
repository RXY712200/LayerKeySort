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
    case LKS_STATUS_ALREADY_EXISTS:
        return "Already exists";
    case LKS_STATUS_NOT_FOUND:
        return "Not found";
    case LKS_STATUS_INTERNAL_ERROR:
        return "Internal error";
    case LKS_STATUS_CAPACITY_LIMIT:
        return "Capacity limit reached";
    case LKS_STATUS_INVALIDATED:
        return "Cursor invalidated";
    case LKS_STATUS_REENTRANT:
        return "Reentrant access rejected";
    case LKS_STATUS_DOMAIN_MISMATCH:
        return "Snapshot key domain mismatch";
    default:
        return "Unknown status";
    }
}
