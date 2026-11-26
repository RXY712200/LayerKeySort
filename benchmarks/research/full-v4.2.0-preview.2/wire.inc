/* Fixed-width portable shifts, with no alignment or host-byte-order premise. */
static uint64_t snap_be_read(const unsigned char *p, size_t width)
{
    uint32_t high = ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
    if (width == 4) return high;
    uint32_t low = ((uint32_t)p[4]<<24)|((uint32_t)p[5]<<16)|((uint32_t)p[6]<<8)|p[7];
    return ((uint64_t)high<<32)|low;
}
static void snap_be_write(unsigned char *p, uint64_t v, size_t width)
{
    if (width == 8) {
        p[0]=(unsigned char)(v>>56); p[1]=(unsigned char)(v>>48);
        p[2]=(unsigned char)(v>>40); p[3]=(unsigned char)(v>>32);
        p+=4;
    }
    p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16);
    p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v;
}
