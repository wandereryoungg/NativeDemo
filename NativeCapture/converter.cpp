#define LOG_TAG "Convert"

#include <android/log.h>
#include <arm_neon.h>
#include <math.h>

#include <cstdint>
#include <cstring>
#include <vector>


#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#pragma pack(1)
typedef struct _rkrawHeader {
    uint16_t version;
    uint8_t snsName[32];
    uint8_t scenario[32];
    uint32_t frameId;
    uint16_t sns_width;
    uint16_t sns_height;
    uint8_t bpp;
    uint8_t bayer_fmt; /* 0(BGGR);1(GBRG); 2(GRBG);3(RGGB);*/
    uint8_t working_mode; /* 1(linear); 2(long + short);3(long + mid + short); */
    uint8_t storage_type; /* 0(packed); 1(unpacked)*/
    uint16_t line_len;
    uint16_t valid_line_len;
    uint8_t endian;
} rkrawHeader_t;
#pragma pack()

bool ConvertCompact2NonCompact(const uint8_t* srcRaw, uint16_t* dstRaw, rkrawHeader_t* rawFormat) {
    int y, x;
    int line_offset = rawFormat->line_len - rawFormat->valid_line_len;
    printf("%s   line_offset: %d\n", __func__, line_offset);
    if (rawFormat->bpp == 10) {
        for (y = 0; y < rawFormat->sns_height; y++) {
            for (x = 0; x < rawFormat->sns_width; x += 4, srcRaw += 5, dstRaw += 4) {
                dstRaw[0] = (uint16_t)(srcRaw[0] & 0xff) >> 0 | ((srcRaw[1] & 0x03) << 8);
                dstRaw[1] = (uint16_t)(srcRaw[1] & 0xfc) >> 2 | ((srcRaw[2] & 0x0f) << 6);
                dstRaw[2] = (uint16_t)(srcRaw[2] & 0xf0) >> 4 | ((srcRaw[3] & 0x3f) << 4);
                dstRaw[3] = (uint16_t)(srcRaw[3] & 0xc0) >> 6 | ((srcRaw[4] & 0xff) << 2);
            }
            srcRaw = srcRaw + line_offset;
        }
    } else if (rawFormat->bpp == 12) {
        for (y = 0; y < rawFormat->sns_height; y++) {
            for (x = 0; x < rawFormat->sns_width; x += 2, srcRaw += 3, dstRaw += 2) {
                dstRaw[0] = (uint16_t)(srcRaw[0] & 0xff) >> 0 | ((srcRaw[1] & 0x0f) << 8);
                dstRaw[1] = (uint16_t)(srcRaw[1] & 0xf0) >> 4 | ((srcRaw[2] & 0xff) << 4);
            }
            srcRaw = srcRaw + line_offset;
        }
    }
    return true;
}

bool ConvertNonCompact2Compact(const uint16_t* srcRaw, uint8_t* dstRaw,
                               rkrawHeader_t* rawFormat) {
    int y, x;
    int line_offset = rawFormat->line_len - rawFormat->valid_line_len;
    printf("%s   line_offset: %d\n", __func__, line_offset);

    if (rawFormat->bpp == 10) {
        for (y = 0; y < rawFormat->sns_height; y++) {
            for (x = 0; x < rawFormat->sns_width; x += 4, srcRaw += 4, dstRaw += 5) {
                dstRaw[0] = (uint8_t)((srcRaw[0] & 0x00ff) >> 0);
                dstRaw[1] = (uint8_t)((srcRaw[0] & 0x0300) >> 8 | ((srcRaw[1] & 0x003f) << 2));
                dstRaw[2] = (uint8_t)((srcRaw[1] & 0x03c0) >> 6 | ((srcRaw[2] & 0x000f) << 4));
                dstRaw[3] = (uint8_t)((srcRaw[2] & 0x03f0) >> 4 | ((srcRaw[3] & 0x0003) << 6));
                dstRaw[4] = (uint8_t)((srcRaw[3] & 0x03fc) >> 2);
            }
            dstRaw = dstRaw + line_offset;
        }
    } else if (rawFormat->bpp == 12) {
        for (y = 0; y < rawFormat->sns_height; y++) {
            for (x = 0; x < rawFormat->sns_width; x += 2, srcRaw += 2, dstRaw += 3) {
                dstRaw[0] = (uint8_t)((srcRaw[0] & 0x00ff) >> 0);
                dstRaw[1] = (uint8_t)((srcRaw[0] & 0x0f00) >> 8 | ((srcRaw[1] & 0x000f) << 4));
                dstRaw[2] = (uint8_t)((srcRaw[1] & 0x0fff) >> 4);
            }
            dstRaw = dstRaw + line_offset;
        }
    }
    return true;
}

int main() {
    rkrawHeader_t rawHeader;
    rawHeader.sns_width = 3840;
    rawHeader.sns_height = 2160;
    rawHeader.bpp = 10;
    rawHeader.working_mode = 1;
    rawHeader.storage_type = 0;
    rawHeader.valid_line_len = rawHeader.sns_width * (float)rawHeader.bpp / 8.0;
    rawHeader.line_len = ceil((float)rawHeader.valid_line_len / 256.0) * 256.0;
    rawHeader.endian = 1;

    uint32_t fileLen = rawHeader.line_len * rawHeader.sns_height;
    uint8_t* srcRaw = new uint8_t[fileLen];
    uint16_t* dstRaw = new uint16_t[3840 * 2160];
    uint8_t* dstPackedRaw = new uint8_t[fileLen];

    FILE* srcFp = nullptr;
    srcFp = fopen("/sdcard/packed.raw", "r");

    FILE* dstFp = nullptr;
    dstFp = fopen("/sdcard/non_packed.raw", "w");

    FILE* dstPackedFp = nullptr;
    dstPackedFp = fopen("/sdcard/dst_packed.raw", "w");

    if (srcFp != nullptr && dstFp != nullptr && dstPackedFp != nullptr) {
        size_t size = fread(srcRaw, 1, fileLen, srcFp);
        printf("%s   read size: %zu\n", __func__, size);

        ALOGI("%s ConvertCompact2NonCompact start", __func__);
        ConvertCompact2NonCompact(srcRaw, dstRaw, &rawHeader);
        ALOGI("%s ConvertCompact2NonCompact done", __func__);
        fwrite(dstRaw, 1, (3840 * 2160) * 2, dstFp);

        ALOGI("%s ConvertNonCompact2Compact start", __func__);
        ConvertNonCompact2Compact(dstRaw, dstPackedRaw, &rawHeader);
        ALOGI("%s ConvertNonCompact2Compact done", __func__);
        fwrite(dstPackedRaw, 1, fileLen, dstPackedFp);
    } else {
        printf("%s   srcFp: %p dstFp: %p dstPackedFp: %p\n", __func__, srcFp, dstFp, dstPackedFp);
    }

    return 0;
}
