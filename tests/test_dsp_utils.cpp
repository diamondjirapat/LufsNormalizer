#include <iostream>
#include <cmath>
#include <cassert>
#include "dsp/DspUtils.h"

void testPowerToDb() {
    std::cout << "Testing powerToDb..." << std::endl;

    // Standard cases
    assert(std::abs(DspUtils::powerToDb(1.0f) - 0.0f) < 1e-5f);
    assert(std::abs(DspUtils::powerToDb(10.0f) - 10.0f) < 1e-5f);
    assert(std::abs(DspUtils::powerToDb(100.0f) - 20.0f) < 1e-5f);
    assert(std::abs(DspUtils::powerToDb(0.1f) - (-10.0f)) < 1e-5f);

    // Boundary conditions
    // Note: powerToDb returns kSilenceDb if power > kMinLinear is false (i.e. power <= kMinLinear)
    assert(DspUtils::powerToDb(DspUtils::kMinLinear) == DspUtils::kSilenceDb);
    assert(std::abs(DspUtils::powerToDb(DspUtils::kMinLinear * 1.1f) - 10.0f * std::log10(DspUtils::kMinLinear * 1.1f)) < 1e-5f);

    // Edge cases
    assert(DspUtils::powerToDb(0.0f) == DspUtils::kSilenceDb);
    assert(DspUtils::powerToDb(-1.0f) == DspUtils::kSilenceDb);

    std::cout << "powerToDb tests passed!" << std::endl;
}

void testGainToDb() {
    std::cout << "Testing gainToDb..." << std::endl;
    assert(std::abs(DspUtils::gainToDb(1.0f) - 0.0f) < 1e-5f);
    assert(std::abs(DspUtils::gainToDb(2.0f) - 6.020599913f) < 1e-5f);
    assert(DspUtils::gainToDb(0.0f) == DspUtils::kSilenceDb);
    std::cout << "gainToDb tests passed!" << std::endl;
}

void testDbToGain() {
    std::cout << "Testing dbToGain..." << std::endl;
    assert(std::abs(DspUtils::dbToGain(0.0f) - 1.0f) < 1e-5f);
    assert(std::abs(DspUtils::dbToGain(6.020599913f) - 2.0f) < 1e-5f);
    assert(std::abs(DspUtils::dbToGain(-6.020599913f) - 0.5f) < 1e-5f);
    std::cout << "dbToGain tests passed!" << std::endl;
}

int main() {
    testPowerToDb();
    testGainToDb();
    testDbToGain();
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
