#include "hps.h"

const char *hps_status_string(HpsStatus status)
{
    switch (status) {
    case HPS_STATUS_OK:
        return "OK";
    case HPS_STATUS_INVALID_ARGUMENT:
        return "Invalid argument";
    case HPS_STATUS_OUT_OF_MEMORY:
        return "Out of memory";
    case HPS_STATUS_BUFFER_TOO_SMALL:
        return "Buffer too small";
    case HPS_STATUS_LEVEL_LIMIT:
        return "Path level limit reached";
    case HPS_STATUS_ALREADY_EXISTS:
        return "Already exists";
    case HPS_STATUS_NOT_FOUND:
        return "Not found";
    case HPS_STATUS_NOT_IMPLEMENTED:
        return "Not implemented";
    case HPS_STATUS_INTERNAL_ERROR:
        return "Internal error";
    default:
        return "Unknown status";
    }
}



