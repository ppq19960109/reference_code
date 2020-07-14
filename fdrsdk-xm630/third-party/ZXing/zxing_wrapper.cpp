



#include <iostream>
#include <cstring>
#include <string>
#include <algorithm>
#include <cctype>
#include "stdio.h"
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include "BarcodeFormat.h"
#include "DecodeHints.h"
#include "MultiFormatReader.h"
#include "GenericLuminanceSource.h"
#include "HybridBinarizer.h"
#include "Result.h"
#include "ResultMetadata.h"
#include "TextUtfEncoding.h"

#include "zxing_wrapper.h"


#ifdef __cplusplus
extern "C" {
#endif

using namespace ZXing;
using Binarizer = ZXing::HybridBinarizer;

static uint32_t get_system_time_ms(void)
{
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int32_t qrcode_read(char *rt, uint32_t rlen, uint8_t *data, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint32_t t1 = get_system_time_ms();
    std::string code_fmt("QR_CODE");
    DecodeHints hints;
	hints.setShouldTryHarder(1);
	hints.setShouldTryRotate(1);
	auto format = BarcodeFormatFromString("QR_CODE");
	if (format == BarcodeFormat::FORMAT_COUNT) {
        return -1;
    }
    hints.setPossibleFormats({ format });
	MultiFormatReader reader(hints);
	GenericLuminanceSource source(x, y, w, h, data, w); /*yuv format data, get grayscale data*/
	Binarizer binImage(std::shared_ptr<LuminanceSource>(&source, [](void*) {}));

	Result result = reader.read(binImage);
	if (result.isValid()) {
        std::cout << "TM: " << (get_system_time_ms() - t1) << std::endl;
        std::cout << "Len: " << TextUtfEncoding::ToUtf8(result.text()).size() << std::endl;
        strncpy(rt, TextUtfEncoding::ToUtf8(result.text()).c_str(), rlen);
		std::cout << "Text:     " << TextUtfEncoding::ToUtf8(result.text()) << "\n"
		          << "Format:   " << ToString(result.format()) << "\n";
		          //<< "Position: " << result.resultPoints() << "\n";
		auto errLevel = result.metadata().getString(ResultMetadata::Key::ERROR_CORRECTION_LEVEL);
		if (!errLevel.empty()) {
			std::cout << "EC Level: " << TextUtfEncoding::ToUtf8(errLevel) << "\n";
		}
		return 0;
	}

    return -1;
}

#ifdef __cplusplus
}
#endif



