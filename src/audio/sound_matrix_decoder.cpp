#include "sound_matrix_decoder.h"

// Standard matrix decoding gains derived from psychoacoustic principles
namespace Gains {
constexpr float gCenter = 0.7071067811865476f * 0.5f; // -3dB (1/sqrt(2)) * 0.5
constexpr float gFront = 0.5f;                        // -6dB
constexpr float gSurroundPrimary = 0.4359f;           // Primary surround contribution
constexpr float gSurroundSecondary = 0.2449f;         // Cross-feed surround contribution
} // namespace Gains

// Timing constants
namespace Timing {
constexpr int gSurroundDelayMs = 10; // ITU-R BS.775 recommends 10-25ms
}

SoundMatrixDecoder::SoundMatrixDecoder(int32_t sampleRate) {
    // Compute delay length from sample rate
    mDelayLength = (sampleRate * Timing::gSurroundDelayMs) / 1000;
    if (mDelayLength > gMaxDelay) {
        mDelayLength = gMaxDelay;
    }

    // Precompute base rate for all-pass sweep
    mAllPassBaseRate = std::pow(std::pow(2.0, 4.0), 0.1 / (sampleRate / 2.0));

    // Design filters for this sample rate
    mCoefCenterHP = DesignHighPass(70.0, sampleRate);    // Remove rumble from center
    mCoefCenterLP = DesignLowPass(20000.0, sampleRate);  // Anti-alias center
    mCoefSurroundHP = DesignHighPass(100.0, sampleRate); // Surround channels high-passed
    mCoefSubLP = DesignLowPass(120.0, sampleRate);       // LFE low-pass

    // Initialize phase chains with sample rate
    mPhaseLeftMain = {};
    mPhaseLeftCross = {};
    mPhaseRightMain = {};
    mPhaseRightCross = {};
    PrepareAllPass(mPhaseLeftMain, sampleRate);
    PrepareAllPass(mPhaseLeftCross, sampleRate);
    PrepareAllPass(mPhaseRightMain, sampleRate);
    PrepareAllPass(mPhaseRightCross, sampleRate);

    // Reset filter states
    ResetState();
}

void SoundMatrixDecoder::ResetState() {
    // Clear all filter states
    mCenterHighPass = {};
    mCenterLowPass = {};
    mSurrLeftMainHP = {};
    mSurrLeftCrossHP = {};
    mSurrRightMainHP = {};
    mSurrRightCrossHP = {};
    mSubLowPass = {};

    // Reset delay lines
    mDelaySurrLeft = {};
    mDelaySurrLeft.Length = mDelayLength;
    mDelaySurrRight = {};
    mDelaySurrRight.Length = mDelayLength;
}

SoundMatrixDecoder::FilterCoefficients SoundMatrixDecoder::DesignLowPass(double frequency, int32_t sampleRate) {
    FilterCoefficients coef = {};

    // Clamp to safe range for bilinear transform stability
    double maxFreq = sampleRate * 0.475;
    if (frequency > maxFreq) {
        frequency = maxFreq;
    }

    // Linkwitz-Riley 4th order: two cascaded Butterworth 2nd order
    double omega = 2.0 * M_PI * frequency;
    double omega2 = omega * omega;
    double omega3 = omega2 * omega;
    double omega4 = omega2 * omega2;

    // Bilinear transform warping
    double kVal = omega / std::tan(M_PI * frequency / sampleRate);
    double k2 = kVal * kVal;
    double k3 = k2 * kVal;
    double k4 = k2 * k2;

    double rt2 = std::sqrt(2.0);
    double term1 = rt2 * omega3 * kVal;
    double term2 = rt2 * omega * k3;
    double norm = 4.0 * omega2 * k2 + 2.0 * term1 + k4 + 2.0 * term2 + omega4;

    // Feedback coefficients
    coef.B[0] = (4.0 * (omega4 + term1 - k4 - term2)) / norm;
    coef.B[1] = (6.0 * omega4 - 8.0 * omega2 * k2 + 6.0 * k4) / norm;
    coef.B[2] = (4.0 * (omega4 - term1 + term2 - k4)) / norm;
    coef.B[3] = (k4 - 2.0 * term1 + omega4 - 2.0 * term2 + 4.0 * omega2 * k2) / norm;

    // Feedforward coefficients (low-pass response)
    coef.A[0] = omega4 / norm;
    coef.A[1] = 4.0 * omega4 / norm;
    coef.A[2] = 6.0 * omega4 / norm;
    coef.A[3] = coef.A[1];
    coef.A[4] = coef.A[0];

    return coef;
}

SoundMatrixDecoder::FilterCoefficients SoundMatrixDecoder::DesignHighPass(double frequency, int32_t sampleRate) {
    FilterCoefficients coef = {};

    double omega = 2.0 * M_PI * frequency;
    double omega2 = omega * omega;
    double omega3 = omega2 * omega;
    double omega4 = omega2 * omega2;

    double kVal = omega / std::tan(M_PI * frequency / sampleRate);
    double k2 = kVal * kVal;
    double k3 = k2 * kVal;
    double k4 = k2 * k2;

    double rt2 = std::sqrt(2.0);
    double term1 = rt2 * omega3 * kVal;
    double term2 = rt2 * omega * k3;
    double norm = 4.0 * omega2 * k2 + 2.0 * term1 + k4 + 2.0 * term2 + omega4;

    coef.B[0] = (4.0 * (omega4 + term1 - k4 - term2)) / norm;
    coef.B[1] = (6.0 * omega4 - 8.0 * omega2 * k2 + 6.0 * k4) / norm;
    coef.B[2] = (4.0 * (omega4 - term1 + term2 - k4)) / norm;
    coef.B[3] = (k4 - 2.0 * term1 + omega4 - 2.0 * term2 + 4.0 * omega2 * k2) / norm;

    // Feedforward coefficients (high-pass response)
    coef.A[0] = k4 / norm;
    coef.A[1] = -4.0 * k4 / norm;
    coef.A[2] = 6.0 * k4 / norm;
    coef.A[3] = coef.A[1];
    coef.A[4] = coef.A[0];

    return coef;
}

float SoundMatrixDecoder::ProcessFilter(float sample, BiquadCascade& state, const FilterCoefficients& coef) {
    double in = sample;
    double out = coef.A[0] * in + coef.A[1] * state.X[0] + coef.A[2] * state.X[1] + coef.A[3] * state.X[2] +
                 coef.A[4] * state.X[3] - coef.B[0] * state.Y[0] - coef.B[1] * state.Y[1] - coef.B[2] * state.Y[2] -
                 coef.B[3] * state.Y[3];

    // Shift history
    state.X[3] = state.X[2];
    state.X[2] = state.X[1];
    state.X[1] = state.X[0];
    state.X[0] = in;
    state.Y[3] = state.Y[2];
    state.Y[2] = state.Y[1];
    state.Y[1] = state.Y[0];
    state.Y[0] = out;

    return static_cast<float>(out);
}

void SoundMatrixDecoder::PrepareAllPass(AllPassChain& chain, int32_t sampleRate) {
    // Sweeping all-pass parameters for decorrelation
    constexpr double depth = 4.0;
    constexpr double baseDelay = 100.0;
    constexpr double sweepSpeed = 0.1;

    chain.FreqMin = (M_PI * baseDelay) / sampleRate;
    chain.Freq = chain.FreqMin;

    double range = std::pow(2.0, depth);
    chain.FreqMax = (M_PI * baseDelay * range) / sampleRate;
    chain.SweepRate = std::pow(range, sweepSpeed / (sampleRate / 2.0));
    chain.Ready = true;
}

float SoundMatrixDecoder::ProcessAllPass(float sample, AllPassChain& chain, bool negate) {
    // First-order all-pass coefficient
    double c = (1.0 - chain.Freq) / (1.0 + chain.Freq);
    double input = static_cast<double>(sample);

    // Cascade of 4 first-order all-pass sections
    chain.YHist[0] = c * (chain.YHist[0] + input) - chain.XHist[0];
    chain.XHist[0] = input;

    chain.YHist[1] = c * (chain.YHist[1] + chain.YHist[0]) - chain.XHist[1];
    chain.XHist[1] = chain.YHist[0];

    chain.YHist[2] = c * (chain.YHist[2] + chain.YHist[1]) - chain.XHist[2];
    chain.XHist[2] = chain.YHist[1];

    chain.YHist[3] = c * (chain.YHist[3] + chain.YHist[2]) - chain.XHist[3];
    chain.XHist[3] = chain.YHist[2];

    double result = negate ? -chain.YHist[3] : chain.YHist[3];

    // Sweep the frequency for time-varying decorrelation
    chain.Freq *= chain.SweepRate;

    if (chain.Freq > chain.FreqMax) {
        chain.SweepRate = 1.0 / mAllPassBaseRate;
    } else if (chain.Freq < chain.FreqMin) {
        chain.SweepRate = mAllPassBaseRate;
    }

    return static_cast<float>(result);
}

float SoundMatrixDecoder::ProcessDelay(float sample, CircularDelay& buffer) {
    float output = buffer.Data[buffer.Head];
    buffer.Data[buffer.Head] = sample;
    buffer.Head = (buffer.Head + 1) % buffer.Length;
    return output;
}

float SoundMatrixDecoder::Saturate(float value) {
    if (value > 1.0f) {
        return 1.0f;
    }
    if (value < -1.0f) {
        return -1.0f;
    }
    return value;
}

std::tuple<const float*, size_t> SoundMatrixDecoder::Process(const float* stereoInput, size_t samplePairs) {
    // Resize output buffer if needed (6 channels per sample pair)
    size_t samplesNeeded = samplePairs * 6;
    if (mSurroundBuffer.size() < samplesNeeded) {
        mSurroundBuffer.resize(samplesNeeded);
    }

    for (size_t i = 0; i < samplePairs; ++i) {
        float inL = stereoInput[i * 2];
        float inR = stereoInput[i * 2 + 1];

        // Center: sum of L+R, band-limited
        float ctr = (inL + inR) * Gains::gCenter;
        ctr = ProcessFilter(ctr, mCenterHighPass, mCoefCenterHP);
        ctr = ProcessFilter(ctr, mCenterLowPass, mCoefCenterLP);

        // Front channels: attenuated direct signal
        float frontL = inL * Gains::gFront;
        float frontR = inR * Gains::gFront;

        // Surround Left: L primary (inverted phase) + R secondary (shifted phase)
        float slMain = inL * Gains::gSurroundPrimary;
        slMain = ProcessFilter(slMain, mSurrLeftMainHP, mCoefSurroundHP);
        slMain = ProcessAllPass(slMain, mPhaseLeftMain, true);

        float slCross = inR * Gains::gSurroundSecondary;
        slCross = ProcessFilter(slCross, mSurrLeftCrossHP, mCoefSurroundHP);
        slCross = ProcessAllPass(slCross, mPhaseLeftCross, false);

        float surrL = ProcessDelay(slMain + slCross, mDelaySurrLeft);

        // Surround Right: R primary (shifted phase) + L secondary (inverted phase)
        float srMain = inR * Gains::gSurroundPrimary;
        srMain = ProcessFilter(srMain, mSurrRightMainHP, mCoefSurroundHP);
        srMain = ProcessAllPass(srMain, mPhaseRightMain, false);

        float srCross = inL * Gains::gSurroundSecondary;
        srCross = ProcessFilter(srCross, mSurrRightCrossHP, mCoefSurroundHP);
        srCross = ProcessAllPass(srCross, mPhaseRightCross, true);

        float surrR = ProcessDelay(srMain + srCross, mDelaySurrRight);

        // LFE: low-passed sum
        float lfe = (inL + inR) * Gains::gCenter;
        lfe = ProcessFilter(lfe, mSubLowPass, mCoefSubLP);

        // Output: FL, FR, C, LFE, SL, SR
        mSurroundBuffer[i * 6 + 0] = Saturate(frontL);
        mSurroundBuffer[i * 6 + 1] = Saturate(frontR);
        mSurroundBuffer[i * 6 + 2] = Saturate(ctr);
        mSurroundBuffer[i * 6 + 3] = Saturate(lfe);
        mSurroundBuffer[i * 6 + 4] = Saturate(surrL);
        mSurroundBuffer[i * 6 + 5] = Saturate(surrR);
    }

    return { mSurroundBuffer.data(), samplesNeeded };
}
